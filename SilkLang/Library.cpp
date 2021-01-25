#include "Library.h"
#include "Interpreter.h"
#include <time.h>
#include <algorithm>
#include <regex>
#include <math.h>

#ifdef _WIN32
	#include <fcntl.h>
	#include <io.h>
	#include <Winsock2.h>
	#pragma comment(lib,"WS2_32.lib")
#else
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <netdb.h>   
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <signal.h>
#endif

#define PARAM_INT 1
#define PARAM_STR 2
#define PARAM_DOUBLE 3
#define PARAM_POINTER 4

struct PARAM{
	void* pValue;
	int nSize;
	int nType;
};
struct PARAMS{
	int nCount;
	int nIndex;
	PARAM* pParams;
};

CInterpreter* _pInterpreter = NULL;
void dll_callback(PARAMS* pParam)
{
	//printf("dll_callback\r\n");
	if (pParam)
	{
		if (pParam->nCount > 0)
		{
			string args;
			vector<CVariable> vecArgv;
			if (pParam->pParams[0].nType == PARAM_POINTER)
			{
				CVariable fun;
				fun.setType(CVariable::POINTER);
				fun.setPointer((void*)(*(unsigned _INT*)pParam->pParams[0].pValue));
				fun.info()["func_type"] = "func";
				vecArgv.push_back(fun);
			}
			for (int i = 1; i < pParam->nCount; i++)
			{
				if (pParam->pParams[i].nType == PARAM_STR)
				{
					string str((char*)pParam->pParams[i].pValue, pParam->pParams[i].nSize);
					vecArgv.push_back(CVariable(str));
					char buff[256];
					sprintf(buff, "_getargv(\"__temp_argv__\")[%d]", i);
					if (args.size() == 0)
						args = buff;
					else
						args = args + "," + buff;
				}
			}

			CVariable globalValue;
			if (_pInterpreter)
			{
				CCallStack& callstack = _pInterpreter->get_callstack();
				ActivationRecord& base = callstack.base();
				globalValue = base.get_value(GLOBAL_DICT_NAME);
			}
			string strCode = "main() { fun=_getargv(\"__temp_argv__\")[0]; fun(" + args + "); }";
			char buff[128];
			sprintf_s(buff, "temp%d%d", rand(),rand());
			Tool::interpreter(strCode, "", vecArgv, globalValue, buff);

		}
	}
}
typedef void(*callback)(PARAMS* pParam);

void* Dll::loadlib(const string& filename)
{
	void* hDLL = NULL;
#ifdef _WIN32
	string::size_type pos = filename.rfind("\\");
	if(pos!=string::npos)
	{
		string strPath=filename.substr(0, pos + 1);
		SetDllDirectoryA(strPath.c_str());
	}
	
	hDLL = LoadLibraryA(filename.c_str());
#else
	hDLL = dlopen(filename.c_str(), RTLD_LAZY);
#endif

	return hDLL;
}
void Dll::freelib(void* handle)
{
#ifdef _WIN32
	FreeLibrary((HINSTANCE)handle);
#else
	dlclose(handle);
#endif
}
bool Dll::calllib(const vector<CVariable>& args, CVariable& arrResult)
{
	if (args.size() < 2)
		return false;

	void* hDLL = NULL;
	if (args[0].type() != CVariable::POINTER)
		return false;
	hDLL = args[0].pointerValue();
	if (hDLL == NULL)
		return false;

	string fun = args[1].strValue();

	typedef PARAMS*(*pFun)(int count);
	pFun create_params = NULL;
#ifdef _WIN32
	create_params = (pFun)GetProcAddress((HINSTANCE)hDLL, "create_params");
#else
	create_params = (pFun)dlsym(hDLL, "create_params");
#endif

	if (create_params)
	{
		typedef void(*pInt)(PARAMS* pParam, _INT value);
		pInt add_int = NULL;
		typedef void(*pDouble)(PARAMS* pParam, double value);
		pDouble add_double = NULL;
		typedef void(*pString)(PARAMS* pParam, const char* value, _INT size);
		pString add_string = NULL;
		typedef void(*pPointer)(PARAMS* pParam, void* value);
		pPointer add_ptr = NULL;
		typedef PARAMS*(*pCall)(PARAMS* pParam);
		pCall function = NULL;
#ifdef _WIN32
		add_int = (pInt)GetProcAddress((HINSTANCE)hDLL, "add_int");
		add_double = (pDouble)GetProcAddress((HINSTANCE)hDLL, "add_double");
		add_string = (pString)GetProcAddress((HINSTANCE)hDLL, "add_string");
		add_ptr = (pPointer)GetProcAddress((HINSTANCE)hDLL, "add_ptr");
		function = (pCall)GetProcAddress((HINSTANCE)hDLL, fun.c_str());
#else
		add_int = (pInt)dlsym(hDLL, "add_int");
		add_double = (pDouble)dlsym(hDLL, "add_double");
		add_string = (pString)dlsym(hDLL, "add_string");
		add_ptr = (pPointer)dlsym(hDLL, "add_ptr");
		function = (pCall)dlsym(hDLL, fun.c_str());
#endif

		bool bCallback = false;
		for (size_t i = 2; i < args.size(); i++)
		{
				CVariable varPointer = args[i];
				if (varPointer.info().find("func_type") != varPointer.info().end())
				{
					if (varPointer.info()["func_type"] == "func")
					{
						bCallback = true;
						break;
					}
				}
		}

		int nParams = (int)args.size() - 2;
		if (bCallback)
			nParams++;
		PARAMS* pParam = create_params(nParams);

		for (size_t i = 2; i < args.size(); i++)
		{
			if (args[i].type() == CVariable::INT)
			{
				add_int(pParam, args[i].intValue());
			}
			if (args[i].type() == CVariable::FLOAT)
			{
				add_double(pParam, args[i].floatValue());
			}
			if (args[i].type() == CVariable::STRING)
			{
				add_string(pParam, args[i].strValue().c_str(), args[i].strValue().size());
			}
			if (args[i].type() == CVariable::POINTER)
			{
				add_ptr(pParam, args[i].pointerValue());
			}
		}
		if (bCallback)
			add_ptr(pParam, (callback*)(dll_callback));

		PARAMS* pRetParam = function(pParam);
		if (pRetParam)
		{
			int n = pRetParam->nCount;

			arrResult.setArray(n);
			for (int i = 0; i < n; i++)
			{
				if (pRetParam->pParams[i].nType == PARAM_INT)
				{
					(*arrResult.arrValue())[i] = CVariable(*(int*)pRetParam->pParams[i].pValue);
				}
				else if (pRetParam->pParams[i].nType == PARAM_DOUBLE)
				{
					(*arrResult.arrValue())[i] = CVariable().setDouble(*(double*)pRetParam->pParams[i].pValue);
				}
				else if (pRetParam->pParams[i].nType == PARAM_STR)
				{
					string str((char*)pRetParam->pParams[i].pValue, pRetParam->pParams[i].nSize);
					(*arrResult.arrValue())[i] = CVariable(str);
				}
				else if (pRetParam->pParams[i].nType == PARAM_POINTER)
				{
					CVariable result;
					result.setType(CVariable::POINTER);
					result.setPointer((void*)(*(unsigned _INT*)pRetParam->pParams[i].pValue));
					(*arrResult.arrValue())[i] = result;
				}
			}
		}
		else
			arrResult.setArray(0);

		typedef void*(*pDel)(PARAMS* param);
		pDel delete_params = NULL;
#ifdef _WIN32
		delete_params = (pDel)GetProcAddress((HINSTANCE)hDLL, "delete_params");
#else
		delete_params = (pDel)dlsym(hDLL, "delete_params");
#endif
		if (delete_params)
		{
			delete_params(pParam);
			delete_params(pRetParam);
		}
	}

	return true;
}

Lock::Lock()
{
}
Lock::~Lock()
{
	for (size_t i = 0; i < m_vecLocks.size(); i++)
	{
		if (m_vecLocks[i])
			delete m_vecLocks[i];
	}
	m_vecLocks.clear();
}
void* Lock::create_lock()
{
	AutoLock lock(m_lock);

	CLock *pLock = new CLock();
	m_vecLocks.push_back(pLock);

	return pLock;
}
void Lock::lock(void* handle)
{
	if (handle)
		((CLock*)handle)->Lock();
}
void Lock::unlock(void* handle)
{
	if (handle)
		((CLock*)handle)->UnLock();
}

string Tool::mysprintf(string& format, vector<CVariable>& vecArgs)
{
	const string err1 = "sprintf argument number error\r\n";
	const string err2 = "sprintf argument type error\r\n";
	if (format.size() < 2)
		return err1;

	size_t index = 0;
	size_t pos = 0;
	while (format[pos] != 0)
	{

		if (format[pos] == '%')
		{
			size_t tPos = pos + 1;
			while (format[tPos] != 's' && format[tPos] != 'c'
				&& format[tPos] != 'd' && format[tPos] != 'f'
				&& format[tPos] != 'x' && format[tPos] != 'X'
				&& format[tPos] != 'o'
				&& format[tPos] != 0)
				tPos++;
			if (format[tPos] == 's')
			{
				string fmt = format.substr(pos, tPos - pos + 1);
				if (index >= vecArgs.size())
					return err1;
				if (vecArgs[index].type() != CVariable::STRING)
					return err2;
				if (vecArgs[index].strValue().size() < 0)
					return err1;
				char *buff = new char[vecArgs[index].strValue().size()+1];
				buff[vecArgs[index].strValue().size()] = 0;
				sprintf(buff, fmt.c_str(), vecArgs[index].strValue().c_str());
				format.replace(pos, fmt.size(), buff);
				pos += strlen(buff);
				delete []buff;
				index++;
			}
			else if (format[tPos] == 'c')
			{
				string fmt = format.substr(pos, tPos - pos + 1);
				if (index >= vecArgs.size())
					return err1;
				if (vecArgs[index].type() != CVariable::INT && vecArgs[index].type() != CVariable::STRING)
					return err2;
				if (vecArgs[index].type() == CVariable::STRING && vecArgs[index].strValue().size() != 1)
					return err2;
				char buff[1024];
				if (vecArgs[index].type() == CVariable::INT)
					sprintf(buff, fmt.c_str(), vecArgs[index].intValue());
				else if (vecArgs[index].type() == CVariable::STRING)
					sprintf(buff, fmt.c_str(), vecArgs[index].strValue()[0]);
				format.replace(pos, fmt.size(), buff);
				pos += strlen(buff);
				index++;

			}
			else if (format[tPos] == 'd')
			{
				string fmt = format.substr(pos, tPos - pos + 1);
				size_t fmt_size = fmt.size();
				if (sizeof(_INT) == 8)
					fmt.replace(fmt_size-1, 1, "lld");

				if (index >= vecArgs.size())
					return err1;
				if (vecArgs[index].type() != CVariable::INT && vecArgs[index].type() != CVariable::STRING)
					return err2;
				if (vecArgs[index].type() == CVariable::STRING && vecArgs[index].strValue().size()!=1)
					return err2;
				char buff[1024];
				if (vecArgs[index].type() == CVariable::INT)
					sprintf(buff, fmt.c_str(), vecArgs[index].intValue());
				else if (vecArgs[index].type() == CVariable::STRING)
					sprintf(buff, fmt.c_str(), vecArgs[index].strValue()[0]);
				format.replace(pos, fmt_size, buff);
				pos += strlen(buff);
				index++;
			}
			else if (format[tPos] == 'x' || format[tPos] == 'X' || format[tPos] == 'o')
			{
				string fmt = format.substr(pos, tPos - pos + 1);

				if (index >= vecArgs.size())
					return err1;
				if (vecArgs[index].type() != CVariable::INT && vecArgs[index].type() != CVariable::STRING)
					return err2;
				if (vecArgs[index].type() == CVariable::STRING && vecArgs[index].strValue().size() != 1)
					return err2;
				char buff[1024];
				if (vecArgs[index].type() == CVariable::INT)
					sprintf(buff, fmt.c_str(), vecArgs[index].intValue());
				else if (vecArgs[index].type() == CVariable::STRING)
					sprintf(buff, fmt.c_str(), vecArgs[index].strValue()[0]);
				format.replace(pos, fmt.size(), buff);
				pos += strlen(buff);
				index++;
			}
			else if (format[tPos] == 'f')
			{
				string fmt = format.substr(pos, tPos - pos + 1);
				if (index >= vecArgs.size())
					return err1;
				if (vecArgs[index].type() != CVariable::FLOAT)
					return err2;
				char buff[1024];
				sprintf(buff, fmt.c_str(), vecArgs[index].floatValue());
				format.replace(pos, fmt.size(), buff);
				pos += strlen(buff);
				index++;
			}
			else
				break;
		}
		else
			pos++;

		if (pos + 1 >= format.size())
			break;
	}
	return "";

}
string Tool::readfile(const string& strFilename)
{
	FILE *fp = fopen(strFilename.c_str(), "rb");
	if (fp == NULL)
		return "";

	fseek(fp, 0, SEEK_END);
	_INT size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* buffer = new char[size + 1];
	_INT ret = fread(buffer, 1, sizeof(char)*size, fp);
	string strbuf = string(buffer, ret);
	delete []buffer;

	fclose(fp);

	return strbuf;
}

char dec2hexChar(short int n) {
	if (0 <= n && n <= 9) {
		return char(short('0') + n);
	}
	else if (10 <= n && n <= 15) {
		return char(short('A') + n - 10);
	}
	else {
		return char(0);
	}
}

short int hexChar2dec(char c) {
	if ('0' <= c && c <= '9') {
		return short(c - '0');
	}
	else if ('a' <= c && c <= 'f') {
		return (short(c - 'a') + 10);
	}
	else if ('A' <= c && c <= 'F') {
		return (short(c - 'A') + 10);
	}
	else {
		return -1;
	}
}

string Tool::url_escape(const string &URL)
{
	string result = "";
	for (size_t i = 0; i<URL.size(); i++) {
		char c = URL[i];
		if (
			('0' <= c && c <= '9') ||
			('a' <= c && c <= 'z') ||
			('A' <= c && c <= 'Z') ||
			c == '/' || c == '.'
			) {
			result += c;
		}
		else {
			int j = (short int)c;
			if (j < 0) {
				j += 256;
			}
			int i1, i0;
			i1 = j / 16;
			i0 = j - i1 * 16;
			result += '%';
			result += dec2hexChar(i1);
			result += dec2hexChar(i0);
		}
	}
	return result;
}

string Tool::url_unescape(const string& URL) {
	string result = "";
	for (size_t i = 0; i<URL.size(); i++) {
		char c = URL[i];
		if (c != '%') {
			result += c;
		}
		else {
			char c1 = URL[++i];
			char c0 = URL[++i];
			int num = 0;
			num += hexChar2dec(c1) * 16 + hexChar2dec(c0);
			result += char(num);
		}
	}
	return result;
}

int Tool::str_count(const string& text, const string& str)
{
	int count = 0;
	string::size_type pos1 = 0;
	string::size_type pos2 = text.find(str,pos1);
	while (pos2 != string::npos)
	{
		count++;
		pos1 = pos2 + str.size();
		pos2 = text.find(str, pos1);
	}
	return count;
}
void Tool::str_replace(string& text, const string& str_old, const string& str_new)
{
	for (string::size_type pos(0); pos != string::npos; pos += str_new.length())
	{
		pos = text.find(str_old, pos);
		if (pos != string::npos)
			text.replace(pos, str_old.length(), str_new);
		else
			break;
	}
}
void Tool::str_split(const string& str, const string& splitstr, vector<string>& vecStr)
{
	string::size_type pos1, pos2;
	pos2 = str.find(splitstr);
	pos1 = 0;
	while (string::npos != pos2)
	{
		vecStr.push_back(str.substr(pos1, pos2 - pos1));

		pos1 = pos2 + splitstr.size();
		pos2 = str.find(splitstr, pos1);
	}
	vecStr.push_back(str.substr(pos1));
}
CVariable Tool::interpreter(const string& code, const string& outputfile, const vector<CVariable>& vecArgv, const CVariable& globalValue, string filename)
{
	FILE *out=NULL;
	if (outputfile.size() > 0)
		out = fopen(outputfile.c_str(), "w");

	string curdir;
#ifdef _WIN32
	string slash = "\\";
#else
	string slash = "/";
#endif		
	string::size_type pos = filename.rfind(slash);
	if (pos != string::npos)
	{
		curdir = filename.substr(0, pos + 1);
		filename = filename.substr(pos + 1);
	}

	CGlobalData globaldata;
	CLexer lexer(code, filename);
	CParser parser = CParser(lexer, &globaldata);
	parser.set_outfile(out);
	parser.set_curdir(curdir);

	CInterpreter interpreter = CInterpreter(parser);
	interpreter.set_outfile(out);
	if (globalValue.type() == CVariable::DICT)
	{
		interpreter.set_globalvalue(globalValue);
		interpreter.set_argvname("__temp_argv__");
	}
	interpreter.set_argv(vecArgv);
	CVariable ret = interpreter.interpret();

	if (out)
		fclose(out);

	return ret;
}

void* File::open(const char *filename, const char *mode)
{
	FILE *fp = fopen(filename, mode);
	return fp;
}
void File::close(void* handle)
{
	FILE* fp = (FILE*)handle;
	if (fp)
		fclose((FILE*)handle);
}
bool File::read(void* handle, _INT size, CVariable& result)
{
	if (size <= 0)
		return false;

	FILE* fp = (FILE*)handle;
	if (fp)
	{
		char* buffer= new char[size+1];
		_INT ret = fread(buffer, 1, sizeof(char)*size, fp);
		string strbuf = string(buffer, ret);
		delete []buffer;
		result = CVariable(strbuf);
		return true;
	}
	return false;
}
bool File::write(void* handle, CVariable& content)
{
	FILE* fp = (FILE*)handle;;
	if (fp)
	{
		if (content.type() == CVariable::STRING)
		{
			const char* buffer = content.strValue().c_str();
			_INT size = content.strValue().size();
			fwrite(buffer, 1, sizeof(char)*size, fp);
		}
		else if (content.type() == CVariable::INT)
		{
			_INT value = content.intValue();
			fwrite(&value, 1, sizeof(_INT) * 1, fp);
		}
		else if (content.type() == CVariable::FLOAT)
		{
			double value = content.floatValue();
			fwrite(&value, 1, sizeof(double) * 1, fp);
		}
		return true;
	}
	return false;

}
_INT File::size(void* handle)
{
	FILE* fp = (FILE*)handle;
	if (fp)
	{
#ifdef _WIN64
		_fseeki64(fp, 0, SEEK_END);
		_INT len = _ftelli64(fp);
		return len;
#else
		fseek(fp, 0, SEEK_END);
		_INT len = ftell(fp);
		return len;
#endif
	}

	return -1;
}
bool File::seek(void* handle, _INT pos)
{
	FILE* fp = (FILE*)handle;
	if (fp)
	{
#ifdef _WIN64
		_fseeki64(fp, pos, SEEK_SET);
#else
		fseek(fp, (long)pos, SEEK_SET);
#endif
		return true;
	}

	return false;
}

bool Lib::compare(const CVariable& a, const CVariable& b){
	
	if (a.type() != b.type())
	{
		if (a.type() < b.type())
			return true;
	}
	else
	{
		if (a.type() == CVariable::INT)
		{
			if (a.intValue() < b.intValue())
				return true;
		}
		if (a.type() == CVariable::FLOAT)
		{
			if (a.floatValue() < b.floatValue())
				return true;
		}
		if (a.type() == CVariable::STRING)
		{
			if (a.strValue() < b.strValue())
				return true;
		}
	}

	return false;
}

Lib_String::Lib_String()
{
	srand((unsigned)time(NULL));
	m_mapMembers["substr"] = SUBSTR;
	m_mapMembers["find"] = FIND;
	m_mapMembers["rfind"] = RFIND;
	m_mapMembers["replace"] = REPLACE;
	m_mapMembers["size"] = SIZE;
	m_mapMembers["split"] = SPLIT;
	m_mapMembers["insert"] = INSERT;
	m_mapMembers["erase"] = ERASE;
	m_mapMembers["trim"] = TRIM;
	m_mapMembers["ltrim"] = LTRIM;
	m_mapMembers["rtrim"] = RTRIM;
	m_mapMembers["lower"] = LOWER;
	m_mapMembers["upper"] = UPPER;
}
Lib_String::~Lib_String()
{
}

bool Lib_String::call_member(const string& name, CVariable& var, const vector<CVariable>& args, CVariable& ret)
{
	map<string, LIBMEMBER>::const_iterator itr = m_mapMembers.find(name);
	if ( itr == m_mapMembers.end())
	{
		error("member " + name + " not found.\r\n");
		return false;
	}

	if (itr->second == SUBSTR)
	{
		if (args.size() == 1)
		{
			CVariable pos = args[0];
			if (pos.type() == CVariable::INT)
			{
				if (pos.intValue() >= 0)
				{
					string str = var.strValue().substr(pos.intValue());
					ret = CVariable(str);
				}
			}
			return true;
		}
		if (args.size() == 2)
		{
			CVariable pos = args[0];
			CVariable len = args[1];
			if (pos.type() == CVariable::INT && len.type() == CVariable::INT)
			{
				if (pos.intValue() >= 0 && len.intValue()>=0)
				{
					string str = var.strValue().substr(pos.intValue(), len.intValue());
					ret = CVariable(str);
				}
			}
			return true;
		}
		else
			error("substr() arguments number error\r\n");
	}

	if (itr->second == FIND)
	{
		if (args.size() == 1)
		{
			if (var.type() == CVariable::STRING)
			{
				CVariable key = args[0];
				_INT pos = -1;
				if (key.type() == CVariable::STRING)
					pos = var.strValue().find(key.strValue());
				ret = CVariable(pos);
			}
			return true;
		}
		else if (args.size() == 2)
		{
			if (var.type() == CVariable::STRING)
			{
				CVariable key = args[0];
				CVariable start = args[1];
				_INT pos = -1;
				if (key.type() == CVariable::STRING)
					pos = var.strValue().find(key.strValue(),start.intValue());
				ret = CVariable(pos);
			}
			return true;
		}
		else
			error("find() arguments number error\r\n");

	}
	if (itr->second == RFIND)
	{
		if (args.size() == 1)
		{
			if (var.type() == CVariable::STRING)
			{
				CVariable key = args[0];
				_INT pos = -1;
				if (key.type() == CVariable::STRING)
					pos = var.strValue().rfind(key.strValue());
				ret = CVariable(pos);
			}
			return true;
		}
		else if (args.size() == 2)
		{
			if (var.type() == CVariable::STRING)
			{
				CVariable key = args[0];
				CVariable start = args[1];
				_INT pos = -1;
				if (key.type() == CVariable::STRING)
					pos = var.strValue().rfind(key.strValue(), start.intValue());
				ret = CVariable(pos);
			}
			return true;
		}
		else
			error("rfind() arguments number error\r\n");

	}

	if (itr->second == REPLACE)
	{
		if (args.size() == 2)
		{
			if (var.type() == CVariable::STRING)
			{
				string str = var.strValue();
				CVariable oldstr = args[0];
				CVariable newstr = args[1];
				if (oldstr.type() == CVariable::STRING && newstr.type() == CVariable::STRING)
				{
					string before = oldstr.strValue();
					string after = newstr.strValue();
					for (string::size_type pos(0); pos != string::npos; pos += after.length())
					{
						pos = str.find(before, pos);
						if (pos != string::npos)
							str.replace(pos, before.length(), after);
						else
							break;
					}
					ret = CVariable(str);
				}
			}
			return true;
		}
		else if (args.size() == 3)
		{
			if (var.type() == CVariable::STRING)
			{
				string str = var.strValue();
				CVariable pos = args[0];
				CVariable len = args[1];
				CVariable dest = args[2];
				if (pos.type() == CVariable::INT && len.type() == CVariable::INT && dest.type() == CVariable::STRING)
				{
					if (pos.intValue() >= 0)
						str.replace(pos.intValue(), len.intValue(), dest.strValue());
					ret = CVariable(str);
				}
			}
			return true;
		}
		else
			error("replace() arguments number error\r\n");
	}

	if (itr->second == SPLIT)
	{
		if (args.size() == 1)
		{
			if (var.type() == CVariable::STRING && args[0].type() == CVariable::STRING)
			{
				string str = var.strValue();
				string splitstr = args[0].strValue();
				
				CVariable vartemp;
				vartemp.setArray(0);

				string::size_type pos1, pos2;
				pos2 = str.find(splitstr);
				pos1 = 0;
				while (string::npos != pos2)
				{
					vartemp.arrValue()->push_back(str.substr(pos1, pos2 - pos1));

					pos1 = pos2 + splitstr.size();
					pos2 = str.find(splitstr, pos1);
				}
				vartemp.arrValue()->push_back(str.substr(pos1));
				ret = vartemp;
			}
			return true;
		}
		else
			error("split() arguments number error\r\n");
	}

	if (itr->second == SIZE)
	{
		if (args.size() == 1 && args[0].type() == CVariable::EMPTY)
		{
			if (var.type() == CVariable::STRING)
			{
				ret = CVariable((_INT)var.strValue().size());
			}
			return true;
		}
		else
			error("size() arguments number error\r\n");
	}

	if (itr->second == INSERT)
	{
		if (args.size() == 2)
		{
			if (var.type() == CVariable::STRING)
			{
				CVariable pos = args[0];
				CVariable str = args[1];
				string temp = var.strValue();
				if (pos.type() == CVariable::INT && pos.intValue() >= 0 && str.type() == CVariable::STRING)
					temp.insert(pos.intValue(), str.strValue());
				ret = CVariable(temp);
			}
			return true;
		}
		else
			error("insert() arguments number error\r\n");
	}
	if (itr->second == ERASE)
	{
		if (args.size() == 2)
		{
			if (var.type() == CVariable::STRING)
			{
				CVariable pos = args[0];
				CVariable len = args[1];
				string temp = var.strValue();
				if (pos.type() == CVariable::INT && pos.intValue() >= 0 && len.type() == CVariable::INT)
					temp.erase(pos.intValue(), len.intValue());
				ret = CVariable(temp);
			}
			return true;
		}
		else
			error("erase() arguments number error\r\n");
	}

	if (itr->second == TRIM)
	{
		if (args.size() == 1 && args[0].type() == CVariable::EMPTY)
		{
			if (var.type() == CVariable::STRING)
			{
				string value = var.strValue();
				string::size_type i = 0;
				for (i = 0; i < value.size(); i++) {
					if (value[i] != ' ' &&
						value[i] != '\t' &&
						value[i] != '\n' &&
						value[i] != '\r')
						break;
				}
				value = value.substr(i);
				for (i = value.size() - 1; i >= 0; i--) {
					if (value[i] != ' ' &&
						value[i] != '\t' &&
						value[i] != '\n' &&
						value[i] != '\r')
						break;
				}
				ret = CVariable(value.substr(0, i + 1));
			}
			return true;
		}
		else
			error("trim() arguments number error\r\n");
	}
	if (itr->second == LTRIM)
	{
		if (args.size() == 1 && args[0].type() == CVariable::EMPTY)
		{
			if (var.type() == CVariable::STRING)
			{
				string::size_type i = 0;
				for (i = 0; i < var.strValue().size(); i++) {
					if (var.strValue()[i] != ' ' &&
						var.strValue()[i] != '\t' &&
						var.strValue()[i] != '\n' &&
						var.strValue()[i] != '\r')
						break;
				}
				ret = CVariable(var.strValue().substr(i));
			}
			return true;
		}
		else
			error("ltrim() arguments number error\r\n");
	}
	if (itr->second == RTRIM)
	{
		if (args.size() == 1 && args[0].type() == CVariable::EMPTY)
		{
			if (var.type() == CVariable::STRING)
			{
				string::size_type i = 0;
				for (i = var.strValue().size()-1; i >=0; i--) {
					if (var.strValue()[i] != ' ' &&
						var.strValue()[i] != '\t' &&
						var.strValue()[i] != '\n' &&
						var.strValue()[i] != '\r')
						break;
				}
				ret = CVariable(var.strValue().substr(0, i + 1));
			}
			return true;
		}
		else
			error("rtrim() arguments number error\r\n");
	}

	if (itr->second == LOWER)
	{
		if (args.size() == 1 && args[0].type() == CVariable::EMPTY)
		{
			if (var.type() == CVariable::STRING)
			{
				string newvalue(var.strValue());
				for (string::size_type i = 0; i < newvalue.size(); i++)
					newvalue[i] = tolower(newvalue[i]);
				ret = CVariable(newvalue);
			}
			return true;
		}
		else
			error("lower() arguments number error\r\n");
	}
	if (itr->second == UPPER)
	{
		if (args.size() == 1 && args[0].type() == CVariable::EMPTY)
		{
			if (var.type() == CVariable::STRING)
			{
				string newvalue(var.strValue());
				for (string::size_type i = 0; i < newvalue.size(); i++)
					newvalue[i] = toupper(newvalue[i]);
				ret = CVariable(newvalue);
			}
			return true;
		}
		else
			error("upper() arguments number error\r\n");
	}

	return false;
}

Lib_Array::Lib_Array()
{
	m_mapMembers["append"] = APPEND;
	m_mapMembers["size"] = SIZE;
	m_mapMembers["resize"] = RESIZE;
	m_mapMembers["clear"] = CLEAR;
	m_mapMembers["erase"] = ERASE;
	m_mapMembers["insert"] = INSERT;
	m_mapMembers["_getptr"] = GETPTR;
	m_mapMembers["_restore"] = RESTORE;
	m_mapMembers["sort"] = SORT;
	m_mapMembers["swap"] = SWAP;
	m_mapMembers["create2d"] = CREATE2D;
	m_mapMembers["create3d"] = CREATE3D;
}
Lib_Array::~Lib_Array()
{
}

bool Lib_Array::call_member(const string& name, CVariable& var, vector<CVariable>& args, CVariable& ret)
{
	map<string, LIBMEMBER>::const_iterator itr = m_mapMembers.find(name);
	if (itr == m_mapMembers.end())
	{
		error("member " + name + " not found.\r\n");
		return false;
	}

	if (itr->second == APPEND)
	{
		if (args.size() == 1)
		{
			if (var.arrValue())
			{
				var.arrValue()->push_back(args[0]);
			}
			return true;
		}
		else
			error("push_back() arguments number error\r\n");
	}

	if (itr->second == SIZE)
	{
		if (args.size() == 1 && args[0].type()==CVariable::EMPTY)
		{
			if (var.arrValue())
			{
				ret = CVariable((_INT)var.arrValue()->size());
			}
			return true;
		}
		else
			error("size() arguments number error\r\n");
	}

	if (itr->second == CLEAR)
	{
		if (args.size() == 1 && args[0].type() == CVariable::EMPTY)
		{
			if (var.arrValue())
			{
				var.arrValue()->clear();
			}
			return true;
		}
		else
			error("clear() arguments number error\r\n");
	}

	if (itr->second == RESIZE)
	{
		if (args.size() == 1 || args.size() == 2)
		{
			if (var.arrValue())
			{
				if (args[0].type() == CVariable::INT)
				{
					_INT size = args[0].intValue();
					if (args.size() == 1)
						var.arrValue()->resize(size);
					else
						var.arrValue()->resize(size,args[1]);
				}
			}
			return true;
		}
		else
			error("resize() arguments number error\r\n");
	}

	if (itr->second == ERASE)
	{
		if (args.size() == 1 || args.size() == 2)
		{
			if (var.arrValue())
			{
				if (args[0].type() == CVariable::INT)
				{
					_INT index = args[0].intValue();
					if (index >= 0 && (size_t)index < var.arrValue()->size())
					{
						if (args.size() == 2)
						{
							_INT index2 = args[1].intValue();
							if (index2 >= index && (size_t)index2 < var.arrValue()->size())
							{
								var.arrValue()->erase(var.arrValue()->begin() + index, var.arrValue()->begin() + index2);
								ret = CVariable(1);
							}
						}
						else
						{
							var.arrValue()->erase(var.arrValue()->begin() + index);
							ret = CVariable(1);
						}
					}
					else
						ret = CVariable(0);
				}
			}
			return true;
		}
		else
			error("erase() arguments number error\r\n");
	}

	if (itr->second == INSERT)
	{
		if (args.size() == 2)
		{
			if (var.arrValue())
			{
				if (args[0].type() == CVariable::INT)
				{
					_INT index = args[0].intValue();
					if (index >= 0 && (size_t)index < var.arrValue()->size())
					{
						var.arrValue()->insert(var.arrValue()->begin() + index,args[1]);
						ret = CVariable(1);
					}
					else
						ret = CVariable(0);
				}
			}
			return true;
		}
		else
			error("insert() arguments number error\r\n");
	}
	if (itr->second == GETPTR)
	{
		if (args.size() == 1 && args[0].type() == CVariable::EMPTY)
		{
			if (var.arrValue())
			{
				ret.setType(CVariable::POINTER);
				ret.setPointer(var.arrValue());
			}
			return true;
		}
		else
			error("getptr() arguments number error\r\n");
	}
	if (itr->second == RESTORE)
	{
		if (args.size() == 1)
		{
			if (var.arrValue())
			{
				ret.setArray(0,(vector<CVariable>*)args[0].pointerValue());
				if (ret.arrValue() == NULL)
					ret.setType(CVariable::NONE);
			}
			return true;
		}
		else
			error("restore() arguments number error\r\n");
	}
	if (itr->second == SORT)
	{
		if (args.size() == 1)
		{
			if (var.arrValue())
			{
				sort(var.arrValue()->begin(), var.arrValue()->end(), compare);
				ret = CVariable(1);
			}
			return true;
		}
		else
			error("restore() arguments number error\r\n");
	}
	if (itr->second == SWAP)
	{
		if (args.size() == 1)
		{
			if (var.arrValue() && args[0].arrValue())
			{
				var.arrValue()->swap(*args[0].arrValue());
			}
			return true;
		}
		else
			error("restore() arguments number error\r\n");
	}
	if (itr->second == CREATE2D)
	{
		if (args.size() >= 2)
		{
			if (var.arrValue())
			{
				_INT d1 = args[0].intValue();
				_INT d2 = args[1].intValue();
				if (d1 >= 0 && d2 >= 0)
				{
					var.arrValue()->resize(d1);
					for (_INT i = 0; i < d1; i++)
					{
						CVariable vartemp;
						if (args.size() == 2)
							vartemp.setArray(d2);
						else
						{
							vartemp.setArray(0);
							vartemp.arrValue()->resize(d2, args[2]);
						}
						(*var.arrValue())[i] = vartemp;
					}
				}

			}
			return true;
		}
		else
			error("restore() arguments number error\r\n");
	}
	if (itr->second == CREATE3D)
	{
		if (args.size() >= 3)
		{
			if (var.arrValue())
			{
				_INT d1 = args[0].intValue();
				_INT d2 = args[1].intValue();
				_INT d3 = args[2].intValue();
				if (d1 >= 0 && d2 >= 0 && d3 >= 0)
				{
					var.arrValue()->resize(d1);
					for (_INT i = 0; i < d1; i++)
					{
						CVariable vartemp;
						vartemp.setArray(d2);
						(*var.arrValue())[i] = vartemp;
						for (_INT j = 0; j < d2; j++)
						{
							CVariable vartemp2;
							if (args.size() == 3)
								vartemp2.setArray(d3);
							else
							{
								vartemp2.setArray(0);
								vartemp2.arrValue()->resize(d3, args[3]);
							}
							(*(*var.arrValue())[i].arrValue())[j] = vartemp2;

						}

					}
				}

			}
			return true;
		}
		else
			error("restore() arguments number error\r\n");
	}

	return false;
}

Lib_Dict::Lib_Dict()
{
	m_mapMembers["find"] = FIND;
	m_mapMembers["size"] = SIZE;
	m_mapMembers["begin"] = BEGIN;
	m_mapMembers["end"] = END;
	m_mapMembers["rbegin"] = RBEGIN;
	m_mapMembers["rend"] = REND;
	m_mapMembers["next"] = NEXT;
	m_mapMembers["get"] = GET;
	m_mapMembers["erase"] = ERASE;
	m_mapMembers["insert"] = INSERT;
	m_mapMembers["clear"] = CLEAR;
	m_mapMembers["_getptr"] = GETPTR;
	m_mapMembers["_restore"] = RESTORE;

}
Lib_Dict::~Lib_Dict()
{
}

bool Lib_Dict::call_member(const string& name, CVariable& var, const vector<CVariable>& args, CVariable& ret)
{
	map<string, LIBMEMBER>::const_iterator itr = m_mapMembers.find(name);
	if (itr == m_mapMembers.end())
	{
		error("member " + name + " not found.\r\n");
		return false;
	}

	if (itr->second == FIND)
	{
		if (args.size() == 1)
		{
			if (var.dictValue())
			{
				if (var.dictValue()->find(args[0]) != var.dictValue()->end())
				{
					ret = (*var.dictValue())[args[0]];
				}
			}
			return true;
		}
		else
			error("find() arguments number error\r\n");
	}

	if (itr->second == ERASE)
	{
		if (args.size() == 1)
		{
			if (var.dictValue())
			{
				int nArgType = 0;
				if (args[0].type() == CVariable::POINTER)
				{
					CVariable arg0 = args[0];
					if (arg0.info().find("reverse_iterator") != arg0.info().end())
						nArgType = 1;
					else if (arg0.info().find("iterator") != arg0.info().end())
						nArgType = 2;
				}

				if (nArgType>0)
				{
					if (nArgType==1)
					{
						map<CVariable, CVariable>::reverse_iterator iter = *(map<CVariable, CVariable>::reverse_iterator*)args[0].pointerValue();
						var.dictValue()->erase((++iter).base());
						ret = args[0];
					}
					else
					{
						map<CVariable, CVariable>::iterator iter = *(map<CVariable, CVariable>::iterator*)args[0].pointerValue();
						var.dictValue()->erase(iter++);
						map<CVariable, CVariable>::iterator* pIter = (map<CVariable, CVariable>::iterator*)args[0].pointerValue();
						*pIter = iter;
						ret = args[0];
					}
				}
				else
				{
					map<CVariable, CVariable>::iterator iter = var.dictValue()->find(args[0]);
					if (iter != var.dictValue()->end())
					{
						var.dictValue()->erase(iter);
						ret = CVariable(1);
					}
					else
						ret = CVariable(0);
				}
			}
			return true;
		}
		else
			error("find() arguments number error\r\n");
	}
	if (itr->second == INSERT)
	{
		if (args.size() == 2)
		{
			if (var.dictValue())
			{
				pair<map<CVariable, CVariable>::iterator, bool> result = var.dictValue()->insert(pair<CVariable, CVariable>(args[0],args[1]));
				if (result.second == true)
					ret = CVariable(1);
				else
					ret = CVariable(0);
			}
			return true;
		}
		else
			error("insert() arguments number error\r\n");
	}
	if (itr->second == SIZE)
	{
		if (args.size() == 1 && args[0].type() == CVariable::EMPTY)
		{
			if (var.dictValue())
			{
				ret = CVariable((_INT)var.dictValue()->size());
			}
			return true;
		}
		else
			error("size() arguments number error\r\n");
	}
	if (itr->second == CLEAR)
	{
		if (args.size() == 1 && args[0].type() == CVariable::EMPTY)
		{
			if (var.dictValue())
			{
				var.dictValue()->clear();
			}
			return true;
		}
		else
			error("clear() arguments number error\r\n");
	}
	if (itr->second == GETPTR)
	{
		if (args.size() == 1 && args[0].type() == CVariable::EMPTY)
		{
			if (var.dictValue())
			{
				ret.setType(CVariable::POINTER);
				ret.setPointer(var.dictValue());
			}
			return true;
		}
		else
			error("getptr() arguments number error\r\n");
	}
	if (itr->second == RESTORE)
	{
		if (args.size() == 1)
		{
			if (var.dictValue())
			{
				ret.setDict((map<CVariable, CVariable>*)args[0].pointerValue());
				if (ret.dictValue() == NULL)
					ret.setType(CVariable::NONE);
			}
			return true;
		}
		else
			error("restore() arguments number error\r\n");
	}

	if (itr->second == BEGIN)
	{
		if (args.size() == 1 && args[0].type() == CVariable::EMPTY)
		{
			if (var.dictValue())
			{
				CVariable vartemp;
				vartemp.setType(CVariable::POINTER);
				vartemp.info()["iterator"] = "1";
				map<CVariable, CVariable>::iterator iter = var.dictValue()->begin();
				map<CVariable, CVariable>::iterator* pIter = new map<CVariable, CVariable>::iterator;
				*pIter = iter;
				vartemp.setPointer(pIter);
				vartemp.initPointerRef(pIter);
				ret = vartemp;
			}
			return true;
		}
		else
			error("begin() arguments number error\r\n");
	}
	if (itr->second == END)
	{
		if (args.size() == 1 && args[0].type() == CVariable::POINTER)
		{
			if (var.dictValue())
			{
				CVariable vartemp(0);
				map<CVariable, CVariable>::iterator iter = *(map<CVariable, CVariable>::iterator*)args[0].pointerValue();
				if (iter == var.dictValue()->end())
					vartemp = CVariable(1);
				ret = vartemp;
			}
			return true;
		}
		else
			error("end() arguments number error\r\n");
	}
	if (itr->second == NEXT)
	{
		if (args.size() == 1 && args[0].type() == CVariable::POINTER)
		{
			if (var.dictValue())
			{
				CVariable arg0 = args[0];
				if (arg0.info().find("reverse_iterator") != arg0.info().end())
				{
					map<CVariable, CVariable>::reverse_iterator iter = *(map<CVariable, CVariable>::reverse_iterator*)args[0].pointerValue();
					iter++;
					map<CVariable, CVariable>::reverse_iterator* pIter = (map<CVariable, CVariable>::reverse_iterator*)args[0].pointerValue();
					*pIter = iter;
					ret = args[0];
				}
				else
				{
					map<CVariable, CVariable>::iterator iter = *(map<CVariable, CVariable>::iterator*)args[0].pointerValue();
					iter++;
					map<CVariable, CVariable>::iterator* pIter = (map<CVariable, CVariable>::iterator*)args[0].pointerValue();
					*pIter = iter;
					ret = args[0];
				}
			}
			return true;
		}
		else
			error("next() arguments number error\r\n");
	}
	if (itr->second == RBEGIN)
	{
		if (args.size() == 1 && args[0].type() == CVariable::EMPTY)
		{
			if (var.dictValue())
			{
				CVariable vartemp;
				vartemp.setType(CVariable::POINTER);
				vartemp.info()["reverse_iterator"] = "1";
				map<CVariable, CVariable>::reverse_iterator iter = var.dictValue()->rbegin();
				map<CVariable, CVariable>::reverse_iterator* pIter = new map<CVariable, CVariable>::reverse_iterator;
				*pIter = iter;
				vartemp.setPointer(pIter);
				vartemp.initPointerRef(pIter);
				ret = vartemp;
			}
			return true;
		}
		else
			error("rbegin() arguments number error\r\n");
	}
	if (itr->second == REND)
	{
		if (args.size() == 1 && args[0].type() == CVariable::POINTER)
		{
			if (var.dictValue())
			{
				CVariable vartemp(0);
				map<CVariable, CVariable>::reverse_iterator  iter = *(map<CVariable, CVariable>::reverse_iterator*)args[0].pointerValue();
				if (iter == var.dictValue()->rend())
					vartemp = CVariable(1);
				ret = vartemp;
			}
			return true;
		}
		else
			error("rend() arguments number error\r\n");
	}
	if (itr->second == GET)
	{
		if (args.size() == 1)
		{
			if (var.dictValue())
			{
				CVariable vartemp;
				vartemp.setArray(2);
				int nArgType = 0;
				if (args[0].type() == CVariable::POINTER)
				{
					CVariable arg0 = args[0];
					if (arg0.info().find("reverse_iterator") != arg0.info().end())
						nArgType = 1;
					else if (arg0.info().find("iterator") != arg0.info().end())
						nArgType = 2;
				}

				if (nArgType>0)
				{
					if (nArgType==1)
					{
						map<CVariable, CVariable>::reverse_iterator iter = *(map<CVariable, CVariable>::reverse_iterator*)args[0].pointerValue();
						if (iter != var.dictValue()->rend())
						{
							(*vartemp.arrValue())[0] = iter->first;
							(*vartemp.arrValue())[1] = iter->second;
							ret = vartemp;
						}
					}
					else
					{
						map<CVariable, CVariable>::iterator iter = *(map<CVariable, CVariable>::iterator*)args[0].pointerValue();
						if (iter != var.dictValue()->end())
						{
							(*vartemp.arrValue())[0] = iter->first;
							(*vartemp.arrValue())[1] = iter->second;
							ret = vartemp;
						}
					}
				}
				else
				{
					map<CVariable, CVariable>::iterator iter = var.dictValue()->find(args[0]);
					if (iter != var.dictValue()->end())
					{
						(*vartemp.arrValue())[0] = iter->first;
						(*vartemp.arrValue())[1] = iter->second;
						ret = vartemp;
					}
				}
			}
			return true;
		}
		else
			error("get() arguments number error\r\n");
	}


	return false;
}

Lib_Class::Lib_Class()
{
	m_mapMembers["_getptr"] = GETPTR;
	m_mapMembers["_restore"] = RESTORE;

}
Lib_Class::~Lib_Class()
{
}

bool Lib_Class::call_member(const string& name, CVariable& var, const vector<CVariable>& args, CVariable& ret)
{
	map<string, LIBMEMBER>::const_iterator itr = m_mapMembers.find(name);
	if (itr == m_mapMembers.end())
	{
		//error("member " + name + " not found.\r\n");
		return false;
	}

	if (itr->second == GETPTR)
	{
		if (args.size() == 1 && args[0].type() == CVariable::EMPTY)
		{
			if (var.dictValue())
			{
				ret.setType(CVariable::POINTER);
				ret.setPointer(var.dictValue());
				return true;
			}
		}
		else
			error("getptr() arguments number error\r\n");
	}
	if (itr->second == RESTORE)
	{
		if (args.size() == 1)
		{
			if (var.dictValue())
			{
				ret.setDict((map<CVariable, CVariable>*)args[0].pointerValue());
				ret.setType(CVariable::CLASS);
				if (ret.dictValue() == NULL)
					ret.setType(CVariable::NONE);
				return true;
			}
		}
		else
			error("restore() arguments number error\r\n");
	}

	return false;
}

bool Func::call_func(const vector<CVariable>& args, CVariable& ret)
{
	if (args.size() == 0)
		return false;

	if (args[0].strValue() == "fopen")
	{
		if (args.size() == 3)
		{
			CVariable filename = args[1];
			CVariable mode = args[2];
			if (filename.type() == CVariable::STRING && mode.type() == CVariable::STRING)
			{
				File file;
				void* handle = file.open(filename.strValue().c_str(), mode.strValue().c_str());
				if (handle)
				{
					ret.setType(CVariable::POINTER);
					ret.setPointer(handle);
					return true;
				}
			}
		}
		return false;
	}
	if (args[0].strValue() == "fclose")
	{
		if (args.size()==2)
		{
			CVariable handle = args[1];
			if (handle.type() == CVariable::POINTER)
			{
				File file;
				file.close(handle.pointerValue());
			}
			return true;
		}
		return false;
	}
	if (args[0].strValue() == "fremove")
	{
		if (args.size() == 2)
		{
			CVariable filename = args[1];
			if (filename.strValue().size() >0)
			{
				int res=remove(filename.strValue().c_str());
				ret = CVariable(res);
			}
			return true;
		}
		return false;
	}
	if (args[0].strValue() == "frename")
	{
		if (args.size() == 3)
		{
			CVariable filename_old = args[1];
			CVariable filename_new = args[2];
			if (filename_old.strValue().size() >0 && filename_new.strValue().size() >0)
			{
				int res = rename(filename_old.strValue().c_str(), filename_new.strValue().c_str());
				ret = CVariable(res);
			}
			return true;
		}
		return false;
	}
	if (args[0].strValue() == "fsize")
	{
		if (args.size() == 2)
		{
			CVariable handle = args[1];
			if (handle.type() == CVariable::POINTER)
			{
				File file;
				_INT size = file.size(handle.pointerValue());
				ret= CVariable(size);
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "fseek")
	{
		if (args.size() == 3)
		{
			CVariable handle = args[1];
			CVariable pos = args[2];
			if (handle.type() == CVariable::POINTER && pos.type() == CVariable::INT)
			{
				File file;
				file.seek(handle.pointerValue(), pos.intValue());
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "fread")
	{
		if (args.size() == 3)
		{
			CVariable handle = args[1];
			CVariable size = args[2];
			if (handle.type() == CVariable::POINTER && size.type() == CVariable::INT)
			{
				File file;
				file.read(handle.pointerValue(), size.intValue(), ret);
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "fwrite")
	{
		if (args.size() == 3)
		{
			CVariable handle = args[1];
			CVariable content = args[2];
			if (handle.type() == CVariable::POINTER)
			{
				File file;
				bool bret = file.write(handle.pointerValue(), content);
				if (bret)
					ret = CVariable(1);
				else
					ret = CVariable(0);
				return true;
			}
		}
		return false;
	}

#ifdef _WIN32
	if (args[0].strValue() == "sock_listen")
	{
		if (args.size() == 2)
		{
			CVariable port = args[1];
			if (port.type() == CVariable::INT)
			{
				_INT nPort = port.intValue();
				if (nPort < 0)
					return false;

				WORD wVersionRequested;
				WSADATA wsaData;
				int err;

				wVersionRequested = MAKEWORD(1, 1);

				err = WSAStartup(wVersionRequested, &wsaData);
				if (err != 0) {
					return false;
				}

				if (LOBYTE(wsaData.wVersion) != 1 ||
					HIBYTE(wsaData.wVersion) != 1) {
					WSACleanup();
					return false;
				}
				SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);

				SOCKADDR_IN addrSrv;
				addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
				addrSrv.sin_family = AF_INET;
				addrSrv.sin_port = htons((u_short)nPort);

				bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR));

				int nRet = listen(sockSrv, 5);
				if (nRet == SOCKET_ERROR)
				{
					WSACleanup();
					return false;
				}

				ret.setType(CVariable::POINTER);
				ret.setPointer((void*)sockSrv);
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_close")
	{
		if (args.size() == 2)
		{
			CVariable socket = args[1];
			if (socket.type() == CVariable::POINTER)
			{
				SOCKET sock = (SOCKET)socket.pointerValue();
				closesocket(sock);
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_accept")
	{
		if (args.size() == 2)
		{
			CVariable socket = args[1];
			if (socket.type() == CVariable::POINTER)
			{
				SOCKET sockSrv = (SOCKET)socket.pointerValue();
				SOCKADDR_IN addrClient;
				int len = sizeof(SOCKADDR);
				SOCKET sockClient = accept(sockSrv, (SOCKADDR*)&addrClient, &len);

				ret.setType(CVariable::POINTER);
				ret.setPointer((void*)sockClient);
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_recv")
	{
		if (args.size() == 3)
		{
			CVariable socket = args[1];
			if (socket.type() == CVariable::POINTER)
			{
				SOCKET sock = (SOCKET)socket.pointerValue();
				int buffsize = 4096;
				buffsize = (int)args[2].intValue();
				if (buffsize <= 0)
					buffsize = 4096;
				char *recvBuf = new char[buffsize];
				int iResult = recv(sock, recvBuf, buffsize, 0);
				if (iResult < 0)
					iResult = 0;
				ret = CVariable(string(recvBuf, iResult));
				delete[]recvBuf;
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_send")
	{
		if (args.size() == 3)
		{
			CVariable socket = args[1];
			CVariable data = args[2];
			if (socket.type() == CVariable::POINTER)
			{
				SOCKET sock = (SOCKET)socket.pointerValue();
				int iResult = send(sock, data.strValue().c_str(), (int)data.strValue().size(), 0);
				ret = CVariable(iResult);
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_connect")
	{
		if (args.size() == 3)
		{
			CVariable server = args[1];
			CVariable port = args[2];
			if (server.type() == CVariable::STRING && port.type() == CVariable::INT)
			{
				WORD wVersionRequested;
				WSADATA wsaData;
				int err;

				wVersionRequested = MAKEWORD(1, 1);

				err = WSAStartup(wVersionRequested, &wsaData);
				if (err != 0) {
					return false;
				}

				if (LOBYTE(wsaData.wVersion) != 1 ||
					HIBYTE(wsaData.wVersion) != 1) {
					WSACleanup();
					return false;
				}
				SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);

				hostent* hostInfo = gethostbyname(server.strValue().c_str());
				if (NULL == hostInfo)
					return false;

				SOCKADDR_IN addrSrv;
				memcpy(&addrSrv.sin_addr, &(*hostInfo->h_addr_list[0]), hostInfo->h_length);
				addrSrv.sin_family = AF_INET;
				addrSrv.sin_port = htons((u_short)port.intValue());
				if (connect(sockClient, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)) == SOCKET_ERROR)
				{
					closesocket(sockClient);
					WSACleanup();
					return false;
				}
				ret.setType(CVariable::POINTER);
				ret.setPointer((void*)sockClient);
				return true;
			}
		}
	}
	if (args[0].strValue() == "sock_remoteaddr")
	{
		if (args.size() == 2)
		{
			CVariable socket = args[1];
			if (socket.type() == CVariable::POINTER)
			{
				SOCKET sock = (SOCKET)socket.pointerValue();
				struct sockaddr_in sa;
				int len = sizeof(sa);
				if(!getpeername(sock, (struct sockaddr *)&sa, &len))
				{
					ret.setArray(0);
					ret.arrValue()->push_back(CVariable(inet_ntoa(sa.sin_addr)));
					ret.arrValue()->push_back(CVariable(ntohs(sa.sin_port)));
					return true;
				}
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_timeout")
	{
		if (args.size() == 3)
		{
			CVariable socket = args[1];
			CVariable timeout = args[2];
			if (socket.type() == CVariable::POINTER)
			{
				SOCKET sock = (SOCKET)socket.pointerValue();
				int nTimeout = (int)timeout.intValue()*1000;
				setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&nTimeout, sizeof(int));
				setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&nTimeout, sizeof(int));
			 }
		}
		return false;
	}
	if (args[0].strValue() == "sock_shutdown")
	{
		if (args.size() == 3)
		{
			CVariable socket = args[1];
			CVariable how = args[2];
			if (socket.type() == CVariable::POINTER)
			{
				SOCKET sock = (SOCKET)socket.pointerValue();
				int nHow = (int)how.intValue();
				shutdown(sock, nHow);
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_ipaddr")
	{
		if (args.size() == 2)
		{
			CVariable domain = args[1];
			if (domain.type() == CVariable::STRING)
			{

				WORD wVersionRequested;
				WSADATA wsaData;
				int err;

				wVersionRequested = MAKEWORD(1, 1);

				err = WSAStartup(wVersionRequested, &wsaData);
				if (err != 0) {
					return false;
				}

				if (LOBYTE(wsaData.wVersion) != 1 ||
					HIBYTE(wsaData.wVersion) != 1) {
					WSACleanup();
					return false;
				}
				hostent* hostInfo = gethostbyname(domain.strValue().c_str());
				if (NULL == hostInfo)
					return false;

				ret.setArray(0);
				char buff[1024];
				for (int i = 0; hostInfo->h_addr_list[i]; i++){
					sprintf_s(buff,"%s", inet_ntoa(*(struct in_addr*)hostInfo->h_addr_list[i]));
					ret.arrValue()->push_back(CVariable(buff));
				}
				return true;
			}
		}
		return false;
	}
#else
	if (args[0].strValue() == "sock_listen")
	{
		if (args.size() == 2)
		{
			CVariable port = args[1];
			if (port.type() == CVariable::INT)
			{
				_INT nPort = port.intValue();
				if (nPort < 0)
					return false;

				int sockSrv = socket(AF_INET, SOCK_STREAM, 0);

				struct sockaddr_in addrSrv;
				memset(&addrSrv, 0, sizeof(addrSrv));
				addrSrv.sin_addr.s_addr = htonl(INADDR_ANY);
				addrSrv.sin_family = AF_INET;
				addrSrv.sin_port = htons(nPort);

				if (::bind(sockSrv, (struct sockaddr*)&addrSrv, sizeof(addrSrv)) < 0)
				{
					return false;
				}
				int nRet = listen(sockSrv, 5);
				if (nRet < 0)
				{
					return false;
				}
				ret = CVariable(sockSrv);
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_close")
	{
		if (args.size() == 2)
		{
			CVariable socket = args[1];
			if (socket.type() == CVariable::INT)
			{
				signal(SIGPIPE, SIG_IGN);
				int sock = (int)socket.intValue();
				::close(sock);
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_accept")
	{
		if (args.size() == 2)
		{
			CVariable socket = args[1];
			if (socket.type() == CVariable::INT)
			{
				int sockSrv = (int)socket.intValue();
				struct sockaddr_in addrClient;
				socklen_t len = sizeof(addrClient);

				int sockClient = accept(sockSrv, (struct sockaddr*)&addrClient, &len);

				ret = CVariable(sockClient);
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_recv")
	{
		if (args.size() == 3)
		{
			CVariable socket = args[1];
			if (socket.type() == CVariable::INT)
			{
				signal(SIGPIPE, SIG_IGN);
				int sock = (int)socket.intValue();
				int buffsize = 4096;
				buffsize = args[2].intValue();
				if (buffsize <= 0)
					buffsize = 4096;
				char *recvBuf = new char[buffsize];
				ssize_t iResult = recv(sock, recvBuf, buffsize, 0);
				if (iResult < 0)
					iResult = 0;
				ret = CVariable(string(recvBuf, iResult));
				delete[]recvBuf;
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_send")
	{
		if (args.size() == 3)
		{
			CVariable socket = args[1];
			CVariable data = args[2];
			if (socket.type() == CVariable::INT)
			{
				signal(SIGPIPE, SIG_IGN);
				int sock = (int)socket.intValue();
				#ifdef __linux__
					ssize_t iResult = send(sock, data.strValue().c_str(), data.strValue().size(), MSG_NOSIGNAL);
				#else
					int set=1;
					setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));
					ssize_t iResult = send(sock, data.strValue().c_str(), data.strValue().size(), 0);
				#endif
				ret=CVariable(iResult);
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_connect")
	{
		if (args.size() == 3)
		{
			CVariable server = args[1];
			CVariable port = args[2];
			if (server.type() == CVariable::STRING && port.type() == CVariable::INT)
			{
				int sockClient = socket(AF_INET, SOCK_STREAM, 0);

				hostent* hostInfo = gethostbyname(server.strValue().c_str());
				if (NULL == hostInfo)
					return false;

				struct sockaddr_in addrSrv;
				memcpy(&addrSrv.sin_addr, &(*hostInfo->h_addr_list[0]), hostInfo->h_length);
				addrSrv.sin_family = AF_INET;
				addrSrv.sin_port = htons((u_short)port.intValue());
				if (connect(sockClient, (struct sockaddr*)&addrSrv, sizeof(addrSrv)) != 0)
				{
					::close(sockClient);
					return false;
				}
				ret = CVariable(sockClient);
				return true;
			}
		}
	}
	if (args[0].strValue() == "sock_remoteaddr")
	{
		if (args.size() == 2)
		{
			CVariable socket = args[1];
			if (socket.type() == CVariable::INT)
			{
				int sock = (int)socket.intValue();
				struct sockaddr_in sa;
				socklen_t len = sizeof(sa);
				if (!getpeername(sock, (struct sockaddr *)&sa, &len))
				{
					ret.setArray(0);
					ret.arrValue()->push_back(CVariable(inet_ntoa(sa.sin_addr)));
					ret.arrValue()->push_back(CVariable(ntohs(sa.sin_port)));
					return true;
				}
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_shutdown")
	{
		if (args.size() == 3)
		{
			CVariable socket = args[1];
			CVariable how = args[2];
			if (socket.type() == CVariable::INT)
			{
				int sock = (int)socket.intValue();
				int nHow = (int)how.intValue();
				shutdown(sock, nHow);
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_ipaddr")
	{
		if (args.size() == 2)
		{
			CVariable domain = args[1];
			if (domain.type() == CVariable::STRING)
			{

				hostent* hostInfo = gethostbyname(domain.strValue().c_str());
				if (NULL == hostInfo)
					return false;

				ret.setArray(0);
				char buff[1024];
				for (int i = 0; hostInfo->h_addr_list[i]; i++){
					sprintf_s(buff, "%s", inet_ntoa(*(struct in_addr*)hostInfo->h_addr_list[i]));
					ret.arrValue()->push_back(CVariable(buff));
				}
				return true;
			}
		}
		return false;
	}
	if (args[0].strValue() == "sock_timeout")
	{
		if (args.size() == 3)
		{
			CVariable socket = args[1];
			CVariable timeout = args[2];
			if (socket.type() == CVariable::POINTER)
			{
				#ifdef __linux__
					int sock = (int)socket.intValue();
					int nTimeout = (int)timeout.intValue();
					struct timeval timeout={nTimeout,0};
					setsockopt(sock,SOL_SOCKET,SO_SNDTIMEO,(const char*)&timeout,sizeof(timeout));
					setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(const char*)&timeout,sizeof(timeout));
				#endif
			}
		}
		return false;
	}
#endif

	if (args[0].strValue() == "run")
	{
		if (args.size() >= 3)
		{
			const CVariable& text = args[1];
			string outputFile = args[2].strValue();
			vector<CVariable> vecArgv;
			for (size_t i = 3; i < args.size(); i++)
			{
				vecArgv.push_back(args[i]);
				vecArgv[vecArgv.size() - 1].index().clear();
			}

			char buff[128];
			sprintf_s(buff, "temp%d%d", rand(), rand());
			CVariable result = Tool::interpreter(text.strValue(), outputFile, vecArgv, CVariable(), buff);
			if (result.type() != CVariable::EMPTY)
				ret = result;
			ret.setTag(CVariable::NORMAL);
			return true;
		 }
	}
	if (args[0].strValue() == "runfile_tag")
	{
		if (args.size() >= 3)
		{
			const string& filename = args[1].strValue();
			string text = Tool::readfile(filename);
			return Func::run_code(text, args, ret, filename);
		}
	}
	if (args[0].strValue() == "run_tag")
	{
		if (args.size() >= 3)
		{
			const string& text = args[1].strValue();
			char buff[128];
			sprintf_s(buff, "temp%d%d", rand(), rand());
			return Func::run_code(text, args, ret, buff);
		}
	}
	if (args[0].strValue() == "url_unescape")
	{
		if (args.size() == 2)
		{
			CVariable str = args[1];
			size_t nSize = str.strValue().size();
			if (nSize>0)
			{
				string strRet = Tool::url_unescape(str.strValue());
				ret = CVariable(strRet);
				return true;
			}
		}
	}
	if (args[0].strValue() == "url_escape")
	{
		if (args.size() == 2)
		{
			CVariable str = args[1];
			size_t nSize = str.strValue().size();
			if (nSize>0)
			{
				string strRet = Tool::url_escape(str.strValue());
				ret = CVariable(strRet);
				return true;
			}
		}
	}
	if (args[0].strValue() == "time_clock")
	{
		if (args.size() == 1)
		{
			_INT t = clock();
			ret = CVariable(t);
			return true;
		}
	}
	if (args[0].strValue() == "time_now")
	{
		if (args.size() == 1)
		{
			time_t now = time(0);
			char* dt = ctime(&now);
			if (dt)
			{
				size_t dtlen = strlen(dt);
				if (dtlen > 0 && dt[dtlen - 1]=='\n')
					dt[dtlen - 1] = 0;
			}
			tm *ltm = localtime(&now);
			CVariable cvar;
			cvar.setDict();
			(*cvar.dictValue())[CVariable("time")] = CVariable((_INT)now);
			(*cvar.dictValue())[CVariable("time_str")] = CVariable(dt);
			(*cvar.dictValue())[CVariable("year")] = CVariable(1900+ltm->tm_year);
			(*cvar.dictValue())[CVariable("mon")] = CVariable(1+ltm->tm_mon);
			(*cvar.dictValue())[CVariable("day")] = CVariable(ltm->tm_mday);
			(*cvar.dictValue())[CVariable("hour")] = CVariable(ltm->tm_hour);
			(*cvar.dictValue())[CVariable("min")] = CVariable(ltm->tm_min);
			(*cvar.dictValue())[CVariable("sec")] = CVariable(ltm->tm_sec);
			ret = cvar;
			return true;
		}
	}
	if (args[0].strValue() == "time_rand")
	{
		if (args.size() == 1)
		{
			int a = rand();
			ret = CVariable(a);
			return true;
		}
	}
	if (args[0].strValue() == "time_sleep")
	{
		if (args.size() == 2)
		{
			int msec = (int)args[1].intValue();
			if (msec < 0)
				msec = 0;
			#ifdef _WIN32
				Sleep(msec);
			#else
				usleep(msec*1000);
			#endif
			return true;
		}
	}
	if (args[0].strValue() == "system")
	{
		if (args.size() == 2)
		{
			const string& text = args[1].strValue();
			system(text.c_str());
			return true;
		}
	}
	if (args[0].strValue() == "encrypt")
	{
		if (args.size() == 3)
		{
			string text = args[1].strValue();
			string key = args[2].strValue();
			if (key.size() > 0)
			{
				for (size_t i = 0; i < text.size(); i++)
					text[i] = text[i] ^ key[i % key.size()];
			}
			ret=CVariable(text);
			return true;
		}
	}
	if (args[0].strValue() == "os_platform")
	{
		if (args.size() == 1)
		{
			#ifdef _WIN32
				string platform = "WIN";
			#else
				#ifdef __linux__
					string platform = "LINUX";
				#else
					string platform = "MAC";
				#endif
			#endif
			if (sizeof(_INT) == 8)
				platform += " x64";
			ret = CVariable(platform);
			return true;
		}
	}
	if (args[0].strValue() == "curdir")
	{
		if (args.size() == 1)
		{
			if (_pInterpreter)
				ret = CVariable(_pInterpreter->get_parser().curdir());
			return true;
		}
	}
	if (args[0].strValue() == "getenv")
	{
		if (args.size() == 2)
		{
			const string& text = args[1].strValue();
			char* pBuf = getenv(text.c_str());
			if (pBuf)
				ret = CVariable(string(pBuf, strlen(pBuf)));
			return true;
		}
	}
	if (args[0].strValue() == "getstdin")
	{
		if (args.size() == 2)
		{
			int nLen = (int)args[1].intValue();
			if (nLen > 0)
			{
#ifdef _WIN32
				_setmode(_fileno(stdin), _O_BINARY);
#endif
				char* pBuf = new char[nLen + 1];
				int i = 0;
				while (i < nLen)
				{
					int x = fgetc(stdin);
					if(feof(stdin))
						break;
					pBuf[i++] = x;
				}
				pBuf[i] = 0;
				if (pBuf)
					ret = CVariable(string(pBuf, i));
				delete[]pBuf;
				return true;
			}
		}
	}
	if (args[0].strValue() == "putstdin")
	{
		if (args.size() == 2)
		{
			const string& strBuf = args[1].strValue();
			if (strBuf.size() > 0)
			{
#ifdef _WIN32
				_setmode(_fileno(stdout), _O_BINARY);
#endif
				for (size_t i = 0; i < strBuf.size(); i++)
				{
					fputc(strBuf[i], stdout);
				}
				
				return true;
			}
		}
	}
	if (args[0].strValue().substr(0, 5) == "math_")
	{
		return math_func(args, ret);
	}
	if (args[0].strValue().substr(0, 6) == "regex_")
	{
		return regex_func(args, ret);
	}
	if (args[0].strValue().substr(0, 4) == "set_")
	{
		return set_func(args, ret);
	}

	return false;
}

bool Func::run_code(const string& text, const vector<CVariable>& args, CVariable& ret, const string& filename)
{
	if (text.size() == 0) 
		return false;

	string outputFile = args[2].strValue();
	vector<CVariable> vecArgv;
	for (size_t i = 3; i < args.size(); i++)
	{
		vecArgv.push_back(args[i]);
		vecArgv[vecArgv.size() - 1].index().clear();
	}

	string code = "class CResponse() { self.content = \"\";	func write(str) { self.content = self.content + str; }	} response = CResponse(); ";
	code += " main() { args=_getargv()[0]; arrayHtml=args[\"html\"]; ";

	CVariable arrayHtml;
	arrayHtml.setArray(0);
	if (vecArgv[0].type() == CVariable::DICT)
		(*vecArgv[0].dictValue())[CVariable("html")] = arrayHtml;
	else
		return false;

	char buff[1024];
	string::size_type pos2 = 0;
	string::size_type pos1 = text.find("<?silk", pos2);
	while (pos1 != string::npos)
	{
		string strHTML = text.substr(pos2, pos1 - pos2);
		arrayHtml.arrValue()->push_back(strHTML);
		int count = Tool::str_count(strHTML, "\n");
		for (int i = 0; i < count; i++)
			code += "\r\n";
		sprintf(buff, "response.write(arrayHtml[%d]);", (int)arrayHtml.arrValue()->size() - 1);
		code += buff;
		pos2 = text.find("?>", pos1);
		if (pos2 == string::npos)
			break;
		pos1 = pos1 + 6;
		code += text.substr(pos1, pos2 - pos1);
		pos2 = pos2 + 2;
		pos1 = text.find("<?silk", pos2);
	}
	arrayHtml.arrValue()->push_back(text.substr(pos2));
	sprintf(buff, "response.write(arrayHtml[%d]);", (int)arrayHtml.arrValue()->size() - 1);
	code += buff;
	code += "return response.content;}";

	ret = Tool::interpreter(code, outputFile, vecArgv, CVariable(),filename);
	ret.setTag(CVariable::NORMAL);
	return true;
}

bool Func::math_func(const vector<CVariable>& args, CVariable& ret)
{
	if (args[0].strValue() == "math_sqrt")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == CVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == CVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = sqrt(dVal);
			ret = CVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_acos")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == CVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == CVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = acos(dVal);
			ret = CVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_asin")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == CVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == CVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = asin(dVal);
			ret = CVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_atan")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == CVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == CVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = atan(dVal);
			ret = CVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_atan2")
	{
		if (args.size() == 3)
		{
			double dVal = 0;
			if (args[1].type() == CVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == CVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			double dVal2 = 0;
			if (args[2].type() == CVariable::INT)
				dVal2 = (double)args[2].intValue();
			else if (args[2].type() == CVariable::FLOAT)
				dVal2 = args[2].floatValue();
			else
				return false;
			dVal = atan2(dVal, dVal2);
			ret = CVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_cos")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == CVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == CVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = cos(dVal);
			ret = CVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_cosh")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == CVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == CVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = cosh(dVal);
			ret = CVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_sin")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == CVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == CVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = sin(dVal);
			ret = CVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_sinh")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == CVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == CVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = sinh(dVal);
			ret = CVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_tanh")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == CVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == CVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = tanh(dVal);
			ret = CVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_exp")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == CVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == CVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = exp(dVal);
			ret = CVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_log")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == CVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == CVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = log(dVal);
			ret = CVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_frexp")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == CVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == CVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			int e;
			dVal = frexp(dVal,&e);
			ret.setArray(0);
			ret.arrValue()->push_back(CVariable().setDouble(dVal));
			ret.arrValue()->push_back(CVariable(e));
			return true;
		}
	}
	if (args[0].strValue() == "math_ldexp")
	{
		if (args.size() == 3)
		{
			double dVal = 0;
			if (args[1].type() == CVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == CVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			int nVal2 = 0;
			if (args[2].type() == CVariable::INT)
				nVal2 = (int)args[2].intValue();
			else if (args[2].type() == CVariable::FLOAT)
				nVal2 = (int)args[2].floatValue();
			else
				return false;
			dVal = ldexp(dVal, nVal2);
			ret = CVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_log10")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == CVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == CVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = log10(dVal);
			ret = CVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_modf")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == CVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == CVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			double i;
			dVal = modf(dVal, &i);
			ret.setArray(0);
			ret.arrValue()->push_back(CVariable().setDouble(i));
			ret.arrValue()->push_back(CVariable().setDouble(dVal));
			return true;
		}
	}
	if (args[0].strValue() == "math_pow")
	{
		if (args.size() == 3)
		{
			double dVal = 0;
			if (args[1].type() == CVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == CVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			double dVal2 = 0;
			if (args[2].type() == CVariable::INT)
				dVal2 = (double)args[2].intValue();
			else if (args[2].type() == CVariable::FLOAT)
				dVal2 = args[2].floatValue();
			else
				return false;
			dVal = pow(dVal, dVal2);
			ret = CVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_ceil")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == CVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == CVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = ceil(dVal);
			ret = CVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_fabs")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == CVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == CVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = fabs(dVal);
			ret = CVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_floor")
	{
		if (args.size() == 2)
		{
			double dVal = 0;
			if (args[1].type() == CVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == CVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			dVal = floor(dVal);
			ret = CVariable().setDouble(dVal);
			return true;
		}
	}
	if (args[0].strValue() == "math_fmod")
	{
		if (args.size() == 3)
		{
			double dVal = 0;
			if (args[1].type() == CVariable::INT)
				dVal = (double)args[1].intValue();
			else if (args[1].type() == CVariable::FLOAT)
				dVal = args[1].floatValue();
			else
				return false;
			double dVal2 = 0;
			if (args[2].type() == CVariable::INT)
				dVal2 = (double)args[2].intValue();
			else if (args[2].type() == CVariable::FLOAT)
				dVal2 = args[2].floatValue();
			else
				return false;
			dVal = fmod(dVal, dVal2);
			ret = CVariable().setDouble(dVal);
			return true;
		}
	}

	return false;
}

bool Func::regex_func(const vector<CVariable>& args, CVariable& ret)
{
	static map<string, regex_constants::syntax_option_type> flagMap;
	if (flagMap.size() == 0)
	{
		flagMap["icase"] = regex::icase;
		flagMap["nosubs"] = regex::nosubs;
		flagMap["optimize"] = regex::optimize;
		flagMap["collate"] = regex::collate;
		flagMap["ECMAScript"] = regex::ECMAScript;
		flagMap["basic"] = regex::basic;
		flagMap["extended"] = regex::extended;
		flagMap["awk"] = regex::awk;
		flagMap["grep"] = regex::grep;
		flagMap["egrep"] = regex::egrep;
	}

	if (args[0].strValue() == "regex_match")
	{
		if (args.size() >= 3)
		{
			const string& text = args[1].strValue();
			const string& pattern = args[2].strValue();
			string flag;
			if (args.size() > 3)
				flag = args[3].strValue();
			regex re(pattern);
			if (flag.size() > 0)
			{
				regex_constants::syntax_option_type type = regex::ECMAScript;
				vector<string> vecFlag;
				Tool::str_split(flag, "|", vecFlag);
				if (vecFlag.size() > 0)
				{
					if (flagMap.find(vecFlag[0]) != flagMap.end())
						type = flagMap[vecFlag[0]];
				}
				for (size_t i = 1; i < vecFlag.size(); i++)
				{
					if (flagMap.find(vecFlag[i]) != flagMap.end())
						type = type | flagMap[vecFlag[i]];
				}

				re = regex(pattern, type);
			}
			smatch result;
			if (regex_match(text, result, re))
			{
				string temp;
				ret.setArray(0);
				for (size_t i = 0; i < result.size(); ++i)
				{
					temp = result[i];
					ret.arrValue()->push_back(CVariable(temp));
				}
			}

			return true;
		}
	}
	if (args[0].strValue() == "regex_search")
	{
		if (args.size() >= 3)
		{
			const string& text = args[1].strValue();
			const string& pattern = args[2].strValue();
			string flag;
			if (args.size() > 3)
				flag = args[3].strValue();
			regex re(pattern);
			if (flag.size() > 0)
			{
				regex_constants::syntax_option_type type = regex::ECMAScript;
				vector<string> vecFlag;
				Tool::str_split(flag, "|", vecFlag);
				if (vecFlag.size() > 0)
				{
					if (flagMap.find(vecFlag[0]) != flagMap.end())
						type = flagMap[vecFlag[0]];
				}
				for (size_t i = 1; i < vecFlag.size(); i++)
				{
					if (flagMap.find(vecFlag[i]) != flagMap.end())
						type = type | flagMap[vecFlag[i]];
				}

				re = regex(pattern, type);
			}
			smatch result;
			string::const_iterator iterStart = text.begin();
			string::const_iterator iterEnd = text.end();
			if (regex_search(iterStart, iterEnd, result, re))
			{
				ret.setArray(0);
				string temp = result[0];
				ret.arrValue()->push_back(CVariable(temp));
			}

			return true;
		}
	}
	if (args[0].strValue() == "regex_searchall")
	{
		if (args.size() >= 3)
		{
			const string& text = args[1].strValue();
			const string& pattern = args[2].strValue();
			string flag;
			if (args.size() > 3)
				flag = args[3].strValue();
			regex re(pattern);
			if (flag.size() > 0)
			{
				regex_constants::syntax_option_type type = regex::ECMAScript;
				vector<string> vecFlag;
				Tool::str_split(flag, "|", vecFlag);
				if (vecFlag.size() > 0)
				{
					if (flagMap.find(vecFlag[0]) != flagMap.end())
						type = flagMap[vecFlag[0]];
				}
				for (size_t i = 1; i < vecFlag.size(); i++)
				{
					if (flagMap.find(vecFlag[i]) != flagMap.end())
						type = type | flagMap[vecFlag[i]];
				}

				re = regex(pattern, type);
			}
			smatch result;
			string::const_iterator iterStart = text.begin();
			string::const_iterator iterEnd = text.end();
			string temp;
			bool bret = regex_search(iterStart, iterEnd, result, re);
			if (bret)
				ret.setArray(0);
			while (bret)
			{
				temp = result[0];
				ret.arrValue()->push_back(CVariable(temp));
				iterStart = result[0].second;
				bret = regex_search(iterStart, iterEnd, result, re);
			}

			return true;
		}
	}
	if (args[0].strValue() == "regex_replace")
	{
		if (args.size() >= 4)
		{
			const string& text = args[1].strValue();
			const string& pattern = args[2].strValue();
			const string& replace = args[3].strValue();
			string flag;
			if (args.size() > 4)
				flag = args[4].strValue();
			regex re(pattern);
			if (flag.size() > 0)
			{
				regex_constants::syntax_option_type type = regex::ECMAScript;
				vector<string> vecFlag;
				Tool::str_split(flag, "|", vecFlag);
				if (vecFlag.size() > 0)
				{
					if (flagMap.find(vecFlag[0]) != flagMap.end())
						type = flagMap[vecFlag[0]];
				}
				for (size_t i = 1; i < vecFlag.size(); i++)
				{
					if (flagMap.find(vecFlag[i]) != flagMap.end())
						type = type | flagMap[vecFlag[i]];
				}

				re = regex(pattern, type);
			}
			string result = regex_replace(text, re, replace);
			ret = CVariable(result);
			return true;
		}
	}

	return false;
}

bool Func::set_func(const vector<CVariable>& args, CVariable& ret)
{
	if (args[0].strValue() == "set_create")
	{
		if (args.size() >= 1)
		{
			set<CVariable>* pSet = new set<CVariable>();
			ret.setType(CVariable::POINTER);
			ret.setPointer(pSet);
			for (size_t i = 1; i < args.size(); i++)
			{
				pSet->insert(args[i]);
			}

			return true;
		}
	}
	if (args[0].strValue() == "set_free")
	{
		if (args.size() == 2)
		{
			CVariable handle = args[1];
			if (handle.type() == CVariable::POINTER)
			{
				set<CVariable>* pSet = (set<CVariable>*)handle.pointerValue();
				delete pSet;
			}
			return true;
		}
	}
	if (args[0].strValue() == "set_size")
	{
		if (args.size() == 2)
		{
			CVariable handle = args[1];
			if (handle.type() == CVariable::POINTER)
			{
				set<CVariable>* pSet = (set<CVariable>*)handle.pointerValue();
				ret = CVariable((_INT)pSet->size());
			}
			return true;
		}
	}
	if (args[0].strValue() == "set_clear")
	{
		if (args.size() == 2)
		{
			CVariable handle = args[1];
			if (handle.type() == CVariable::POINTER)
			{
				set<CVariable>* pSet = (set<CVariable>*)handle.pointerValue();
				pSet->clear();
			}
			return true;
		}
	}
	if (args[0].strValue() == "set_insert")
	{
		if (args.size() == 3)
		{
			CVariable handle = args[1];
			if (handle.type() == CVariable::POINTER)
			{
				set<CVariable>* pSet = (set<CVariable>*)handle.pointerValue();
				pair<set<CVariable>::iterator, bool> result = pSet->insert(args[2]);
				if (result.second)
					ret = CVariable(1);
			}
			return true;
		}
	}
	if (args[0].strValue() == "set_erase")
	{
		if (args.size() == 3)
		{
			CVariable handle = args[1];
			if (handle.type() == CVariable::POINTER)
			{
				set<CVariable>* pSet = (set<CVariable>*)handle.pointerValue();
				int nArgType = 0;
				CVariable item = args[2];
				if (item.type() == CVariable::POINTER)
				{
					if (item.info().find("iterator_set") != item.info().end())
						nArgType = 1;
				}
				if (nArgType > 0)
				{
					set<CVariable>::iterator iter = *(set<CVariable>::iterator*)item.pointerValue();
					if (iter != pSet->end())
					{
						pSet->erase(iter++);
						set<CVariable>::iterator* pIter = (set<CVariable>::iterator*)item.pointerValue();
						*pIter = iter;
						ret = item;
					}
				}
				else
				{
					set<CVariable>::iterator iter = pSet->find(item);
					if (iter != pSet->end())
					{
						pSet->erase(iter);
						ret = CVariable(1);
					}
				}

			}
			return true;
		}
	}
	if (args[0].strValue() == "set_begin")
	{
		if (args.size() == 2)
		{
			CVariable handle = args[1];
			if (handle.type() == CVariable::POINTER)
			{
				set<CVariable>* pSet = (set<CVariable>*)handle.pointerValue();
				set<CVariable>::iterator iter = pSet->begin();
				if (iter != pSet->end())
				{
					CVariable vartemp;
					vartemp.setType(CVariable::POINTER);
					vartemp.info()["iterator_set"] = "1";
					set<CVariable>::iterator* pIter = new set<CVariable>::iterator;
					*pIter = iter;
					vartemp.setPointer(pIter);
					vartemp.initPointerRef(pIter);
					ret = vartemp;
				}
			}
			return true;
		}
	}
	if (args[0].strValue() == "set_next")
	{
		if (args.size() == 3)
		{
			CVariable handle = args[1];
			if (handle.type() == CVariable::POINTER)
			{
				set<CVariable>* pSet = (set<CVariable>*)handle.pointerValue();

				if (args[2].type() == CVariable::POINTER && args[2].pointerValue())
				{
					set<CVariable>::iterator iter = *(set<CVariable>::iterator*)args[2].pointerValue();
					if (iter != pSet->end())
						iter++;
					if (iter != pSet->end())
					{
						set<CVariable>::iterator* pIter = (set<CVariable>::iterator*)args[2].pointerValue();
						*pIter = iter;
						ret = args[2];
					}
				}
			}
			return true;
		}
	}
	if (args[0].strValue() == "set_get")
	{
		if (args.size() == 3)
		{
			CVariable handle = args[1];
			CVariable item = args[2];
			if (handle.type() == CVariable::POINTER && item.type() == CVariable::POINTER)
			{
				set<CVariable>* pSet = (set<CVariable>*)handle.pointerValue();
				set<CVariable>::iterator iter = *(set<CVariable>::iterator*)item.pointerValue();
				if (iter != pSet->end())
					ret = *iter;
			}
			return true;
		}
	}
	if (args[0].strValue() == "set_find")
	{
		if (args.size() == 3)
		{
			CVariable handle = args[1];
			CVariable item = args[2];
			if (handle.type() == CVariable::POINTER)
			{
				set<CVariable>* pSet = (set<CVariable>*)handle.pointerValue();
				set<CVariable>::iterator iter = pSet->find(item);
				if (iter != pSet->end())
					ret = *iter;
			}
			return true;
		}
	}
	if (args[0].strValue() == "set_union")
	{
		if (args.size() == 3)
		{
			CVariable set1 = args[1];
			CVariable set2 = args[2];
			if (set1.type() == CVariable::POINTER && set2.type() == CVariable::POINTER)
			{
				set<CVariable>* pSet1 = (set<CVariable>*)set1.pointerValue();
				set<CVariable>* pSet2 = (set<CVariable>*)set2.pointerValue();
				//ret.setArray(0);
				//set_union(pSet1->begin(), pSet1->end(), pSet2->begin(), pSet2->end(), back_inserter(*ret.arrValue()));

				set<CVariable>* pSet = new set<CVariable>();
				ret.setType(CVariable::POINTER);
				ret.setPointer(pSet);
				set_union(pSet1->begin(), pSet1->end(), pSet2->begin(), pSet2->end(), inserter(*pSet, pSet->begin()));
			}
			return true;
		}
	}
	if (args[0].strValue() == "set_intersection")
	{
		if (args.size() == 3)
		{
			CVariable set1 = args[1];
			CVariable set2 = args[2];
			if (set1.type() == CVariable::POINTER && set2.type() == CVariable::POINTER)
			{
				set<CVariable>* pSet1 = (set<CVariable>*)set1.pointerValue();
				set<CVariable>* pSet2 = (set<CVariable>*)set2.pointerValue();
				//ret.setArray(0);
				//set_intersection(pSet1->begin(), pSet1->end(), pSet2->begin(), pSet2->end(), back_inserter(*ret.arrValue()));

				set<CVariable>* pSet = new set<CVariable>();
				ret.setType(CVariable::POINTER);
				ret.setPointer(pSet);
				set_intersection(pSet1->begin(), pSet1->end(), pSet2->begin(), pSet2->end(), inserter(*pSet, pSet->begin()));
			}
			return true;
		}
	}
	if (args[0].strValue() == "set_difference")
	{
		if (args.size() == 3)
		{
			CVariable set1 = args[1];
			CVariable set2 = args[2];
			if (set1.type() == CVariable::POINTER && set2.type() == CVariable::POINTER)
			{
				set<CVariable>* pSet1 = (set<CVariable>*)set1.pointerValue();
				set<CVariable>* pSet2 = (set<CVariable>*)set2.pointerValue();
				//ret.setArray(0);
				//set_difference(pSet1->begin(), pSet1->end(), pSet2->begin(), pSet2->end(), back_inserter(*ret.arrValue()));

				set<CVariable>* pSet = new set<CVariable>();
				ret.setType(CVariable::POINTER);
				ret.setPointer(pSet);
				set_difference(pSet1->begin(), pSet1->end(), pSet2->begin(), pSet2->end(), inserter(*pSet, pSet->begin()));
			}
			return true;
		}
	}

	return false;
}