#include "Lexer.h"


CLexer::CLexer(const string& text, const string& filename)
{
	m_strText = text;
	m_nPos = 0;
	m_curChar = 0;
	if (m_nPos >= 0 && m_nPos<m_strText.size())
		m_curChar = m_strText[m_nPos];
	m_nLineNo = 1;
	m_nColumn = 1;
	m_strFilename = filename;
}


CLexer::~CLexer()
{
}

void CLexer::error()
{
	printf("Invalid character\r\n");
}
void CLexer::advance()
{
	if (m_curChar == '\n')
	{
		m_nLineNo += 1;
		m_nColumn = 0;
	}

	m_nPos++;
	if (m_nPos > m_strText.size() - 1)
		m_curChar = 0; 
	else
	{
		m_curChar = m_strText[m_nPos];
		m_nColumn += 1;
	}
}
char CLexer::peek()
{
	string::size_type peek_pos = m_nPos + 1;
	if (peek_pos > m_strText.size() - 1)
		return 0; 
	else
		return m_strText[peek_pos];

}
void CLexer::skip_whitespace()
{
	while (m_curChar != 0 && (m_curChar == ' ' || m_curChar == '	' || m_curChar == '\r' || m_curChar == '\n'))
		advance();
}
void CLexer::skip_comment()
{
	while (m_curChar != '\n' && m_curChar != 0)
		advance();
	advance();
}
void CLexer::skip_comment_block()
{
	while (!(m_curChar == '*' && peek() == '/') && m_curChar != 0)
		advance();
	advance();
	advance();
}

CToken CLexer::id()
{
	string result;
	while (m_curChar != 0)
	{
		if (m_curChar >= 'a' && m_curChar <= 'z' || m_curChar >= 'A' && m_curChar <= 'Z'//is alphabet
			|| m_curChar >= '0' && m_curChar <= '9'//is number
			|| m_curChar == '_' || m_curChar == '$'
			)
			result += m_curChar;
		else
			break;
		advance();
	}
	return CToken::getToken(result, m_nLineNo, m_nColumn,m_strFilename);
}
CToken CLexer::number()
{
	string result;
	while (m_curChar != 0)
	{
		if (m_curChar >= '0' && m_curChar <= '9')//is number
			result += m_curChar;
		else
			break;
		advance();
	}
	if (m_curChar == '.')
	{
		result += m_curChar;
		advance();
		while (m_curChar != 0)
		{
			if (m_curChar >= '0' && m_curChar <= '9')//is number
				result += m_curChar;
			else
				break;
			advance();
		}
		return CToken(REAL, result, m_nLineNo, m_nColumn, m_strFilename);
	}
	else
	{
		return CToken(INTEGER, result, m_nLineNo, m_nColumn, m_strFilename);
	}
	return CToken::getToken(result, m_nLineNo, m_nColumn, m_strFilename);
}

void CLexer::process_special_char(string& result)
{
	string new_result;
	for (string::size_type i = 0; i < result.size(); i++)
	{
		if (result[i] == '\\' && i < result.size() - 1)
		{
			if (result[i + 1] == '\\')
			{
				new_result += '\\';
				i++;
				continue;
			}
			if (result[i + 1] == 'r')
			{
				new_result += '\r';
				i++;
				continue;
			}
			if (result[i + 1] == 'n')
			{
				new_result += '\n';
				i++;
				continue;
			}
			if (result[i + 1] == 't')
			{
				new_result += '\t';
				i++;
				continue;
			}
			if (result[i + 1] == 'a')
			{
				new_result += '\a';
				i++;
				continue;
			}
			if (result[i + 1] == 'b')
			{
				new_result += '\b';
				i++;
				continue;
			}
			if (result[i + 1] == 'v')
			{
				new_result += '\v';
				i++;
				continue;
			}

		}
		new_result += result[i];
	}
	result = new_result;
}
CToken CLexer::str()
{
	string result;
	while (m_curChar != '"' && m_curChar != 0)
	{
		if (m_curChar == '\\')
		{
			//int nPrevPos = m_nPos - 1;
			if (m_nPos >= 1 && m_strText[m_nPos - 1] != '\\')
			{
				string::size_type nNextPos = m_nPos + 1;
				if (nNextPos < m_strText.size() && m_strText[nNextPos] == '"')
					advance();
			}
		}

		result += m_curChar;
		advance();
	}
	advance();

	process_special_char(result);

	return CToken(STRING, result, m_nLineNo, m_nColumn, m_strFilename);
}
CToken CLexer::peek_next_token()
{

	string::size_type peek_pos = m_nPos;
	string result = "";
	while (m_strText[peek_pos] != 0)
	{
		if (m_strText[peek_pos] == ' ' || m_strText[peek_pos] == '	' || m_strText[peek_pos] == '\r' || m_strText[peek_pos] == '\n')//space
		{
			peek_pos++;
			continue;
		}
		if (m_strText[peek_pos] == '/' && m_strText[peek_pos + 1] == '/')//comment
		{
			peek_pos++;
			peek_pos++;
			while (m_strText[peek_pos] != 0 &&
				m_strText[peek_pos] != '\n' && m_strText[peek_pos] != 0)
				peek_pos++;
			peek_pos++;
			continue;
		}
		if (m_strText[peek_pos] == '/' && m_strText[peek_pos + 1] == '*')//comment block
		{
			peek_pos++;
			peek_pos++;
			while (m_strText[peek_pos] != 0 &&
				!(m_strText[peek_pos] == '*' && m_strText[peek_pos + 1] == '/'))
				peek_pos++;
			peek_pos++;
			peek_pos++;
			continue;
		}

		break;

	}

	while (m_strText[peek_pos] != 0 &&
		(m_strText[peek_pos] >= 'a' && m_strText[peek_pos] <= 'z' || m_strText[peek_pos] >= 'A' && m_strText[peek_pos] <= 'Z'//is alphabet
		|| m_strText[peek_pos] >= '0' && m_strText[peek_pos] <= '9' //is number
		|| m_strText[peek_pos] == '_' || m_strText[peek_pos] == '$'))
	{
		result += m_strText[peek_pos];
		peek_pos++;
	}

	return CToken::getToken(result, m_nLineNo, m_nColumn, m_strFilename);
}
CToken CLexer::get_next_token()
{
	while (m_curChar != 0)
	{
		if (m_curChar == ' ' || m_curChar == '	' || m_curChar == '\r' || m_curChar == '\n')
		{
			skip_whitespace();
			continue;
		}
		if (m_curChar == '/' && peek() == '/')
		{
			advance();
			advance();
			skip_comment();
			continue;
		}
		if (m_curChar == '#' && peek() == '!')//for apache cgi
		{
			advance();
			advance();
			skip_comment();
			continue;
		}
		if (m_curChar == '/' && peek() == '*')
		{
			advance();
			advance();
			skip_comment_block();
			continue;
		}
		if (m_curChar == '{')
		{
			advance();
			return CToken(BEGIN, "{", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '}')
		{
			advance();
			return CToken(END, "}", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '[')
		{
			advance();
			return CToken(LSQUARE, "[", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == ']')
		{
			advance();
			return CToken(RSQUARE, "]", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '"')
		{
			advance();
			return str();
		}
		if (m_curChar >= 'a' && m_curChar <= 'z' || m_curChar >= 'A' && m_curChar <= 'Z' || m_curChar == '_' || m_curChar == '$')//is alphabet
		{
			return id();
		}
		if (m_curChar >= '0' && m_curChar <= '9')//is number
		{
			return number();
		}
		if (m_curChar == '&' && peek() == '&')
		{
			advance();
			advance();
			return CToken(AND, "&&", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '|' && peek() == '|')
		{
			advance();
			advance();
			return CToken(OR, "||", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '=' && peek() == '=')
		{
			advance();
			advance();
			return CToken(EQUAL, "==", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '!' && peek() == '=')
		{
			advance();
			advance();
			return CToken(NOT_EQUAL, "!=", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '>'&& peek() == '=')
		{
			advance();
			advance();
			return CToken(GREATER_EQUAL, ">=", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '<'&& peek() == '=')
		{
			advance();
			advance();
			return CToken(LESS_EQUAL, "<=", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '>')
		{
			advance();
			return CToken(GREATER, ">", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '<')
		{
			advance();
			return CToken(LESS, "<", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '=')
		{
			advance();
			return CToken(ASSIGN, "=", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '!')
		{
			advance();
			return CToken(NOT, "!", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '+' && peek() == '+')
		{
			advance();
			advance();
			return CToken(PLUS_PLUS, "++", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '-' && peek() == '-')
		{
			advance();
			advance();
			return CToken(MINUS_MINUS, "--", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '+' && peek() == '=')
		{
			advance();
			advance();
			return CToken(PLUS_EQUAL, "+=", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '-' && peek() == '=')
		{
			advance();
			advance();
			return CToken(MINUS_EQUAL, "-=", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '*' && peek() == '=')
		{
			advance();
			advance();
			return CToken(MUL_EQUAL, "*=", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '/' && peek() == '=')
		{
			advance();
			advance();
			return CToken(DIV_EQUAL, "/=", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '%' && peek() == '=')
		{
			advance();
			advance();
			return CToken(MOD_EQUAL, "%=", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '+')
		{
			advance();
			return CToken(PLUS, "+", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '-')
		{
			advance();
			return CToken(MINUS, "-", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '*')
		{
			advance();
			return CToken(MUL, "*", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '/')
		{
			advance();
			return CToken(DIV, "/", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '%')
		{
			advance();
			return CToken(MOD, "%", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '(')
		{
			advance();
			return CToken(LPAREN, "(", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == ')')
		{
			advance();
			return CToken(RPAREN, ")", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == ';')
		{
			advance();
			return CToken(SEMI, ";", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == ',')
		{
			advance();
			return CToken(COMMA, ",", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == ':')
		{
			advance();
			return CToken(COLON, ":", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '.')
		{
			advance();
			return CToken(DOT, ".", m_nLineNo, m_nColumn, m_strFilename);
		}
		if (m_curChar == '#')
		{
			advance();
			return CToken(SHARP, "#", m_nLineNo, m_nColumn, m_strFilename);
		}

		error();
		break;
	}

	return  CToken(EOFI, "", m_nLineNo, m_nColumn, m_strFilename);
}