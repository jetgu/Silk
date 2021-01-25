//A C-like language Interpreter, actually it's more similar to Python
//Created by Gu Hong 2019/10

#include "Interpreter.h"
#include <stdarg.h>

extern CInterpreter* _pInterpreter;

struct threadParams
{
	FunctionExec* func;
	CGlobalData* globaldata;
	CCallStack callstack;
};
#ifdef _WIN32
	#include <process.h>
	void newthread(void* pParam)
	{
		//printf("new thread\r\n");
		threadParams *pParams = (threadParams*)pParam;
		CGlobalData *globaldata = pParams->globaldata;
		FunctionExec* func = pParams->func;
		CCallStack callstack = pParams->callstack;
		delete pParams;

		if (globaldata == NULL) return;
		if (func == NULL) return;

		//we need to have a new Interpreter which has a new callstack for thread.
		CLexer lexer("", "");
		CParser parser = CParser(lexer, globaldata);
		CInterpreter interp = CInterpreter(parser);
		interp.set_callstack(callstack);
		CVariable result;
		interp.visit(func,result);
	
		delete func;
		return;
	}
#else
	void* newthread(void* pParam)
	{
		//printf("new thread\r\n");
		threadParams *pParams = (threadParams*)pParam;
		CGlobalData *globaldata = pParams->globaldata;
		FunctionExec* func = pParams->func;
		CCallStack callstack = pParams->callstack;
		delete pParams;

		if (globaldata == NULL) return NULL;
		if (func == NULL) return NULL;

		//we need to have a new Interpreter which has a new callstack for thread.
		CLexer lexer("", "");
		CParser parser = CParser(lexer, globaldata);
		CInterpreter interp = CInterpreter(parser);
		interp.set_callstack(callstack);
		CVariable result;
		interp.visit(func,result);

		delete func;
		return NULL;
	}
#endif

CInterpreter::CInterpreter(const CParser& parser)
{
	m_pFileOut = NULL;
	m_parser = parser;
	if (_pInterpreter==NULL)
		_pInterpreter = this;
}
CInterpreter::~CInterpreter()
{
}

void CInterpreter::error(const string& err, const CToken& token)
{
	char buff[512];
	sprintf_s(buff, "%s: %s (file: %s line: %d, column: %d)\r\n", err.c_str(), token.value().c_str(), token.filename().c_str(), token.lineNo(), token.column());
	re_printf("%s",buff);
	m_error = buff;
}

void CInterpreter::visit(AST* node, CVariable& res)
{
	res.reset();
	if (node == NULL || node->type() == AST::EMPTY || m_error.size() > 0)
		return;

	AST::ASTTYPE type = node->type();
	if (type == AST::BINOP)
	{
		visit_BinOp((BinOp*)node, res);
		if (res.tag() == CVariable::ERR)
			error("unsupported operand type", node->token());
	}
	else if (type == AST::NUM)
	{
		visit_Num((Num*)node,res);
	}
	else if (type == AST::BOOL)
	{
		visit_Bool((Bool*)node,res);
	}
	else if (type == AST::STR)
	{
		visit_Str((Str*)node,res);
	}
	else if (type == AST::ARRAY)
	{
		visit_Array((Array*)node,res);
	}
	else if (type == AST::DICT)
	{
		visit_Dict((Dict*)node,res);
	}
	else if (type == AST::VAR)
	{
		visit_Var((Var*)node, res);
	}
	else if (type == AST::ASSIGN)
	{
		visit_Assign((Assign*)node, res);
	}
	else if (type == AST::PROGRAM)
	{
		visit_Program((Program*)node,res);
	}
	else if (type == AST::BLOCK)
	{
		visit_Block((Block*)node,res);
	}
	else if (type == AST::COMPOUND)
	{
		visit_Compound((Compound*)node,res);
	}
	else if (type == AST::IFCOMPOUND)
	{
		visit_IfCompound((IfCompound*)node,res);
	}
	else if (type == AST::WHILECOMPOUND)
	{
		visit_WhileCompound((WhileCompound*)node,res);
	}
	else if (type == AST::FORCOMPOUND)
	{
		visit_ForCompound((ForCompound*)node,res);
	}
	else if (type == AST::BREAK)
	{
		visit_Break((BreakStatement*)node,res);
	}
	else if (type == AST::CONTINUE)
	{
		visit_Continue((ContinueStatement*)node,res);
	}
	else if (type == AST::RETURN)
	{
		visit_Return((ReturnStatement*)node,res);
	}
	else if (type == AST::BUILTIN)
	{
		visit_Builtin((BuiltinStatement*)node,res);
	}
	else if (type == AST::FUNCTION)
	{
		visit_Function((FunctionStatement*)node,res);
	}
	else if (type == AST::FUNCTION_EXEC)
	{
		visit_FunctionExec((FunctionExec*)node,res);
	}
	else if (type == AST::INCLUDE)
	{
		visit_Include((IncludeStatement*)node,res);
	}
	else if (type == AST::NONE)
	{
		res.setType(CVariable::NONE);
	}
}

void CInterpreter::visit_BinOp(BinOp* node, CVariable& res)
{
	KEYWORD type = node->token().type();
	if (type == PLUS)
	{
		CVariable res2;
		visit(node->left(), res);
		visit(node->right(),res2);
		res = res + res2;
	}
	else if (type == MINUS)
	{
		CVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res - res2;
	}
	else if (type == MUL)
	{
		CVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res * res2;
	}
	else if (type == DIV)
	{
		CVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res / res2;
	}
	else if (type == MOD)
	{
		CVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res % res2;
	}
	else if (type == EQUAL)
	{
		CVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res == res2;
	}
	else if (type == GREATER)
	{
		CVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res > res2;
	}
	else if (type == LESS)
	{
		CVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res < res2;
	}
	else if (type == GREATER_EQUAL)
	{
		CVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res >= res2;
	}
	else if (type == LESS_EQUAL)
	{
		CVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res <= res2;
	}
	else if (type == NOT_EQUAL)
	{
		CVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res != res2;
	}

	else if (type == AND)
	{
		CVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res && res2;
	}
	else if (type == OR)
	{
		CVariable res2;
		visit(node->left(), res);
		visit(node->right(), res2);
		res = res || res2;
	}
	else if (type == DOT)
	{
		visit_Member(node->left(), node->right(),res);
	}
	else if (type == LSQUARE)
	{
		visit_Index(node->left(), node->right(),res);
	}
	else if (type == NOT)
	{
		visit_Not(node->right(),res);
	}
	else
		res.reset();

}

void CInterpreter::visit_Num(Num* node, CVariable& res)
{
	if (node->num_type() == Num::INT)
	{
		res.setInt(node->value());
	}
	else if (node->num_type() == Num::FLOAT)
	{
		res.setDouble(node->float_value());
	}
}
void CInterpreter::visit_Bool(Bool* node, CVariable& res)
{
	int n = 0;
	if (node->value())
		n = 1;
	res.setInt(n);
}
void CInterpreter::visit_Str(Str* node, CVariable& res)
{
	res.setType(CVariable::STRING);
	res.strValue() = node->value();
}
void CInterpreter::visit_Array(Array* node, CVariable& arr)
{
	arr.setArray(0);
	if (arr.arrValue() == NULL)
		return arr.reset();

	CVariable result;
	vector<AST*>& inits = node->inits();
	for (size_t i = 0; i < inits.size(); i++)
	{
		visit(inits[i],result);
		(*arr.arrValue()).push_back(result);
	}
}
void CInterpreter::visit_Dict(Dict* node, CVariable& dict)
{
	string var_name = node->token().value();
	dict.setDict();
	if (dict.dictValue() == NULL)
		return dict.reset();

	vector<AST*>& inits1 = node->inits_left();
	vector<AST*>& inits2 = node->inits_right();
	if (inits1.size() != inits2.size())
		error(var_name + ":dict pairs error\r\n",node->token());

	CVariable left;
	CVariable right;
	for (size_t i = 0; i < inits1.size(); i++)
	{
		visit(inits1[i],left);
		visit(inits2[i],right);
		(*dict.dictValue())[left]=right;
	}
}
void CInterpreter::set_index_value(const string& var_name, CVariable& var, CVariable& idx_value, const CVariable& result, const CToken& token)
{
	CVariable index;
	CVariable* pVar_value = &idx_value;
	for (size_t i = 1; i < var.index().size() - 1; i++)
	{
		index = var.index()[i];
		if (pVar_value->type() == CVariable::DICT || pVar_value->type() == CVariable::CLASS)
		{
			if (pVar_value->dictValue()->find(index) != pVar_value->dictValue()->end())
				pVar_value = &(*pVar_value->dictValue())[index];
			else
			{
				if (i < var.index().size() - 2)
				{
					error("dict index error for " + var_name + "\r\n",token);
					break;
				}
			}
		}
		else if (pVar_value->type() == CVariable::ARRAY)
		{
			_INT idx = -1;
			if (index.type() == CVariable::INT)
				idx = index.intValue();
			if (idx >= 0 && (size_t)idx < (*pVar_value->arrValue()).size())
				pVar_value = &(*pVar_value->arrValue())[idx];
			else
			{
				if (i < var.index().size() - 2)
				{
					error("array index error for " + var_name + "\r\n",token);
					break;
				}
			}
		}

	}
	if (pVar_value->type() == CVariable::DICT || pVar_value->type() == CVariable::CLASS)
	{
		index = var.index()[var.index().size() - 1];
		if (index.type()!=CVariable::EMPTY)
			(*pVar_value->dictValue())[index] = result;
	}
	else if (pVar_value->type() == CVariable::ARRAY)
	{
		_INT idx = -1;
		index = var.index()[var.index().size() - 1];
		if (index.type() == CVariable::INT)
			idx = index.intValue();
		if (idx >= 0 && (size_t)idx < (*pVar_value->arrValue()).size())
			(*pVar_value->arrValue())[index.intValue()] = result;
	}
	else if (pVar_value->type() == CVariable::STRING)
	{
		_INT idx = -1;
		index = var.index()[var.index().size() - 1];
		if (index.type() == CVariable::INT)
			idx = index.intValue();
		if (idx >= 0 && (size_t)idx < pVar_value->strValue().size())
			pVar_value->strValue()[index.intValue()] = result.strValue()[0];
	}
	else
		error("index error for " + var_name + "\r\n",token);

}

void CInterpreter::get_index_value(CVariable& var, CVariable& idx, CVariable& var_value)
{
	if (var.type() == CVariable::VARTYPE::DICT || var.type() == CVariable::VARTYPE::CLASS)
	{
		CVariable dict_index = idx;
		if (dict_index.type() != CVariable::EMPTY)
		{
			map<CVariable, CVariable>* dict = var.dictValue();
			if (dict && dict->find(dict_index) != dict->end())
				var_value = (*dict)[dict_index];
		}
	}
	else
	{
		_INT arr_index = -1;
		CVariable index = idx;
		if (index.type() == CVariable::INT)
			arr_index = index.intValue();

		if (var.type() == CVariable::STRING)
		{
			const string& strValue = var.strValue();
			if (arr_index >= 0 && (size_t)arr_index < strValue.size())
			{
				var_value = CVariable(string(1, strValue[arr_index]));
			}
		}
		else if (var.type() == CVariable::ARRAY)
		{
			vector<CVariable>* arr = var.arrValue();
			if (arr && arr_index >= 0 && (size_t)arr_index < arr->size())
				var_value = (*arr)[arr_index];
		}

	}

	var_value.setInfo(var.info());
	var_value.setIndex(var.index());
	var_value.index().push_back(idx);

}

void CInterpreter::visit_Var(Var* node, CVariable& var_value)
{
	ActivationRecord* ar = NULL;
	if (node->global())
		ar = &m_callstack.base();
	else
		ar = &m_callstack.peek();

	string& var_name = node->value();
	if (node->global())
		var_value = ar->get__global_value(var_name);
	else
		var_value = ar->get_value(var_name);
	if (var_value.type()==CVariable::EMPTY)
		error(var_name + " is undefined.\r\n", node->token());

	var_value.info()["name"] = var_name;
	if (node->global())
		var_value.info()["global"] = "1";
	else
		var_value.info()["global"] = "0";


	if (node->is_func())
	{
		if (var_value.type() == CVariable::POINTER)
		{
			FunctionStatement* fun = (FunctionStatement*)var_value.pointerValue();
			if (var_value.info().find("func_type") != var_value.info().end())
			{
				if (var_value.info()["func_type"] == "func")
				{
					return exec_function(fun, node->exprs(), node->token(), var_value);
				}
				if (var_value.info()["func_type"] == "class")
				{
					ClassStatement* cls = (ClassStatement*)fun;
					return exec_class(cls, node->exprs(), node->token(), var_value);
				}
				if (var_value.info()["func_type"] == "member")
				{
					return exec_function(fun, node->exprs(), node->token(), var_value);
				}
			}
		}
		else
			error(var_name + " is not a function\r\n", node->token());
	}

}
void CInterpreter::visit_Assign(Assign* node, CVariable& res)
{
	if (node->left()->type()!=AST::VAR)//dict, array, class
	{
		CVariable var_value;
		visit(node->left(),var_value);
		string var_name;
		bool bGlobal = false;
		if (var_value.info().find("name") != var_value.info().end())
			var_name = var_value.info()["name"];
		if (var_value.info().find("global") != var_value.info().end())
		{
			if (var_value.info()["global"] == "1")
				bGlobal = true;
		}

		CVariable &result=res;
		visit(node->right(),result);
		result.index().clear();

		ActivationRecord* ar = NULL;
		if (bGlobal)
			ar = &m_callstack.base();
		else
			ar = &m_callstack.peek();
		if (var_value.index().size()>0)
		{
			if (bGlobal)
			{
				CVariable index = var_value.index()[0];
				CVariable::VARTYPE vartype = ar->get_global_vartype(var_name);
				if (vartype == CVariable::VARTYPE::DICT || vartype == CVariable::VARTYPE::CLASS)
				{
					CVariable& global_value = ar->get__global_value(var_name);
					if (global_value.dictValue() == NULL)
						error("global dict error for " + var_name + "\r\n", node->token());
					if (var_value.index().size() == 1)
						(*global_value.dictValue())[index] = result;
					else
					{
						CVariable& idx_value=(*global_value.dictValue())[index];
						set_index_value(var_name, var_value, idx_value, result, node->token());
					}
				}
				else
				{
					_INT idx = -1;
					if (index.type() == CVariable::INT)
						idx = index.intValue();
					else
						error("array index error for " + var_name + "\r\n", node->token());

					CVariable& global_value = ar->get__global_value(var_name);
					if (global_value.arrValue() == NULL)
						error("global array error for " + var_name + "\r\n", node->token());

					if (var_value.index().size() == 1)
						(*global_value.arrValue())[idx] = result;
					else
					{
						CVariable& idx_value = (*global_value.arrValue())[idx];
						set_index_value(var_name, var_value, idx_value, result, node->token());
					}
				}

			}
			else
			{
				CVariable index = var_value.index()[0];
				CVariable::VARTYPE vartype = ar->get_vartype(var_name);
				if (vartype == CVariable::VARTYPE::DICT || vartype == CVariable::VARTYPE::CLASS)
				{
					if (var_value.index().size() == 1)
						ar->set_dict_value(var_name, result, index);
					else
					{
						CVariable& idx_value = ar->get_dict_value(var_name, index);
						set_index_value(var_name, var_value, idx_value, result, node->token());
					}
				}
				else
				{
					_INT idx = -1;
					if (index.type() == CVariable::INT)
						idx = index.intValue();
					else
						error("array index error for " + var_name + "\r\n", node->token());

					if (var_value.index().size() == 1)
						ar->set_array_value(var_name, result, idx);
					else
					{
						CVariable& idx_value = ar->get_array_value(var_name, idx);
						set_index_value(var_name, var_value, idx_value, result, node->token());
					}
				}
			}
		}
		else
			error("error assign \r\n", node->token());

	}
	else
	{
		Var* var = (Var*)node->left();
		string& var_name = var->value();
		if (var->exprs().size() > 0)
		{
			error("function " + var_name+" cannot be assigned\r\n", node->token());
			return res.reset();
		}

		CVariable &result=res;
		visit(node->right(),result);
		result.index().clear();

		ActivationRecord* ar = NULL;
		if (var->global())
			ar = &m_callstack.base();
		else
			ar = &m_callstack.peek();

		if (var->global())
			ar->set_global_value(var_name, result);
		else
			ar->set_value(var_name, result);
	}

	return res.reset();
}
void CInterpreter::visit_Program(Program* node, CVariable& res)
{
	ActivationRecord ar(node->name(),"program",1);
	ar.create_global(m_globalValue, m_vecArgv, m_argvName);

	m_callstack.push(ar);
	for (size_t i = 0; i<node->globals().size(); i++)
	{
		if(node->globals()[i]->type()==AST::ASSIGN)
			visit(node->globals()[i],res);
	}
	visit(node->block(),res);
	m_callstack.pop();
}
void CInterpreter::visit_Block(Block* node, CVariable& res)
{
	visit(node->compound(),res);
}
void CInterpreter::visit_Compound(Compound* node, CVariable& result)
{
	for (size_t i = 0; i < node->child().size(); i++)
	{
		visit(node->child()[i],result);
		if (result.tag() == CVariable::RETURN)
			return;
		if (result.tag() == CVariable::BREAK || result.tag() == CVariable::CONTINUE)
			error("break/continue is not in loop", node->child()[i]->token());
	}
}

bool CInterpreter::check_condition(CVariable& condition)
{
	if (condition.type() == CVariable::INT)
	{
		if (condition.intValue() != 0)
			return true;
		else
			return false;
	}
	if (condition.type() == CVariable::FLOAT)
	{
		if (condition.floatValue() != 0)
			return true;
		else
			return false;
	}
	if (condition.type() == CVariable::STRING)
	{
		if (condition.strValue().size() != 0)
			return true;
		else
			return false;
	}
	if (condition.type() == CVariable::POINTER)
	{
		if (condition.pointerValue() != 0)
			return true;
		else
			return false;
	}
	if (condition.type() == CVariable::ARRAY)
	{
		if (condition.arrValue()->size() != 0)
			return true;
		else
			return false;
	}
	if (condition.type() == CVariable::DICT)
	{
		if (condition.dictValue()->size() != 0)
			return true;
		else
			return false;
	}
	if (condition.type() == CVariable::CLASS)
			return true;

	return false;
}
void CInterpreter::visit_IfCompound(IfCompound* node, CVariable& result)
{
	visit(node->expr(), result);
	bool bTrue = check_condition(result);
	if (bTrue)
	{
		for (size_t i = 0; i < node->true_statements().size(); i++)
		{
			visit(node->true_statements()[i],result);
			if (result.tag() == CVariable::BREAK || result.tag() == CVariable::RETURN || result.tag() == CVariable::CONTINUE)
				return;
		}
	}
	else
	{
		for (size_t i = 0; i < node->false_statements().size(); i++)
		{
			visit(node->false_statements()[i],result);
			if (result.tag() == CVariable::BREAK || result.tag() == CVariable::RETURN || result.tag() == CVariable::CONTINUE)
				return;
		}
	}

	return result.reset();//
}
void CInterpreter::visit_ForCompound(ForCompound* node, CVariable& result)
{
	for (size_t i = 0; i < node->init_statements().size(); i++)
		visit(node->init_statements()[i], result);

	visit(node->expr(), result);
	bool bTrue = check_condition(result);
	while (bTrue)
	{
		for (size_t i = 0; i < node->statements().size(); i++)
		{
			visit(node->statements()[i],result);
			if (result.tag() == CVariable::BREAK)
				return result.reset();
			if (result.tag() == CVariable::RETURN)
				return;
			if (result.tag() == CVariable::CONTINUE)
				break;
		}

		for (size_t i = 0; i < node->update_statements().size(); i++)
			visit(node->update_statements()[i], result);
		visit(node->expr(), result);
		bTrue = check_condition(result);
	}

	return result.reset();
}
void CInterpreter::visit_WhileCompound(WhileCompound* node, CVariable& result)
{
	visit(node->expr(), result);
	bool bTrue = check_condition(result);
	while (bTrue)
	{
		for (size_t i = 0; i < node->statements().size(); i++)
		{
			visit(node->statements()[i],result);
			if (result.tag() == CVariable::BREAK)
				return result.reset();
			if (result.tag() == CVariable::RETURN)
				return;
			if (result.tag() == CVariable::CONTINUE)
				break;
		}

		visit(node->expr(), result);
		bTrue = check_condition(result);
	}

	return result.reset();
}
void CInterpreter::visit_Break(BreakStatement* node, CVariable& res)
{
	res.setTag(CVariable::BREAK);
}
void CInterpreter::visit_Continue(ContinueStatement* node, CVariable& res)
{
	res.setTag(CVariable::CONTINUE);
}
void CInterpreter::visit_Return(ReturnStatement* node, CVariable& res)
{
	visit(node->expr(), res);
	res.setTag(CVariable::RETURN);
}
void CInterpreter::visit_Include(IncludeStatement* node, CVariable& res)
{
	for (size_t i = 0; i<node->globals().size(); i++)
	{
		if (node->globals()[i]->type() == AST::ASSIGN)
			visit(node->globals()[i],res);
	}
	return res.reset();
}
void CInterpreter::visit_Builtin(BuiltinStatement* node, CVariable& res)
{
	if (node->token().value() == "print")
	{
		vector<AST*>& exprs = node->exprs();
		for (size_t i = 0; i < exprs.size(); i++)
		{
			CVariable &result=res;
			visit(exprs[i], result);
			if (result.type() == CVariable::INT)
				re_printf(_INTFMT, result.intValue());
			else if (result.type() == CVariable::FLOAT)
				re_printf("%f ", result.floatValue());
			else if (result.type() == CVariable::STRING)
			{
				if (m_pFileOut)
					fprintf(m_pFileOut, "%s ", result.strValue().c_str());
				else
					printf("%s ", result.strValue().c_str());
			}
			else if (result.type() == CVariable::POINTER)
				re_printf("%p ", result.pointerValue());
			else if (result.type() == CVariable::ARRAY)
			{
				print_object(result);
			}
			else if (result.type() == CVariable::DICT)
			{
				print_object(result);
			}
			else if (result.type() == CVariable::CLASS)
			{
				re_printf("<class %p> ", result.dictValue());
			}
			else if (result.type() == CVariable::EMPTY)
			{
				re_printf("<undefined> ");
			}
			else if (result.type() == CVariable::NONE)
			{
				re_printf("<null> ");
			}
		}
		if (exprs.size()>0)
			re_printf("\n");
	}
	else if (node->token().value() == "_loadlib")
	{
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() == 1)
		{
			CVariable filename;
			visit(exprs[0], filename);
			if (filename.type() == CVariable::STRING)
			{
				void* handle = m_dll.loadlib(filename.strValue());
				CVariable &result=res;
				result.setType(CVariable::POINTER);
				result.setPointer(handle);
				return;
			}
		}
		return res.reset();
	}
	else if (node->token().value() == "_freelib")
	{
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() == 1)
		{
			CVariable handle;
			visit(exprs[0], handle);
			if (handle.type() == CVariable::POINTER)
			{
				m_dll.freelib(handle.pointerValue());
			}
		}
		return res.reset();
	}
	else if (node->token().value() == "_calllib")
	{
		vector<CVariable> params;
		vector<AST*>& exprs = node->exprs();
		for (size_t i = 0; i < exprs.size(); i++)
		{
			CVariable result;
			visit(exprs[i], result);
			params.push_back(result);
		}
		CVariable &result=res;
		result.setType(CVariable::NONE);
		m_dll.calllib(params, result);
		return;
	}
	else if (node->token().value() == "_createthread")
	{
		CVariable &result=res;
		CVariable param0;
		vector<AST*>& exprs = node->exprs();
		if (exprs.size()>0)
			visit(exprs[0],param0);

		AST* save = NULL;
		if (param0.type() == CVariable::POINTER)
		{
			save = (AST*)param0.pointerValue();
			if (param0.info().find("func_type") != param0.info().end())
			{
				if (param0.info()["func_type"] != "func")
				{
					error("error function type for thread.", node->token());
					return;
				}
			}
		}
		if (save == NULL)
		{
			error("not found thread function name.", node->token());
			return;
		}

		FunctionExec* newnode = new FunctionExec(save, CToken());
		for (size_t i = 1; i < exprs.size(); i++)
			((FunctionExec*)newnode)->exprs().push_back(exprs[i]);

		threadParams *pThreadparams = new threadParams;
		pThreadparams->func = newnode;
		pThreadparams->globaldata = m_parser.global_data();
		pThreadparams->callstack = this->get_callstack();

#ifdef _WIN32
		result = CVariable(1);
		if (_beginthread(newthread, 0, pThreadparams) == -1)
		{
			//printf("create thread error");
			delete newnode;
			delete pThreadparams;
			result = CVariable(0);
		}
#else
		result = CVariable(1);
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_t tid;
		if (pthread_create(&tid, &attr, newthread, pThreadparams))
		{
			//printf("create thread error");
			delete newnode;
			delete pThreadparams;
			result = CVariable(0);
		}
#endif
		return;
	}
	else if (node->token().value() == "_createlock")
	{
		void* handle = m_parser.global_data()->lock().create_lock();
		CVariable &result=res;
		result.setType(CVariable::POINTER);
		result.setPointer(handle);
		return;
	}
	else if (node->token().value() == "_lock")
	{
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() != 1)
			error("lock() takes only 1 argument.\r\n", node->token());
		else
		{
			CVariable result;
			visit(exprs[0], result);
			if (result.type() == CVariable::POINTER)
				m_parser.global_data()->lock().lock(result.pointerValue());
		}
	}
	else if (node->token().value() == "_unlock")
	{
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() != 1)
			error("unlock() takes only 1 argument.\r\n", node->token());
		else
		{
			CVariable result;
			visit(exprs[0], result);
			if (result.type() == CVariable::POINTER)
				m_parser.global_data()->lock().unlock(result.pointerValue());
		}
	}
	else if (node->token().value() == "_len")
	{
		vector<CVariable> results;
		vector<AST*>& exprs = node->exprs();
		if (exprs.size()!=1)
			error("len() takes only 1 argument.\r\n", node->token());
		else
		{
			CVariable result;
			visit(exprs[0], result);
			if (result.type() == CVariable::ARRAY)
			{
				if (result.arrValue() == NULL)
				{
					res = CVariable(0);
					return;
				}
				_INT size = result.arrValue()->size();
				res = CVariable(size);
				return;
			}
			else if (result.type() == CVariable::DICT)
			{
				if (result.dictValue() == NULL)
				{
					res = CVariable(0);
					return;
				}
				_INT size = result.dictValue()->size();
				res = CVariable(size);
				return;
			}
			else if (result.type() == CVariable::STRING)
			{
				_INT size = result.strValue().size();
				res = CVariable(size);
				return;
			}
			else
				error("object has no _len()\r\n", node->token());
		}

	}
	else if (node->token().value() == "_str")
	{
		vector<CVariable> results;
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() != 1)
			error("str() takes only 1 argument.\r\n", node->token());
		else
		{
			CVariable result;
			visit(exprs[0], result);
			if (result.type() == CVariable::INT)
			{
				_INT value = result.intValue();
				ostringstream   os;
				os << value;
				res = CVariable(os.str());
				return;
			}
			else if (result.type() == CVariable::FLOAT)
			{
				double value = result.floatValue();
				ostringstream   os;
				os << value;
				res = CVariable(os.str());
				return;
			}
			else if (result.type() == CVariable::POINTER)
			{
				void* value = result.pointerValue();
				ostringstream   os;
				os << value;
				res = CVariable(os.str());
				return;
			}
			else if (result.type() == CVariable::STRING)
			{
				res = result;
				return;
			}
			else
				error("object has no _str()\r\n", node->token());
		}

	}
	else if (node->token().value() == "_int")
	{
		vector<CVariable> results;
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() != 1)
			error("int() takes only 1 argument.\r\n", node->token());
		else
		{
			CVariable result;
			visit(exprs[0], result);
			if (result.type() == CVariable::STRING)
			{
				string value = result.strValue();
				res = CVariable(_ATOI(value.c_str()));
				return;
			}
			else if (result.type() == CVariable::FLOAT)
			{
				double value = result.floatValue();
				ostringstream   os;
				os << value;
				res = CVariable(_ATOI(os.str().c_str()));
				return;
			}
			else if (result.type() == CVariable::INT)
			{
				res = result;
				return;
			}
			else
				error("object has no _int()\r\n", node->token());
		}

	}
	else if (node->token().value() == "_float")
	{
		vector<CVariable> results;
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() != 1)
			error("float() takes only 1 argument.\r\n", node->token());
		else
		{
			CVariable result;
			visit(exprs[0], result);
			if (result.type() == CVariable::STRING)
			{
				string value = result.strValue();
				res = CVariable().setDouble(atof(value.c_str()));
				return;
			}
			else if (result.type() == CVariable::INT)
			{
				_INT value = result.intValue();
				ostringstream   os;
				os << value;
				res = CVariable().setDouble(atof(os.str().c_str()));
				return;
			}
			else if (result.type() == CVariable::FLOAT)
			{
				res = result;
				return;
			}
			else
				error("object has no _float()\r\n", node->token());
		}

	}
	else if (node->token().value() == "_type")
	{
		vector<CVariable> results;
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() != 1)
			error("type() takes only 1 argument.\r\n", node->token());
		else
		{
			CVariable result;
			visit(exprs[0], result);
			if (result.type() == CVariable::INT)
				res = CVariable("INT");
			else if (result.type() == CVariable::FLOAT)
				res = CVariable("FLOAT");
			else if (result.type() == CVariable::STRING)
				res = CVariable("STRING");
			else if (result.type() == CVariable::ARRAY)
				res = CVariable("ARRAY");
			else if (result.type() == CVariable::DICT)
				res = CVariable("DICT");
			else if (result.type() == CVariable::CLASS)
				res = CVariable("CLASS");
			else if (result.type() == CVariable::POINTER)
				res = CVariable("HANDLE");
			else if (result.type() == CVariable::NONE)
				res = CVariable("NULL");
			else if (result.type() == CVariable::EMPTY)
				res = CVariable("UNDEFINED");
			else
				res = CVariable("UNKNOWN");
			return;
		}

	}
	else if (node->token().value() == "sprintf")
	{
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() >= 1)
		{
			CVariable fmt;
			visit(exprs[0], fmt);
			if (fmt.type() == CVariable::STRING)
			{
				vector<CVariable> vecArgs;
				for (size_t i = 1; i < exprs.size(); i++)
				{
					CVariable result;
					visit(exprs[i], result);
					vecArgs.push_back(result);
				}
				string format = fmt.strValue();
				string err=Tool::mysprintf(format, vecArgs);
				if (err.size()>0)
					error(err, node->token());
				else
				{
					res = CVariable(format);
					return;
				}
			}

		}
		else
			error("sprintf() arguments number error\r\n", node->token());

		res = CVariable("");//empty str
		return;
	}
	else if (node->token().value() == "printf")
	{
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() >= 1)
		{
			CVariable fmt;
			visit(exprs[0], fmt);
			if (fmt.type() == CVariable::STRING)
			{
				vector<CVariable> vecArgs;
				for (size_t i = 1; i < exprs.size(); i++)
				{
					CVariable result;
					visit(exprs[i], result);
					vecArgs.push_back(result);
				}
				string format = fmt.strValue();
				string err;
				if (vecArgs.size()>0)
					err = Tool::mysprintf(format, vecArgs);
				if (err.size()>0)
					error(err, node->token());
				else
				{
					if (m_pFileOut)
						fprintf(m_pFileOut, "%s", format.c_str());
					else
						printf("%s", format.c_str());
				}
			}
			else
				error("printf() argument type error\r\n", node->token());

		}
		else
			error("printf() arguments number error\r\n", node->token());

	}
	else if (node->token().value() == "_getargv")
	{
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() >= 0)
		{
			string argvName = ARGV_ARRAY_NAME;
			if (exprs.size() == 1)
			{
				CVariable argValue;
				visit(exprs[0], argValue);
				argvName = argValue.strValue();
			}

			ActivationRecord* ar = NULL;
			ar = &m_callstack.base();
			res = ar->get__global_value(argvName);
			return;
		}
	}
	else if (node->token().value() == "_input")
	{
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() == 0)
		{
			char buff[1024];
			buff[0] = 0;
			fgets(buff, 1024, stdin);
			res = CVariable(buff);
			return;
		}
	}
	else if (node->token().value() == "_copy")
	{
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() == 1)
		{
			CVariable old;
			visit(exprs[0], old);
			copy_object(old, res);
			return;
		}
	}
	else if (node->token().value() == "_fun")
	{
		CVariable ret;
		ret.setType(CVariable::NONE);
		vector<AST*>& exprs = node->exprs();
		if (exprs.size() >0)
		{
			vector<CVariable> args;
			for (size_t i = 0; i < exprs.size(); i++)
			{
				CVariable result;
				visit(exprs[i], result);
				args.push_back(result);
			}
			Func::call_func(args, ret);
		}
		res = ret;
		return;
	}


	return res.reset();
}
void CInterpreter::visit_Class(FunctionExec* node, CVariable& res)
{
	ClassStatement* cls = (ClassStatement*)node->fun_statement();
	exec_class(cls, node->exprs(), node->token(),res);
}
void CInterpreter::visit_Function(FunctionStatement* node, CVariable& res)
{
	string::size_type pos = node->name().rfind(".");
	if ( pos != string::npos)//class function
	{
		string fun_name = node->name().substr(pos + 1);
		ActivationRecord& ar = m_callstack.peek();

		CVariable& self = ar.get_value("self");
		CVariable &fun=res;
		fun.setType(CVariable::POINTER);
		fun.setPointer(node);
		fun.info()["func_type"] = "member";
		(*self.dictValue())[fun_name] = fun;
	}
	return res.reset();
}

void CInterpreter::visit_FunctionExec(FunctionExec* node, CVariable& res)
{ 
	if (node->is_var())
	{
		CVariable &var=res;
		var.setType(CVariable::POINTER);
		var.setPointer(node->fun_statement());
		var.info()["func_type"] = "func";
		if (node->fun_statement()->type() == AST::CLASS)
			var.info()["func_type"] = "class";

		return;
	}

	if (node->fun_statement()->type() == AST::CLASS)
		return visit_Class(node,res);

	FunctionStatement* fun = (FunctionStatement*)node->fun_statement();
	return exec_function(fun, node->exprs(), node->token(),res);
}

void CInterpreter::exec_function(FunctionStatement* fun, vector<AST*>& exprs, CToken& token, CVariable& res)
{
	if (fun->type() == AST::EMPTY)
		return res.reset();

	CVariable &result = res;
	vector<CVariable> params_pass;
	for (size_t i = 0; i < exprs.size(); i++)
	{
		visit(exprs[i], result);
		if (result.type() == CVariable::EMPTY)
		{
			error("argument value error for " + fun->name() + "\r\n", token);
			return res.reset();
		}
		result.index().clear();
		params_pass.push_back(result);
	}

	vector<string>& params = fun->params();
	if (params.size() < params_pass.size())
	{
		error("argument number error for " + fun->name() + "\r\n", token);
		return res.reset();
	}
	vector<AST*>& params_value = fun->params_value();

	ActivationRecord new_ar(fun->name(), "function", 2);
	for (size_t i = 0; i < params.size(); i++)
	{
		if (i >= 0 && i<params_pass.size())
			new_ar.set_value(params[i], params_pass[i]);
		else
		{
			visit(params_value[i], result);
			if (result.type() == CVariable::EMPTY)
			{
				error("no default argument or default argument value error for " + fun->name() + "\r\n", token);
				return res.reset();
			}
			result.index().clear();
			new_ar.set_value(params[i], result);
		}
	}

	m_callstack.push(new_ar);
	for (size_t i = 0; i < fun->statements().size(); i++)
	{
		visit(fun->statements()[i],result);
		if (result.tag() == CVariable::RETURN)
		{
			m_callstack.pop();
			result.setTag(CVariable::NORMAL);
			return;
		}
		if (result.tag() == CVariable::BREAK || result.tag() == CVariable::CONTINUE)
		{
			error("break/continue is not in loop", fun->statements()[i]->token());
			break;
		}
	}
	m_callstack.pop();

	return res.reset();
}

void CInterpreter::exec_class(ClassStatement* cls, vector<AST*>& exprs, CToken& token, CVariable& res)
{
	CVariable &result=res;
	vector<CVariable> params_pass;
	for (size_t i = 0; i < exprs.size(); i++)
	{
		visit(exprs[i], result);
		if (result.type() == CVariable::EMPTY)
		{
			error("argument value error for " + cls->name() + "\r\n", token);
			return res.reset();
		}
		result.index().clear();
		params_pass.push_back(result);
	}

	vector<string>& params = cls->params();
	if (params.size() < params_pass.size())
	{
		error("argument number error for " + cls->name() + "\r\n", token);
		return res.reset();
	}
	vector<AST*>& params_value = cls->params_value();

	ActivationRecord new_ar(cls->name(), "class", 2);
	for (size_t i = 0; i < params.size(); i++)
	{
		if (i >= 0 && i<params_pass.size())
			new_ar.set_value(params[i], params_pass[i]);
		else
		{
			visit(params_value[i], result);
			if (result.type() == CVariable::EMPTY)
			{
				error("no default argument or default argument value error for " + cls->name() + "\r\n", token);
				return res.reset();
			}
			result.index().clear();
			new_ar.set_value(params[i], result);
		}
	}
	CVariable self;
	self.setDict();
	self.setType(CVariable::CLASS);
	new_ar.set_value("self", self);

	m_callstack.push(new_ar);
	for (size_t i = 0; i < cls->statements().size(); i++)
	{
		visit(cls->statements()[i],result);
		if (result.tag() == CVariable::RETURN)
		{
			//m_callstack.pop();
			result.setTag(CVariable::NORMAL);
			//return result;
			if (result.type() != CVariable::NONE && result.type() != CVariable::EMPTY)
				error("class " + cls->name()+" cannot return non-null type \r\n", cls->token());
			break;
		}
		if (result.tag() == CVariable::BREAK || result.tag() == CVariable::CONTINUE)
		{
			error("break/continue is not in loop", cls->statements()[i]->token());
			break;
		}
	}
	new_ar = m_callstack.peek();
	m_callstack.pop();

	res = new_ar.get_value("self");
}

CVariable CInterpreter::interpret()
{
	AST* tree=m_parser.parse();
	if (m_parser.error_msg().size() > 0)
		return m_parser.error_msg();
	CVariable result;
	visit(tree, result);
	if (error_msg().size() > 0)
		return CVariable(error_msg());
	return result;
}
void CInterpreter::re_printf(const char* format, ...)
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

void CInterpreter::visit_Index(AST* obj, AST* idx, CVariable& res)
{
	CVariable var;
	visit(obj, var);
	CVariable index;
	visit(idx, index);
	get_index_value(var, index,res);
}

void CInterpreter::visit_Member(AST* obj, AST* member, CVariable& res)
{
	CVariable &ret=res;
	ret.setType(CVariable::NONE);

	CVariable var;
	visit(obj, var);
	
	Member* memb = (Member*)member;
	vector<AST*>& exprs = memb->exprs();
	vector<CVariable> args;
	for (size_t i = 0; i < memb->exprs().size(); i++)
	{
		CVariable result;
		visit(memb->exprs()[i], result);
		args.push_back(result);
	}

	if (var.type() == CVariable::VARTYPE::STRING)
	{
		if (!m_libString.call_member(memb->token().value(), var, args, ret))
			error(m_libString.err_msg(), memb->token());
	}
	else if (var.type() == CVariable::VARTYPE::ARRAY)
	{
		if (!m_libArray.call_member(memb->token().value(), var, args, ret))
			error(m_libArray.err_msg(), memb->token());
	}
	else if (var.type() == CVariable::VARTYPE::DICT)
	{
		if (!m_libDict.call_member(memb->token().value(), var, args, ret))
			error(m_libDict.err_msg(), memb->token());
	}
	else if (var.type() == CVariable::VARTYPE::CLASS)
	{
		if (m_libClass.call_member(memb->token().value(), var, args, ret))
			return;
		if (m_libClass.err_msg().size() > 0)
		{
			error(m_libClass.err_msg(), memb->token());
			return;
		}

		if (exprs.size() == 0)//member
		{
			CVariable idx = CVariable(memb->token().value());
			CVariable var_value;
			get_index_value(var, idx, var_value);
			bool bSelf = false;
			if (var.info().find("name") != var.info().end() && var.index().size() == 0)
			{
				if (var.info()["name"] == "self")
					bSelf = true;
			}
			if (!bSelf)
			{
				if (idx.strValue().size() > 2 && idx.strValue().substr(0, 2) == "__")//private member
				{
					error("cannot access private member [" + idx.strValue() + "]\r\n", memb->token());
					return;
				}
			}
			if (var_value.type() != CVariable::EMPTY || bSelf)
			{
				res = var_value;
				return;
			}
			else
			{
				error("not found member [" + idx.strValue() + "]\r\n", memb->token());
				return;
			}
		}
		else//function
		{
			AST* save = NULL;
			if (var.dictValue()->find(memb->token().value()) != var.dictValue()->end())
				save = (AST*)(*var.dictValue())[memb->token().value()].pointerValue();
			if (save == NULL)
			{
				error("not found class function name [" + memb->token().value() + "]\r\n", memb->token());
				return;
			}

			bool bSelf = false;
			if (var.info().find("name") != var.info().end() && var.index().size() == 0)
			{
				if (var.info()["name"] == "self")
					bSelf = true;
			}
			if (!bSelf)
			{
				if (memb->token().value().size() > 2 && memb->token().value().substr(0, 2) == "__")//private function
				{
					error("cannot access private function [" + memb->token().value() + "]\r\n", memb->token());
					return;
				}
			}

			FunctionStatement* fun = (FunctionStatement*)save;
			CVariable& fun_value = (*var.dictValue())[memb->token().value()];
			if (fun_value.info().find("func_type") != fun_value.info().end())
			{
				if (fun_value.info()["func_type"] == "func")
					return exec_function(fun, exprs, memb->token(),res);
				if (fun_value.info()["func_type"] == "class")
				{
					ClassStatement* cls = (ClassStatement*)fun;
					return exec_class(cls, exprs, memb->token(),res);
				}
			}

			vector<string>& params = fun->params();
			size_t paramCount = args.size();
			if (args.size() == 1 && args[0].type() == CVariable::EMPTY)
				paramCount = 0;
			if (params.size() < paramCount + 1)
			{
				error("argument number error for [" + fun->name() + "]\r\n", memb->token());
				return;
			}

			vector<AST*>& params_value = fun->params_value();

			ActivationRecord new_ar(fun->name(), "function", 2);
			var.index().clear();
			new_ar.set_value(params[0], var);//self
			for (size_t i = 0; i < params.size()-1; i++)
			{
				if (i >= 0 && i < args.size() && (args.size()>0 && args[0].type()!=CVariable::EMPTY))
				{
					args[i].index().clear();
					new_ar.set_value(params[i + 1], args[i]);
				}
				else
				{
					CVariable result;
					visit(params_value[i], result);
					if (result.type() == CVariable::EMPTY)
					{
						error("no default argument or default argument value error for " + fun->name() + "\r\n", memb->token());
						return;
					}
					result.index().clear();
					new_ar.set_value(params[i + 1], result);
				}

			}

			CVariable result;
			m_callstack.push(new_ar);
			for (size_t i = 0; i < fun->statements().size(); i++)
			{
				visit(fun->statements()[i],result);
				if (result.tag() == CVariable::RETURN)
				{
					m_callstack.pop();
					result.setTag(CVariable::NORMAL);
					res = result;
					return;
				}
				if (result.tag() == CVariable::BREAK || result.tag() == CVariable::CONTINUE)
				{
					error("break/continue is not in loop", fun->statements()[i]->token());
					break;
				}
			}
			m_callstack.pop();

		}
	}
	else
	{
		char buff[512];
		sprintf_s(buff, "Object [%s] not found function name [%s]\r\n", obj->token().value().c_str(), memb->token().value().c_str());
		error(buff, memb->token());
	}

}

void CInterpreter::visit_Not(AST* node, CVariable& res)
{
	int ret = 0;
	CVariable value;
	visit(node, value);
	if (value.type() == CVariable::INT)
	{
		if (!value.intValue())
			ret = 1;
	}
	else if (value.type() == CVariable::FLOAT)
	{
		if (!value.floatValue())
			ret = 1;
	}
	else if (value.type() == CVariable::NONE || value.type() == CVariable::EMPTY)
	{
		ret = 1;
	}
	else if (value.type() == CVariable::STRING)
	{
		if (value.strValue().size() == 0)
			ret = 1;
	}
	else if (value.type() == CVariable::ARRAY)
	{
		if (value.arrValue()->size() == 0)
			ret = 1;
	}
	else if (value.type() == CVariable::DICT)
	{
		if (value.dictValue()->size() == 0)
			ret = 1;
	}
	else if (value.type() == CVariable::CLASS)
	{
		ret = 0;
	}
	else if (value.type() == CVariable::POINTER)
	{
		if (value.pointerValue() == 0)
			ret = 1;
	}

	res.setInt(ret);
}

void CInterpreter::copy_object(CVariable& object, CVariable& newObj)
{
	if (object.type() == CVariable::ARRAY)
	{
		newObj.setArray(object.arrValue()->size());
		for (size_t i = 0; i < object.arrValue()->size(); i++)
		{
			if ((*object.arrValue())[i].type() == CVariable::ARRAY ||
				(*object.arrValue())[i].type() == CVariable::DICT ||
				(*object.arrValue())[i].type() == CVariable::CLASS
				)
			{
				copy_object((*object.arrValue())[i], (*newObj.arrValue())[i]);
			}
			else
				(*newObj.arrValue())[i] = (*object.arrValue())[i];
		}
		return;
	}
	if (object.type() == CVariable::DICT || object.type() == CVariable::CLASS)
	{
		newObj.setDict();
		newObj.setType(object.type());
		newObj.setPointer(object.pointerValue());
		newObj.setInfo(object.info());
		map<CVariable, CVariable>::iterator iter = object.dictValue()->begin();
		for (; iter != object.dictValue()->end(); iter++)
		{
			if (iter->second.type() == CVariable::ARRAY ||
				iter->second.type() == CVariable::DICT ||
				iter->second.type() == CVariable::CLASS
				)
			{
				copy_object(iter->second, (*newObj.dictValue())[iter->first]);
			}
			else
				(*newObj.dictValue())[iter->first] = iter->second;
		}
		return;

	}

	newObj = object;
}

void CInterpreter::print_object(CVariable& object)
{
	if (object.type() == CVariable::ARRAY)
	{
		re_printf("[");
		for (size_t i = 0; i < object.arrValue()->size(); i++)
		{
			if ((*object.arrValue())[i].type() == CVariable::ARRAY ||
				(*object.arrValue())[i].type() == CVariable::DICT
				)
			{
				print_object((*object.arrValue())[i]);
			}
			else
			{
				CVariable& result = (*object.arrValue())[i];
				if (result.type() == CVariable::INT)
					re_printf(_INTFMT, result.intValue());
				else if (result.type() == CVariable::FLOAT)
					re_printf("%f ", result.floatValue());
				else if (result.type() == CVariable::STRING)
				{
					if (m_pFileOut)
						fprintf(m_pFileOut, "\"%s\" ", result.strValue().c_str());
					else
						printf("\"%s\" ", result.strValue().c_str());
				}
				else if (result.type() == CVariable::POINTER)
					re_printf("%p ", result.pointerValue());
				else if (result.type() == CVariable::CLASS)
				{
					re_printf("<class %p> ", result.dictValue());
				}
				else if (result.type() == CVariable::EMPTY)
				{
					re_printf("<undefined> ");
				}
				else if (result.type() == CVariable::NONE)
				{
					re_printf("<null> ");
				}

			}
			if (i< object.arrValue()->size() - 1)
				re_printf(", ");
		}
		re_printf("] ");
	}
	if (object.type() == CVariable::DICT)
	{
		re_printf("{");
		size_t i = 0;
		size_t count = object.dictValue()->size();
		map<CVariable, CVariable>::iterator iter = object.dictValue()->begin();
		for (; iter != object.dictValue()->end(); iter++)
		{
			const CVariable& result = iter->first;
			if (result.type() == CVariable::INT)
				re_printf(_INTFMT, result.intValue());
			else if (result.type() == CVariable::FLOAT)
				re_printf("%f ", result.floatValue());
			else if (result.type() == CVariable::STRING)
			{
				if (m_pFileOut)
					fprintf(m_pFileOut, "\"%s\" ", result.strValue().c_str());
				else
					printf("\"%s\" ", result.strValue().c_str());
			}
			else if (result.type() == CVariable::POINTER)
				re_printf("%p ", result.pointerValue());
			else if (result.type() == CVariable::ARRAY)
			{
				CVariable key = iter->first;
				re_printf("<array %p> ", key.arrValue());
			}
			else if (result.type() == CVariable::DICT)
			{
				CVariable key = iter->first;
				re_printf("<dict %p> ", key.dictValue());
			}
			else if (result.type() == CVariable::CLASS)
			{
				CVariable key = iter->first;
				re_printf("<class %p> ", key.dictValue());
			}
			else if (result.type() == CVariable::EMPTY)
			{
				re_printf("<undefined> ");
			}
			else if (result.type() == CVariable::NONE)
			{
				re_printf("<null> ");
			}

			re_printf(": ");

			if (iter->second.type() == CVariable::ARRAY ||
				iter->second.type() == CVariable::DICT
				)
			{
				print_object(iter->second);
			}
			else
			{
				CVariable& result2 = iter->second;
				if (result2.type() == CVariable::INT)
					re_printf(_INTFMT, result2.intValue());
				else if (result2.type() == CVariable::FLOAT)
					re_printf("%f ", result2.floatValue());
				else if (result2.type() == CVariable::STRING)
				{
					if (m_pFileOut)
						fprintf(m_pFileOut, "\"%s\" ", result2.strValue().c_str());
					else
						printf("\"%s\" ", result2.strValue().c_str());
				}
				else if (result2.type() == CVariable::POINTER)
					re_printf("%p ", result2.pointerValue());
				else if (result2.type() == CVariable::CLASS)
				{
					re_printf("<class %p> ", result2.dictValue());
				}
				else if (result2.type() == CVariable::EMPTY)
				{
					re_printf("<undefined> ");
				}
				else if (result2.type() == CVariable::NONE)
				{
					re_printf("<null> ");
				}

			}
			if (i<count - 1)
				re_printf(", ");
			i++;
		}
		re_printf("} ");

	}

}