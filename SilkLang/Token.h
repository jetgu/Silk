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
#include <string>
#include <map>
using namespace std;

#define GLOBAL_DICT_NAME "__G__"
#define ARGV_ARRAY_NAME "__ARGV__"

enum KEYWORD {
	PROGRAM, BEGIN, END, IF, ELSE, WHILE, FOR, ID, INTEGER, REAL, STRING, GLOBAL,
	EQUAL, NOT_EQUAL, NOT, ASSIGN, PLUS, MINUS, MUL, DIV, MOD, SEMI, LPAREN, RPAREN,
	BREAK, RETURN, CONTINUE, GREATER, LESS, GREATER_EQUAL, LESS_EQUAL, AND, OR, COLON,
	BUILTIN, FUNCTION, COMMA, PLUS_PLUS, MINUS_MINUS, PLUS_EQUAL, MINUS_EQUAL, MUL_EQUAL, 
	DIV_EQUAL, MOD_EQUAL, LSQUARE, RSQUARE, DOT, SHARP, NONE, BTRUE, BFALSE, CLASS, STATIC, EOFI
};

class CToken
{
public:
	CToken();
	CToken(KEYWORD type, const string& value, int lineNo, int column, const string& filename);
	~CToken();
	KEYWORD type() { return m_enuType; }
	string value() const { return m_strValue; }
	int lineNo() const { return m_nLineNo; }
	int column() const { return m_nColumn; }
	string filename() const { return m_strFilename; }
	void set_filename(const string& filename){ m_strFilename = filename; }
	void set_line_column(int lineNo, int column);
	static CToken getToken(const string& key, int lineNo, int column, const string& filename);
private:
	KEYWORD m_enuType;
	string m_strValue;
	int		m_nLineNo;
	int		m_nColumn;
	string m_strFilename;
};

