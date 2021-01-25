/*
License for Silk

Copyright 2020 by Gu Hong <macrogu@126.com>

All Rights Reserved

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE. 
*/

#pragma once
#include "Parser.h"
#include "Variable.h"
#include "CallStack.h"
#include "Library.h"
#include<vector>

#define _VERSION "1.0.0"
#ifdef _WIN32
	#ifdef _WIN64
		#define _PLATFORM "WIN64"
	#else
		#define _PLATFORM "WIN32"
	#endif
#else
	#ifdef __linux__
		#ifdef __x86_64__
			#define _PLATFORM "LINUX 64"
		#elif __i386__
			#define _PLATFORM "LINUX 32"
		#endif
	#else
		#ifdef __x86_64__
			#define _PLATFORM "MAC 64"
		#elif __i386__
			#define _PLATFORM "MAC 32"
		#endif
	#endif
#endif



class CInterpreter
{
public:
	CInterpreter(const CParser& parser);
	~CInterpreter();
	CVariable interpret();
	string error_msg(){ return m_error; }
	void set_outfile(FILE* out){ m_pFileOut = out; }
	void visit(AST* node, CVariable& res);
	CParser& get_parser(){ return m_parser; }
	CCallStack& get_callstack(){ return m_callstack; }
	void set_callstack(CCallStack& stack){ m_callstack=stack; }
	void set_argv(const vector<CVariable>& argv){ m_vecArgv = argv; }
	void set_globalvalue(const CVariable globalvalue){ m_globalValue = globalvalue; }
	void set_argvname(const string& name){ m_argvName = name; }

private:
	void visit_BinOp(BinOp* node, CVariable& res);
	void visit_Num(Num* node, CVariable& res);
	void visit_Bool(Bool* node, CVariable& res);
	void visit_Str(Str* node, CVariable& res);
	void visit_Array(Array* node, CVariable& res);
	void visit_Dict(Dict* node, CVariable& res);
	void visit_Var(Var* node, CVariable& res);
	void visit_Assign(Assign* node, CVariable& res);
	void visit_Program(Program* node, CVariable& res);
	void visit_Block(Block* node, CVariable& res);
	void visit_Compound(Compound* node, CVariable& res);
	void visit_IfCompound(IfCompound* node, CVariable& res);
	void visit_WhileCompound(WhileCompound* node, CVariable& res);
	void visit_ForCompound(ForCompound* node, CVariable& res);
	void visit_Break(BreakStatement* node, CVariable& res);
	void visit_Continue(ContinueStatement* node, CVariable& res);
	void visit_Return(ReturnStatement* node, CVariable& res);
	void visit_Include(IncludeStatement* node, CVariable& res);
	void visit_Builtin(BuiltinStatement* node, CVariable& res);
	void visit_Function(FunctionStatement* node, CVariable& res);
	void visit_FunctionExec(FunctionExec* node, CVariable& res);
	void visit_Class(FunctionExec* node, CVariable& res);
	void visit_Member(AST* obj, AST* member, CVariable& res);
	void visit_Index(AST* obj, AST* idx, CVariable& res);
	void visit_Not(AST* obj, CVariable& res);
	void exec_function(FunctionStatement* fun, vector<AST*>& exprs, CToken& token, CVariable& res);
	void exec_class(ClassStatement* cls, vector<AST*>& exprs, CToken& token, CVariable& res);
	void copy_object(CVariable& object, CVariable& res);
	void print_object(CVariable& object);
	void get_index_value(CVariable& var, CVariable& idx, CVariable& res);
	void set_index_value(const string& var_name, CVariable& var, CVariable& idx_value, const CVariable& result, const CToken& token);
	bool check_condition(CVariable& condition);

	void error(const string& err, const CToken& token);
	void re_printf(const char* format, ...);
private:
	CParser m_parser;
	CCallStack m_callstack;
	Lib_String m_libString;
	Lib_Array m_libArray;
	Lib_Dict m_libDict;
	Lib_Class m_libClass;
	Dll m_dll;
	string m_error;
	FILE* m_pFileOut;
	vector<CVariable> m_vecArgv;
	CVariable m_globalValue;
	string m_argvName;
};
