#include "Parser.h"
#include <stdarg.h>

CParser::CParser(const CLexer& lexer, CGlobalData* pGlobalData)
{
	m_pFileOut = NULL;
	m_lexer = lexer;
	m_current_token = m_lexer.get_next_token();
	m_pGlobalData = pGlobalData;
}

CParser::~CParser()
{
}

AST* CParser::create_node(AST* node)
{
	if (m_pGlobalData)
		m_pGlobalData->all_nodes().push_back(node);

	return node;
}

void CParser::re_printf(const char* format, ...)
{
	char buffer[1024];
	va_list args;
	int n;
	va_start(args, format);
	n = vsnprintf(buffer, 1024, format, args);
	va_end(args);

	if (m_pFileOut)
		fprintf(m_pFileOut, "%s", buffer);
	else
		printf("%s", buffer);
}

void CParser::error(const string& err, const CToken* pToken)
{
	char buff[1024];
	if (pToken)
		sprintf_s(buff, "%s: %s (file: %s line: %d, column: %d)\r\n", err.c_str(), pToken->value().c_str(), pToken->filename().c_str(), pToken->lineNo(), pToken->column());
	else
		sprintf_s(buff, "%s: %s (file: %s line: %d, column: %d)\r\n", err.c_str(), m_prev_token.value().c_str(), m_prev_token.filename().c_str(), m_prev_token.lineNo(), m_prev_token.column());
	re_printf("%s", buff);
	m_error = buff;
}


void CParser::consume(KEYWORD token_type)
{
	if (m_current_token.type() == token_type)
	{
		m_prev_token = m_current_token;
		m_current_token = m_lexer.get_next_token();
	}
	else
	{
		string strError = "Invalid syntax";
		if (m_prev_token.type() == BUILTIN)
			strError = m_prev_token.value() + " is built-in name";
		error(strError);
	}
}

void CParser::parse_content(const string& content, vector<AST*>& globals, string filename)
{
	CLexer lexer(content,filename);
	CParser parser = CParser(lexer, m_pGlobalData);

	string curdir;
	string temp = filename;
#ifdef _WIN32
	string slash = "\\";
	Tool::str_replace(temp, "/", "\\");
#else
	string slash = "/";
	Tool::str_replace(temp, "\\", "/");
#endif
	string::size_type pos = temp.rfind(slash);
	if (pos != string::npos)
		curdir = temp.substr(0, pos + 1);
	parser.set_curdir(curdir);

	//globals, functions, includes
	while (parser.m_current_token.type() == FUNCTION ||
		parser.m_current_token.type() == SHARP ||
		parser.m_current_token.type() == CLASS ||
		parser.m_current_token.type() == STATIC ||
		parser.m_current_token.type() == ID)
	{
		AST* node = NULL;
		if (parser.m_current_token.type() == SHARP)
		{
			parser.consume(SHARP);
			if (parser.m_current_token.type() == ID && parser.m_current_token.value() == "include")
			{
				parser.consume(ID);
				string filename = parser.m_current_token.value();
				parser.consume(STRING);
				string filecontent = Tool::readfile(filename);
				if (filecontent.size() == 0)
				{
					if (curdir.size()>0)
						filename = curdir + filename;
					filecontent = Tool::readfile(filename);
					if (filecontent.size() == 0)
					{
						parser.error("error include file: " + filename);
						break;
					}
				}
				string strKeyname = "#include " + filename;
				Tool::str_replace(strKeyname, "/", "\\");
				if (m_pGlobalData->globals().find(strKeyname) == m_pGlobalData->globals().end())
				{
					parse_content(filecontent, globals, filename);
					m_pGlobalData->globals()[strKeyname] = true;
				}
			}
			else
			{
				parser.error("error include");
				break;
			}
		}
		else if (parser.m_current_token.type() == ID)
		{
			node = parser.expr();
			if (node->type() != AST::ASSIGN)
			{
				parser.error("error assign");
				break;
			}
			Var* var = (Var*)((Assign*)node)->left();
			if (var->type() != AST::VAR)
			{
				error("error variable");
				break;
			}
			var->set_global(true);
			m_pGlobalData->globals()[var->value()] = true;
			parser.consume(SEMI);
			globals.push_back(node);
		}
		else if (parser.m_current_token.type() == STATIC)
		{
			parser.consume(STATIC);
			if (parser.m_current_token.type() == FUNCTION)
				node = parser.function("",true);
			else if (parser.m_current_token.type() == CLASS)
				node = parser.class_def(true);
			else
			{
				node = parser.expr();
				if (node->type() != AST::ASSIGN)
				{
					parser.error("error assign");
					break;
				}
				Var* var = (Var*)((Assign*)node)->left();
				if (var->type() != AST::VAR)
				{
					error("error variable");
					break;
				}
				var->set_global(true);
				if (var->value().find("|") == string::npos)
				{
					parser.m_statics[var->value()] = STATICTYPE::VAR;
					var->value() = node->token().filename() + "|" + var->value();
				}
				parser.consume(SEMI);
				globals.push_back(node);
			}
		}
		else if (parser.m_current_token.type() == CLASS)
			node = parser.class_def();
		else
			node = parser.function();

		if (parser.m_current_token.type() == SEMI)
			parser.consume(SEMI);
	}

	if (parser.error_msg().size() > 0)
		m_error = parser.error_msg();
}

AST* CParser::parse()
{
	return program();
}
AST* CParser::class_def(bool bStatic)
{
	global_check_init();

	consume(CLASS);
	string class_name = m_current_token.value();
	consume(ID);
	consume(LPAREN);
	if (m_statics.find(class_name) != m_statics.end())
		error(class_name + " already defined as static variable/function. ");

	if (m_pGlobalData->globals().find(class_name) != m_pGlobalData->globals().end())
		error(class_name + " already defined as variable. ");

	if (bStatic)
	{
		m_statics[class_name] = STATICTYPE::FUN;
		class_name = m_current_token.filename() + "|" + class_name;
	}

	if (m_pGlobalData->functions().find(class_name) != m_pGlobalData->functions().end())
	{
		error("redefine function/class: " + class_name);
		//return create_node(new NoOp());
	}

	AST* root = create_node(new ClassStatement(class_name, m_current_token));
	while (m_current_token.type() == ID)
	{
		string param = m_current_token.value();
		((ClassStatement*)root)->params().push_back(param);
		m_global_check.add_param(param);
		consume(ID);

		if (m_current_token.type() == ASSIGN)
		{
			consume(ASSIGN);
			AST* param_value = expr();
			((ClassStatement*)root)->params_value().push_back(param_value);
		}
		else
			((ClassStatement*)root)->params_value().push_back(NULL);

		if (m_current_token.type() == COMMA)
			consume(COMMA);
	}

	for (int i = 0; i < (int)((ClassStatement*)root)->params_value().size()- 1; i++)
	{
		if (((ClassStatement*)root)->params_value()[i] && ((ClassStatement*)root)->params_value()[i + 1] == NULL)
		{
			error("error default arguments: " + class_name);
			break;
		}
	}

	consume(RPAREN);

	m_pGlobalData->functions()[class_name] = root;

	consume(BEGIN);
	vector<AST*> nodes = statement_list(class_name);
	consume(END);

	global_check_start(nodes);

	for (size_t i = 0; i < nodes.size(); i++)
		((ClassStatement*)root)->statements().push_back(nodes[i]);

	return root;
}

AST* CParser::function(string classname, bool bStatic)
{
	global_check_init();

	consume(FUNCTION);
	string fun_name = m_current_token.value();
	if (classname.size()>0)
		fun_name = classname + "." + fun_name;
	consume(ID);
	consume(LPAREN);

	if (m_statics.find(fun_name) != m_statics.end())
		error(fun_name + " already defined as static variable/function. ");

	if (m_pGlobalData->globals().find(fun_name) != m_pGlobalData->globals().end())
		error(fun_name + " already defined as variable. ");

	if (bStatic)
	{
		m_statics[fun_name] = STATICTYPE::FUN;
		fun_name = m_current_token.filename() + "|" + fun_name;
	}

	if (m_pGlobalData->functions().find(fun_name) != m_pGlobalData->functions().end())
	{
		error("redefine function: " + fun_name);
		//return create_node(new NoOp());
	}

	AST* root = create_node(new FunctionStatement(fun_name, m_current_token));
	if (classname.size()>0)
		((FunctionStatement*)root)->params().push_back("self");
	while (m_current_token.type() == ID)
	{
		string param = m_current_token.value();
		((FunctionStatement*)root)->params().push_back(param);
		m_global_check.add_param(param);
		consume(ID);
		if (m_current_token.type() == ASSIGN)
		{
			consume(ASSIGN);
			AST* param_value = expr();
			((FunctionStatement*)root)->params_value().push_back(param_value);
		}
		else
			((FunctionStatement*)root)->params_value().push_back(NULL);


		if (m_current_token.type() == COMMA)
			consume(COMMA);
	}

	for (int i = 0; i < (int)((FunctionStatement*)root)->params_value().size()-1;i++)
	{
		if (((FunctionStatement*)root)->params_value()[i] && ((FunctionStatement*)root)->params_value()[i + 1] == NULL)
		{
			error("error default arguments: " + fun_name);
			break;
		}
	}

	consume(RPAREN);

	m_pGlobalData->functions()[fun_name] = root;

	consume(BEGIN);
	vector<AST*> nodes = statement_list();
	consume(END);

	global_check_start(nodes);

	for (size_t i = 0; i < nodes.size(); i++)
		((FunctionStatement*)root)->statements().push_back(nodes[i]);

	return root;
}
AST* CParser::function_exec()
{
	string fun_name = m_current_token.value();
	consume(ID);

	AST* save = NULL;
	if (m_statics.find(fun_name) != m_statics.end())
		fun_name = m_current_token.filename() + "|" + fun_name;
	if (m_pGlobalData->functions().find(fun_name) != m_pGlobalData->functions().end())
		save = m_pGlobalData->functions()[fun_name];

	AST* node = create_node(new FunctionExec(save,m_current_token));

	if (m_current_token.type() == LPAREN)
	{
		consume(LPAREN);
		if (m_current_token.type() != RPAREN)
		{
			((FunctionExec*)node)->exprs().push_back(expr());
			while (m_current_token.type() == COMMA)
			{
				consume(COMMA);
				((FunctionExec*)node)->exprs().push_back(expr());
			}
		}
		consume(RPAREN);
	}
	else
		((FunctionExec*)node)->set_var(true);

	return node;
}

AST* CParser::program()
{
	vector<AST*> globals;

	//includes, globals, functions 
	while (m_current_token.type() == FUNCTION || 
		m_current_token.type() == SHARP ||
		m_current_token.type() == CLASS ||
		m_current_token.type() == STATIC ||
		m_current_token.type() == ID)
	{

		AST* node=NULL;
		if (m_current_token.type() == SHARP)
		{
			consume(SHARP);
			if (m_current_token.type() == ID && m_current_token.value() == "include")
			{
				consume(ID);
				string filename = m_current_token.value();
				consume(STRING);
				string filecontent = Tool::readfile(filename);
				if (filecontent.size() == 0)
				{
					if (m_curdir.size()>0)
						filename = m_curdir + filename;
					filecontent = Tool::readfile(filename);
					if (filecontent.size() == 0)
					{
						error("error include file: " + filename);
						break;
					}
				}
				string strKeyname = "#include " + filename;
				Tool::str_replace(strKeyname, "/", "\\");
				if (m_pGlobalData->globals().find(strKeyname) == m_pGlobalData->globals().end())
				{
					parse_content(filecontent, globals, filename);
					m_pGlobalData->globals()[strKeyname] = true;
				}

			}
			else
			{
				error("error include");
				break;
			}
		}
		else if(m_current_token.type() == ID)
		{
			node = expr();
			if (node->type() != AST::ASSIGN)
			{
				error("error defination");
				break;
			}
			Var* var=(Var*)((Assign*)node)->left();
			if (var->type()!=AST::VAR)
			{
				error("error variable");
				break;
			}
			var->set_global(true);
			m_pGlobalData->globals()[var->value()]=true;
			consume(SEMI);
			globals.push_back(node);
		}
		else if (m_current_token.type() == STATIC)
		{
			consume(STATIC);
			if (m_current_token.type() == FUNCTION)
				node = function("",true);
			else if (m_current_token.type() == CLASS)
				node = class_def(true);
			else
			{
				node = expr();
				if (node->type() != AST::ASSIGN)
				{
					error("error defination");
					break;
				}
				Var* var = (Var*)((Assign*)node)->left();
				if (var->type() != AST::VAR)
				{
					error("error variable");
					break;
				}
				var->set_global(true);
				if (var->value().find("|") == string::npos)
				{
					m_statics[var->value()] = STATICTYPE::VAR;
					var->value() = node->token().filename() + "|" + var->value();
				}
				consume(SEMI);
				globals.push_back(node);
			}
		}
		else if (m_current_token.type() == CLASS)
			node = class_def();
		else
			node = function();

		if (m_current_token.type() == SEMI)
			consume(SEMI);
	}


	//main
	consume(PROGRAM);
	consume(LPAREN);
	vector<string> params;
	while (m_current_token.type() == ID)
	{
		string param = m_current_token.value();
		params.push_back(param);
		consume(ID);
		if (m_current_token.type() == COMMA)
			consume(COMMA);
	}
	consume(RPAREN);

	AST* bk = block();
	if (m_current_token.type() != EOFI)
		error("error EOF");

	AST* root = create_node(new Program("main", (Block*)bk));
	for (size_t i = 0; i < params.size(); i++)
		((Program*)root)->params().push_back(params[i]);

	for (size_t i = 0; i<globals.size(); i++)
		((Program*)root)->globals().push_back(globals[i]);

	return root;
}
AST* CParser::block()
{
	AST* compound_node = compound_statement();
	AST* node = create_node(new Block((Compound*)compound_node));
	return node;
}
AST* CParser::compound_statement()
{
	consume(BEGIN);
	vector<AST*> nodes=statement_list();
	consume(END);

	AST* root = create_node(new Compound());
	for (size_t i = 0; i < nodes.size(); i++)
		((Compound*)root)->child().push_back(nodes[i]);

	return root;
}

bool CParser::CanSkipSEMI(AST* node)
{
	bool bSkipSEMI = false;
	if (node->type() == AST::WHILECOMPOUND
		|| node->type() == AST::IFCOMPOUND
		|| node->type() == AST::FORCOMPOUND
		|| node->type() == AST::INCLUDE
		|| node->type() == AST::FUNCTION
		|| node->type() == AST::CLASS)
	{
		if (m_current_token.type() != SEMI)//while(if,for) may have no SEMI at the end.
			bSkipSEMI = true;
	}

	return bSkipSEMI;
}
vector<AST*> CParser::statement_list(string classname)
{
	AST* node = statement(classname);
	vector<AST*> nodes;
	nodes.push_back(node);

	bool bSkipSEMI = CanSkipSEMI(node);
	while (m_current_token.type() == SEMI || bSkipSEMI)
	{
		if (!bSkipSEMI)
			consume(SEMI);
		AST* ret_statement = statement(classname);
		if (ret_statement->type() != AST::EMPTY)
			nodes.push_back(ret_statement);
		
		bSkipSEMI = CanSkipSEMI(ret_statement);
	}
	return nodes;
}
AST* CParser::statement(string classname)
{
	AST* node = NULL;
	if (m_current_token.type() == BEGIN)
	{
		node = compound_statement();
	}
	else if (m_current_token.type() == ID || 
		m_current_token.type() == BUILTIN ||
		m_current_token.type() == STRING ||
		m_current_token.type() == LPAREN)
	{
		node = expr();
	}
	else if (m_current_token.type() == IF)
	{
		node = if_statement();
	}
	else if (m_current_token.type() == WHILE)
	{
		node = while_statement();
	}
	else if (m_current_token.type() == FOR)
	{
		node = for_statement();
	}
	else if (m_current_token.type() == BREAK)
	{
		node = break_statement();
	}
	else if (m_current_token.type() == CONTINUE)
	{
		node = continue_statement();
	}
	else if (m_current_token.type() == RETURN)
	{
		node = return_statement();
	}
	//else if (m_current_token.type() == BUILTIN)
	//{
		//node = builtin_statement();
	//}
	else if (m_current_token.type() == FUNCTION)
	{
		node = function(classname);
	}
	else if (m_current_token.type() == CLASS)
	{
		node = class_def();
	}
	else if (m_current_token.type() == SHARP)
	{
		node = include_statement();
	}
	else if (m_current_token.type() == GLOBAL)
	{
		node = global_statement();
	}

	if (node==NULL)
		node = create_node(new NoOp());
	return node;
}
AST* CParser::include_statement()
{
	vector<AST*> globals;
	consume(SHARP);
	if (m_current_token.type() == ID && m_current_token.value() == "include")
	{
		consume(ID);
		string filename = m_current_token.value();
		consume(STRING);
		string filecontent = Tool::readfile(filename);
		if (filecontent.size() == 0)
		{
			if (m_curdir.size()>0)
				filename = m_curdir + filename;
			filecontent = Tool::readfile(filename);
			if (filecontent.size() == 0)
				error("error include file: " + filename);
		}
		string strKeyname = "#include " + filename;
		Tool::str_replace(strKeyname, "/", "\\");
		if (m_pGlobalData->globals().find(strKeyname) == m_pGlobalData->globals().end())
		{
			parse_content(filecontent, globals, filename);
			m_pGlobalData->globals()[strKeyname] = true;
		}
	}
	else
	{
		error("error include");
	}
	AST* node = create_node(new IncludeStatement(m_current_token));
	for (size_t i = 0; i<globals.size(); i++)
		((IncludeStatement*)node)->globals().push_back(globals[i]);

	return node;
}
AST* CParser::builtin_statement()
{
	AST* node = create_node(new BuiltinStatement(m_current_token));
	consume(BUILTIN);
	consume(LPAREN);
	if (m_current_token.type() != RPAREN)
	{
		((BuiltinStatement*)node)->exprs().push_back(expr());
		while (m_current_token.type() == COMMA)
		{
			consume(COMMA);
			((BuiltinStatement*)node)->exprs().push_back(expr());
		}
	}
	consume(RPAREN);

	return node;
}
AST* CParser::break_statement()
{
	consume(BREAK);
	AST* node = create_node(new BreakStatement(m_current_token));
	return node;
}
AST* CParser::continue_statement()
{
	consume(CONTINUE);
	AST* node = create_node(new ContinueStatement(m_current_token));
	return node;
}
AST* CParser::return_statement()
{
	consume(RETURN);
	AST* exp = NULL;
	if (m_current_token.type() == SEMI)
		exp = create_node(new NoOp());
	else
		exp = expr();
	AST* node = create_node(new ReturnStatement(m_current_token, exp));
	return node;
}
AST* CParser::for_statement()
{
	consume(FOR);
	consume(LPAREN);
	vector<AST*> init;
	if (m_current_token.type() != SEMI)
	{
		init.push_back(statement());
		while (m_current_token.type() == COMMA)
		{
			consume(COMMA);
			init.push_back(statement());
		}
	}
	consume(SEMI);

	AST* exp = NULL;
	if (m_current_token.type() != SEMI)
		exp = expr();
	else
		exp = create_node(new Num(CToken(INTEGER, "1", m_current_token.lineNo(), m_current_token.column(), m_current_token.filename())));
	consume(SEMI);

	vector<AST*> update;
	if (m_current_token.type() != RPAREN)
	{
		update.push_back(statement());
		while (m_current_token.type() == COMMA)
		{
			consume(COMMA);
			update.push_back(statement());
		}
	}
	consume(RPAREN);

	AST* root = create_node(new ForCompound(m_current_token, init, exp, update));

	if (m_current_token.type() == BEGIN)
	{
		consume(BEGIN);
		vector<AST*> nodes = statement_list();
		consume(END);
		for (size_t i = 0; i < nodes.size(); i++)
			((ForCompound*)root)->statements().push_back(nodes[i]);
	}
	else
	{
		((ForCompound*)root)->statements().push_back(statement());
	}

	return root;
}
AST* CParser::while_statement()
{
	consume(WHILE);
	consume(LPAREN);
	AST* exp = expr();
	consume(RPAREN);

	AST* root = create_node(new WhileCompound(m_current_token, exp));

	if (m_current_token.type() == BEGIN)
	{
		consume(BEGIN);
		vector<AST*> nodes = statement_list();
		consume(END);
		for (size_t i = 0; i < nodes.size(); i++)
			((WhileCompound*)root)->statements().push_back(nodes[i]);
	}
	else
	{
		((WhileCompound*)root)->statements().push_back(statement());
	}

	return root;
}
AST* CParser::if_statement()
{
	consume(IF);
	consume(LPAREN);
	AST* exp = expr();
	consume(RPAREN);

	AST* root = create_node(new IfCompound(m_current_token, exp));

	if (m_current_token.type() == BEGIN)
	{
		consume(BEGIN);
		vector<AST*> true_nodes = statement_list();
		consume(END);
		for (size_t i = 0; i < true_nodes.size(); i++)
			((IfCompound*)root)->true_statements().push_back(true_nodes[i]);
	}
	else
	{
		((IfCompound*)root)->true_statements().push_back(statement());
		if (m_current_token.type() == SEMI)
		{
			CToken token = m_lexer.peek_next_token();
			if (token.type() == ELSE)
				consume(SEMI);
		}
	}

	if (m_current_token.type() == ELSE)
	{
		consume(ELSE);
		if (m_current_token.type() == BEGIN)
		{
			consume(BEGIN);
			vector<AST*> false_nodes = statement_list();
			consume(END);
			for (size_t i = 0; i < false_nodes.size(); i++)
				((IfCompound*)root)->false_statements().push_back(false_nodes[i]);
		}
		else
		{
			((IfCompound*)root)->false_statements().push_back(statement());
		}
	}

	return root;
}

AST* CParser::factor()
{
	CToken token = m_current_token;
	if (token.type() == INTEGER)
	{
		consume(INTEGER);
		return create_node(new Num(token));
	}
	if (token.type() == REAL)
	{
		consume(REAL);
		return create_node(new Num(token, Num::FLOAT));
	}
	if (token.type() == STRING)
	{
		consume(STRING);
		return create_node(new Str(token));
	}
	if (token.type() == BTRUE || token.type() == BFALSE)
	{
		consume(token.type());
		return create_node(new Bool(token));
	}
	if (token.type() == NONE)
	{
		consume(NONE);
		return create_node(new None());
	}
	if (token.type() == LPAREN)
	{
		consume(LPAREN);
		AST* node = expr();
		consume(RPAREN);
		return node;
	}
	if (token.type() == ID)
	{
		AST* node = NULL;
		bool bFun = false;
		if (m_pGlobalData->functions().find(token.value()) != m_pGlobalData->functions().end())
			bFun = true;
		else if (m_statics.find(token.value()) != m_statics.end())
			if (m_statics[token.value()] == STATICTYPE::FUN)
				bFun = true;

		if (bFun)
			node = function_exec();
		else
			node = variable();
		return node;
	}
	if (token.type() == BUILTIN)
	{
		AST* node = builtin_statement();
		return node;
	}
	if (token.type() == MINUS)
	{
		consume(MINUS);
		token = m_current_token;
		if (token.type() == INTEGER)
		{
			consume(INTEGER);
			Num* pNum = new Num(token);
			pNum->set_minus();
			return create_node(pNum);
		}
		else if (token.type() == REAL)
		{
			consume(REAL);
			Num* pNum = new Num(token, Num::FLOAT);
			pNum->set_minus();
			return create_node(pNum);
		}
	}

	error("error factor.");
	return create_node(new NoOp());
}

AST* CParser::term_square_dot()
{
	AST* node = factor();
	while (m_current_token.type() == LSQUARE ||
		m_current_token.type() == DOT
		)
	{
		if (m_current_token.type() == LSQUARE)
		{
			CToken token = m_current_token;
			consume(LSQUARE);
			AST* array_index = expr();
			consume(RSQUARE);
			node = create_node(new BinOp(node, token, array_index));
		}
		else if (m_current_token.type() == DOT)
		{
			CToken token = m_current_token;
			consume(DOT);
			CToken member_token = m_current_token;
			consume(ID);
			Member::MEMBERTYPE memtype = Member::MEMBERTYPE::VAR;
			if (m_current_token.type() == LPAREN)
				memtype = Member::MEMBERTYPE::FUN;
			AST* member = create_node(new Member(member_token, memtype));

			if (m_current_token.type() == LPAREN)
			{
				consume(LPAREN);
				if (m_current_token.type() != RPAREN)
				{
					((Member*)member)->exprs().push_back(expr());
					while (m_current_token.type() == COMMA)
					{
						consume(COMMA);
						((Member*)member)->exprs().push_back(expr());
					}
				}
				consume(RPAREN);
				if (((Member*)member)->exprs().size() == 0)
					((Member*)member)->exprs().push_back(NULL);
			}

			node = create_node(new BinOp(node, token, member));
		}

	}

	return node;

}

AST* CParser::term_not()
{
	if (m_current_token.type() == NOT)
	{
		CToken token = m_current_token;
		consume(token.type());
		AST* nullNode = create_node(new NoOp());
		AST* node = create_node(new BinOp(nullNode, token, term_square_dot()));
		return  node;
	}

	AST* node = term_square_dot();
	return node;
}

AST* CParser::term_mul_div()
{
	AST* node = term_not();
	while (m_current_token.type() == MUL ||
		m_current_token.type() == DIV ||
		m_current_token.type() == MOD 
		)
	{
		CToken token = m_current_token;
		consume(token.type());
		node = create_node(new BinOp(node, token, term_not()));
	}

	return node;

}
AST* CParser::term_plus_minus()
{
	AST* node = term_mul_div();
	while (m_current_token.type() == PLUS ||
		m_current_token.type() == MINUS
		)
	{
		CToken token = m_current_token;
		consume(token.type());
		node = create_node(new BinOp(node, token, term_mul_div()));
	}

	return node;
}
AST* CParser::term_comparison()
{
	AST* node = term_plus_minus();
	while (m_current_token.type() == GREATER ||
		m_current_token.type() == LESS ||
		m_current_token.type() == GREATER_EQUAL ||
		m_current_token.type() == LESS_EQUAL
		)
	{
		CToken token = m_current_token;
		consume(token.type());
		node = create_node(new BinOp(node, token, term_plus_minus()));
	}

	return node;
}
AST* CParser::term_equal()
{
	AST* node = term_comparison();
	while (m_current_token.type() == EQUAL ||
		m_current_token.type() == NOT_EQUAL
		)
	{
		CToken token = m_current_token;
		consume(token.type());
		node = create_node(new BinOp(node, token, term_comparison()));
	}

	return node;
}
AST* CParser::term_and()
{
	AST* node = term_equal();
	while (m_current_token.type() == AND)
	{
		CToken token = m_current_token;
		consume(token.type());
		node = create_node(new BinOp(node, token, term_equal()));
	}

	return node;
}
AST* CParser::term_or()
{
	AST* node = term_and();
	while (m_current_token.type() == OR)
	{
		CToken token = m_current_token;
		consume(token.type());
		node = create_node(new BinOp(node, token, term_and()));
	}

	return node;
}
AST* CParser::term_plus_plus()
{
	AST* node = term_or();
	if (m_current_token.type() == PLUS_PLUS || m_current_token.type() == MINUS_MINUS)
	{
		m_global_check.add_assign(node->token().value());

		CToken token = m_current_token;
		consume(token.type());
		//node = create_node(new BinOp(node, token, create_node(new NoOp())));

		AST* left_child = node;
		AST* right_child = create_node(new Num(CToken(INTEGER, "1",token.lineNo(),token.column(),token.filename())));
		CToken child_token = CToken(PLUS, "+", token.lineNo(), token.column(), token.filename());
		if (token.type() == MINUS_MINUS)
			child_token = CToken(MINUS, "-", token.lineNo(), token.column(), token.filename());
		AST* right = create_node(new BinOp(left_child, child_token, right_child));
		node = create_node(new Assign(node, CToken(ASSIGN, "=", token.lineNo(), token.column(), token.filename()), right));

	}
	else if (m_current_token.type() == PLUS_EQUAL || m_current_token.type() == MINUS_EQUAL ||
		m_current_token.type() == MUL_EQUAL || m_current_token.type() == DIV_EQUAL || m_current_token.type() == MOD_EQUAL
		)
	{
		m_global_check.add_assign(node->token().value());

		CToken token = m_current_token;
		consume(token.type());

		AST* left_child = node;
		AST* right_child = term_or();
		CToken child_token = CToken(PLUS, "+", token.lineNo(), token.column(), token.filename());
		if (token.type() == MINUS_EQUAL)
			child_token = CToken(MINUS, "-", token.lineNo(), token.column(), token.filename());
		else if (token.type() == MUL_EQUAL)
			child_token = CToken(MUL, "*", token.lineNo(), token.column(), token.filename());
		else if (token.type() == DIV_EQUAL)
			child_token = CToken(DIV, "/", token.lineNo(), token.column(), token.filename());
		else if (token.type() == MOD_EQUAL)
			child_token = CToken(MOD, "%", token.lineNo(), token.column(), token.filename());
		AST* right = create_node(new BinOp(left_child, child_token, right_child));
		node = create_node(new Assign(node, CToken(ASSIGN, "=", token.lineNo(), token.column(), token.filename()), right));
	}

	return node;
}
AST* CParser::expr()
{
	CToken prev_token = m_current_token;
	
	AST* node = term_plus_plus();
	if (m_current_token.type() == ASSIGN)
	{
		m_global_check.add_assign(node->token().value());

		CToken token = m_current_token;
		consume(token.type());
		
		if (m_current_token.type() == LSQUARE || m_current_token.type() == BEGIN)//array,dict
			return assignment_array_dict(prev_token, token, node);

		node = create_node(new Assign(node, token, term_plus_plus()));
	}

	return node;
}
AST* CParser::variable()
{
	CToken token = m_current_token;
	AST* node = create_node(new Var(token));
	consume(ID);

	Var* var=(Var*)node;
	if (m_pGlobalData->globals().find(var->value()) != m_pGlobalData->globals().end())
		var->set_global(true);
	if (m_statics.find(var->value()) != m_statics.end())
	{
		var->set_global(true);
		var->value() = var->token().filename() + "|" + var->value();
	}
	if (var->global())
		m_global_check.add_node(var);

	if (m_current_token.type() == LPAREN)
	{
		var->set_func(true);
		consume(LPAREN);
		if (m_current_token.type() != RPAREN)
		{
			var->exprs().push_back(expr());
			while (m_current_token.type() == COMMA)
			{
				consume(COMMA);
				var->exprs().push_back(expr());
			}
		}
		consume(RPAREN);
	}

	return node;
}
AST* CParser::array_dict(const CToken& prev_token)
{
	if (m_current_token.type() == LSQUARE)
	{
		consume(LSQUARE);
		AST* array = create_node(new Array(prev_token, NULL));
		if (m_current_token.type() != RSQUARE)
		{
			if (m_current_token.type() == LSQUARE || m_current_token.type() == BEGIN)
				((Array*)array)->inits().push_back(array_dict(prev_token));
			else
				((Array*)array)->inits().push_back(expr());

			while (m_current_token.type() == COMMA)
			{
				consume(COMMA);
				if (m_current_token.type() == LSQUARE || m_current_token.type() == BEGIN)
					((Array*)array)->inits().push_back(array_dict(prev_token));
				else
					((Array*)array)->inits().push_back(expr());
			}
		}
		consume(RSQUARE);
		return array;
	}
	if (m_current_token.type() == BEGIN)
	{
		consume(BEGIN);
		AST* dict = create_node(new Dict(prev_token));
		if (m_current_token.type() != END)
		{
			if (m_current_token.type() == LSQUARE || m_current_token.type() == BEGIN)
				((Dict*)dict)->inits_left().push_back(array_dict(prev_token));
			else
				((Dict*)dict)->inits_left().push_back(expr());
			consume(COLON);
			if (m_current_token.type() == LSQUARE || m_current_token.type() == BEGIN)
				((Dict*)dict)->inits_right().push_back(array_dict(prev_token));
			else
				((Dict*)dict)->inits_right().push_back(expr());

			while (m_current_token.type() == COMMA)
			{
				consume(COMMA);

				if (m_current_token.type() == LSQUARE || m_current_token.type() == BEGIN)
					((Dict*)dict)->inits_left().push_back(array_dict(prev_token));
				else
					((Dict*)dict)->inits_left().push_back(expr());
				consume(COLON);
				if (m_current_token.type() == LSQUARE || m_current_token.type() == BEGIN)
					((Dict*)dict)->inits_right().push_back(array_dict(prev_token));
				else
					((Dict*)dict)->inits_right().push_back(expr());
			}
		}
		consume(END);
		return dict;
	}

	return NULL;
}

AST* CParser::assignment_array_dict(const CToken& prev_token, const CToken& token, AST* left)
{
	if (m_current_token.type() == LSQUARE)//array
	{
		consume(LSQUARE);
		AST* right = create_node(new Array(prev_token, NULL));
		if (m_current_token.type() != RSQUARE)
		{
			if (m_current_token.type() == LSQUARE || m_current_token.type() == BEGIN)
				((Array*)right)->inits().push_back(array_dict(prev_token));
			else
				((Array*)right)->inits().push_back(expr());

			while (m_current_token.type() == COMMA)
			{
				consume(COMMA);
				if (m_current_token.type() == LSQUARE || m_current_token.type() == BEGIN)
					((Array*)right)->inits().push_back(array_dict(prev_token));
				else
					((Array*)right)->inits().push_back(expr());
			}
		}
		consume(RSQUARE);

		AST* node = create_node(new Assign(left, token, right));
		return node;
	}
	if (m_current_token.type() == BEGIN)//dict
	{
		AST* right = create_node(new Dict(prev_token));

		consume(BEGIN);
		if (m_current_token.type() != END)
		{
			if (m_current_token.type() == LSQUARE || m_current_token.type() == BEGIN)
				((Dict*)right)->inits_left().push_back(array_dict(prev_token));
			else
				((Dict*)right)->inits_left().push_back(expr());
			consume(COLON);
			if (m_current_token.type() == LSQUARE || m_current_token.type() == BEGIN)
				((Dict*)right)->inits_right().push_back(array_dict(prev_token));
			else
				((Dict*)right)->inits_right().push_back(expr());

			while (m_current_token.type() == COMMA)
			{
				consume(COMMA);

				if (m_current_token.type() == LSQUARE || m_current_token.type() == BEGIN)
					((Dict*)right)->inits_left().push_back(array_dict(prev_token));
				else
					((Dict*)right)->inits_left().push_back(expr());
				consume(COLON);
				if (m_current_token.type() == LSQUARE || m_current_token.type() == BEGIN)
					((Dict*)right)->inits_right().push_back(array_dict(prev_token));
				else
					((Dict*)right)->inits_right().push_back(expr());
			}
		}
		consume(END);

		AST* node = create_node(new Assign(left, token, right));
		return node;
	}

	error("assign array/dict error.");
	return create_node(new NoOp());
}

AST* CParser::global_statement()
{
	AST* node = create_node(new GlobalStatement(m_current_token));
	consume(GLOBAL);
	if (m_current_token.type() != ID)
	{
		error("invalid global declaration.");
		return node;
	}
	((GlobalStatement*)node)->vars()[m_current_token.value()] = true;
	consume(m_current_token.type());
	while (m_current_token.type() == COMMA)
	{
		consume(COMMA);
		if (m_current_token.type() != ID)
		{
			error("invalid global declaration.");
			break;
		}
		((GlobalStatement*)node)->vars()[m_current_token.value()] = true;
		consume(m_current_token.type());
	}

	return node;
}
void  CParser::global_check_init()
{
	m_global_check.push();
}
void  CParser::global_check_start(const vector<AST*>& nodes)
{
	vector<AST*> &m_global_nodes = m_global_check.top().nodes();
	map<string, bool> &m_param_vars = m_global_check.top().params();
	map<string, bool> &m_assign_vars = m_global_check.top().assigns();

	vector<GlobalStatement*> vec_global_vars;
	for (size_t i = 0; i < nodes.size(); i++)
	{
		if (nodes[i]->type() == AST::GLOBAL)
			vec_global_vars.push_back((GlobalStatement*)nodes[i]);
	}

	for (size_t i = 0; i < m_global_nodes.size(); i++)
	{
		if (m_assign_vars.find(m_global_nodes[i]->token().value()) != m_assign_vars.end())
		{
			bool bGlobal = false;
			for (size_t j = 0; j < vec_global_vars.size(); j++)
			{
				if (vec_global_vars[j]->vars().find(m_global_nodes[i]->token().value()) != vec_global_vars[j]->vars().end())
				{
					if (vec_global_vars[j]->token().lineNo() > m_global_nodes[i]->token().lineNo())
					{
						error("the variable " + m_global_nodes[i]->token().value() + " is used before global declaration.", &m_global_nodes[i]->token());
						break;
					}

					bGlobal = true;
					break;
				}
			}
			if (!bGlobal)
			{
				Var* var = (Var*)m_global_nodes[i];
				var->value() = var->token().value();
				var->set_global(false);
			}
		}
		if (m_param_vars.find(m_global_nodes[i]->token().value()) != m_param_vars.end())
		{
			for (size_t j = 0; j < vec_global_vars.size(); j++)
			{
				if (vec_global_vars[j]->vars().find(m_global_nodes[i]->token().value()) != vec_global_vars[j]->vars().end())
				{
					error("the variable " + m_global_nodes[i]->token().value() + " cannot be both global and parameter.", &m_global_nodes[i]->token());
					break;
				}
			}
			Var* var = (Var*)m_global_nodes[i];
			var->value() = var->token().value();
			var->set_global(false);
		}
	}

	m_global_check.pop();
}
