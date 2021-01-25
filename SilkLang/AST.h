#pragma once
#include "Token.h"
#include "Variable.h"

//Abstract Syntax Tree
class AST
{
public:
	enum ASTTYPE{
		EMPTY, NONE, BINOP, NUM, BOOL, STR, ARRAY, DICT, VAR, ASSIGN, INCLUDE, PROGRAM, BLOCK, COMPOUND,IFCOMPOUND, 
		WHILECOMPOUND, FORCOMPOUND, BREAK, RETURN, CONTINUE, BUILTIN, FUNCTION, CLASS, FUNCTION_EXEC, MEMBER, GLOBAL
	};
	AST();
	~AST();
	ASTTYPE type() const { return m_type; }
	CToken& token(){ return m_token; }
protected:
	ASTTYPE m_type;
	CToken m_token;
};

class NoOp : public AST
{
public:
	NoOp(){ m_type = EMPTY; }
};

class None : public AST
{
public:
	None(){ m_type = NONE; }
};

class BinOp: public AST
{
public:
	BinOp(AST* left, const CToken& op, AST* right);
	AST* left(){ return m_left; }
	AST* right(){ return m_right; }
private:
	AST* m_left;
	AST* m_right;
};

class Num : public AST
{
public:
	enum NUMTYPE{ INT, FLOAT };
	Num(const CToken& token, NUMTYPE ntype = INT);
	_INT value() const { return m_nValue; }
	double float_value() const { return m_dValue; }
	NUMTYPE num_type() const { return m_num_type; }
	void set_minus();
private:
	NUMTYPE m_num_type;
	_INT m_nValue;
	double m_dValue;
};

class Bool : public AST
{
public:
	Bool(const CToken& token);
	bool value() const { return m_value; }
private:
	bool m_value;
};

class Str : public AST
{
public:
	Str(const CToken& token){ m_type = STR; m_token = token; m_value = m_token.value(); }
	string& value() { return m_value; }
private:
	string m_value;
};
class Array : public AST
{
public:
	Array(const CToken& token, AST* array_size);
	AST* array_size(){ return m_array_size; }
	vector<AST*>& inits(){ return m_inits; }
private:
	AST* m_array_size;
	vector<AST*> m_inits;
};
class Dict : public AST
{
public:
	Dict(const CToken& token);
	vector<AST*>& inits_left(){ return m_inits_left; }
	vector<AST*>& inits_right(){ return m_inits_right; }
private:
	vector<AST*> m_inits_left;
	vector<AST*> m_inits_right;
};

class Var : public AST
{
public:
	Var(const CToken& token);
	string& value() { return m_value; }
	void set_global(bool global){ m_global = global; }
	bool global(){ return m_global; }
	vector<AST*>& exprs(){ return m_exprs; }
	void set_func(bool func){ m_func = func; }
	bool is_func(){ return m_func; }
private:
	string m_value;
	bool m_global;
	vector<AST*> m_exprs;
	bool m_func;
};

class Assign : public AST
{
public:
	Assign(AST* left, const CToken& op, AST* right);
	AST* left(){ return m_left; }
	AST* right(){ return m_right; }
private:
	AST* m_left;
	AST* m_right;
};

class ClassStatement : public AST
{
public:
	ClassStatement(string name, const CToken& token);
	vector<string>& params(){ return m_params; }
	vector<AST*>& params_value(){ return m_params_value; }
	vector<AST*>& statements(){ return m_statements; }
	string name() const { return m_name; }
private:
	string m_name;
	vector<string> m_params;
	vector<AST*> m_params_value;
	vector<AST*> m_statements;
};
class FunctionStatement : public AST
{
public:
	FunctionStatement(string name, const CToken& token);
	vector<string>& params(){ return m_params; }
	vector<AST*>& params_value(){ return m_params_value; }
	vector<AST*>& statements(){ return m_statements; }
	string name() const { return m_name; }
private:
	string m_name;
	vector<string> m_params;
	vector<AST*> m_params_value;
	vector<AST*> m_statements;
};
class FunctionExec : public AST
{
public:
	FunctionExec(AST* statement, const CToken& token);
	AST* fun_statement(){ return m_statement; }
	vector<AST*>& exprs(){ return m_exprs; }
	bool is_var() { return m_var; }
	void set_var(bool var){ m_var = var; }
private:
	AST* m_statement;
	vector<AST*> m_exprs;
	bool m_var;
};

class BuiltinStatement : public AST
{
public:
	BuiltinStatement(const CToken& token);
	vector<AST*>& exprs(){ return m_exprs; }
private:
	vector<AST*> m_exprs;
};

class Member : public AST
{
public:
	enum MEMBERTYPE{ VAR, FUN };
	Member(const CToken& token,MEMBERTYPE memtype=FUN);
	vector<AST*>& exprs(){ return m_exprs; }
	MEMBERTYPE member_type(){ return m_memtype; }
private:
	MEMBERTYPE m_memtype;
	vector<AST*> m_exprs;
};

class BreakStatement : public AST
{
public:
	BreakStatement(const CToken& token){ m_type = BREAK; m_token = token; }
};
class ContinueStatement : public AST
{
public:
	ContinueStatement(const CToken& token){ m_type = CONTINUE; m_token = token; }
};
class ReturnStatement : public AST
{
public:
	ReturnStatement(const CToken& token, AST* expr);
	AST* expr(){ return m_expr; }
private:
	AST* m_expr;
};
class WhileCompound : public AST
{
public:
	WhileCompound(const CToken& token, AST* expr);
	vector<AST*>& statements(){ return m_statements; }
	AST* expr(){ return m_expr; }
private:
	AST* m_expr;
	vector<AST*> m_statements;
};
class ForCompound : public AST
{
public:
	ForCompound(const CToken& token, const vector<AST*>& init, AST* expr, const vector<AST*>& update);
	vector<AST*>& statements(){ return m_statements; }
	vector<AST*>& init_statements(){ return m_init; }
	AST* expr(){ return m_expr; }
	vector<AST*>& update_statements(){ return m_update; }
private:
	vector<AST*> m_init;
	AST* m_expr;
	vector<AST*> m_update;
	vector<AST*> m_statements;
};
class IfCompound : public AST
{
public:
	IfCompound(const CToken& token, AST* expr);
	vector<AST*>& true_statements(){ return m_true_statements; }
	vector<AST*>& false_statements(){ return m_false_statements; }
	AST* expr(){ return m_expr; }
private:
	AST* m_expr;
	vector<AST*> m_true_statements;
	vector<AST*> m_false_statements;
};
class Compound : public AST
{
public:
	Compound();
	vector<AST*>& child(){ return m_child; }
private:
	vector<AST*> m_child;
};
class Block : public AST
{
public:
	Block(Compound* compound);
	Compound* compound(){ return m_compound; }
private:
	Compound* m_compound;
};
class Program : public AST
{
public:
	Program(string name, Block* block);
	string name() const { return m_name; }
	Block* block(){ return m_block; }
	vector<AST*>& globals(){ return m_global_vars; }
	vector<string>& params(){ return m_params; }
private:
	string m_name;
	Block* m_block;
	vector<string> m_params;
	vector<AST*> m_global_vars;
};
class IncludeStatement : public AST
{
public:
	IncludeStatement(const CToken& token){ m_type = INCLUDE; m_token = token; }
	vector<AST*>& globals(){ return m_global_vars; }
private:
	vector<AST*> m_global_vars;
};
class GlobalStatement : public AST
{
public:
	GlobalStatement(const CToken& token){ m_type = GLOBAL; m_token = token; }
	map<string, bool>& vars(){ return m_vars; }
private:
	map<string, bool> m_vars;
};