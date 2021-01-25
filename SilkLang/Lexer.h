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
#include "Token.h"

class CLexer
{
public:
	CLexer(){}
	CLexer(const string& text, const string& filename);
	~CLexer();

	void error();
	void advance();
	void skip_whitespace();
	void skip_comment();
	void skip_comment_block();
	char peek();
	CToken get_next_token();
	CToken peek_next_token();
	CToken id();
	CToken number();
	CToken str();
	void process_special_char(string& result);

private:
	string m_strText;
	string::size_type m_nPos;
	char m_curChar;
	int		m_nLineNo;
	int		m_nColumn;
	string m_strFilename;
};

