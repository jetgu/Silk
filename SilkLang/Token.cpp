#include "Token.h"

CToken::CToken()
{
	m_nLineNo = 0;
	m_nColumn = 0;
}
CToken::CToken(KEYWORD type, const string& value, int lineNo, int column, const string& filename)
{
	m_enuType = type;
	m_strValue = value;
	m_nLineNo = lineNo;
	m_nColumn = column;
	m_strFilename = filename;
}

CToken::~CToken()
{
}
void CToken::set_line_column(int lineNo, int column)
{
	m_nLineNo = lineNo;
	m_nColumn = column;
}

CToken CToken::getToken(const string& key, int lineNo, int column, const string& filename)
{
	static map<string, CToken> keywordMap;
	if(keywordMap.size()==0)
	{
		keywordMap["main"] = CToken(PROGRAM, "main", 0, 0, "");
		keywordMap["if"] = CToken(IF, "if", 0, 0, "");
		keywordMap["else"] = CToken(ELSE, "else", 0, 0, "");
		keywordMap["while"] = CToken(WHILE, "while", 0, 0, "");
		keywordMap["break"] = CToken(BREAK, "break", 0, 0, "");
		keywordMap["return"] = CToken(RETURN, "return", 0, 0, "");
		keywordMap["continue"] = CToken(CONTINUE, "continue", 0, 0, "");
		keywordMap["for"] = CToken(FOR, "for", 0, 0, "");
		keywordMap["null"] = CToken(NONE, "null", 0, 0, "");
		keywordMap["true"] = CToken(BTRUE, "true", 0, 0, "");
		keywordMap["false"] = CToken(BFALSE, "false", 0, 0, "");
		keywordMap["func"] = CToken(FUNCTION, "func", 0, 0, "");
		keywordMap["class"] = CToken(CLASS, "class", 0, 0, "");
		keywordMap["static"] = CToken(STATIC, "static", 0, 0, "");
		keywordMap["global"] = CToken(GLOBAL, "global", 0, 0, "");
		keywordMap["print"] = CToken(BUILTIN, "print", 0, 0, "");
		keywordMap["sprintf"] = CToken(BUILTIN, "sprintf", 0, 0, "");
		keywordMap["printf"] = CToken(BUILTIN, "printf", 0, 0, "");

		keywordMap[GLOBAL_DICT_NAME] = CToken(BUILTIN, GLOBAL_DICT_NAME, 0, 0, "");
		keywordMap[ARGV_ARRAY_NAME] = CToken(BUILTIN, ARGV_ARRAY_NAME, 0, 0, "");
		keywordMap["_input"] = CToken(BUILTIN, "_input", 0, 0, "");
		keywordMap["_getargv"] = CToken(BUILTIN, "_getargv", 0, 0, "");
		keywordMap["_copy"] = CToken(BUILTIN, "_copy", 0, 0, "");
		keywordMap["_loadlib"] = CToken(BUILTIN, "_loadlib", 0, 0, "");
		keywordMap["_freelib"] = CToken(BUILTIN, "_freelib", 0, 0, "");
		keywordMap["_calllib"] = CToken(BUILTIN, "_calllib", 0, 0, "");
		keywordMap["_createthread"] = CToken(BUILTIN, "_createthread", 0, 0, "");
		keywordMap["_createlock"] = CToken(BUILTIN, "_createlock", 0, 0, "");
		keywordMap["_lock"] = CToken(BUILTIN, "_lock", 0, 0, "");
		keywordMap["_unlock"] = CToken(BUILTIN, "_unlock", 0, 0, "");
		keywordMap["_len"] = CToken(BUILTIN, "_len", 0, 0, "");
		keywordMap["_str"] = CToken(BUILTIN, "_str", 0, 0, "");
		keywordMap["_int"] = CToken(BUILTIN, "_int", 0, 0, "");
		keywordMap["_float"] = CToken(BUILTIN, "_float", 0, 0, "");
		keywordMap["_type"] = CToken(BUILTIN, "_type", 0, 0, "");
		keywordMap["_fun"] = CToken(BUILTIN, "_fun", 0, 0, "");
	}

	map<string, CToken>::iterator iter = keywordMap.find(key);
	if (iter != keywordMap.end())
	{
		iter->second.set_line_column(lineNo, column);
		iter->second.set_filename(filename);
		return iter->second;
	}
	return CToken(ID, key,lineNo,column,filename);
}