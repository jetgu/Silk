#pragma once
#include "Variable.h"
#include "AutoLock.h"
#include <string>
#include <map>

using namespace std;

#ifdef _WIN32
	#include<windows.h>
#else
	#include<dlfcn.h>
	#include <cstring>
	#define sprintf_s sprintf
#endif


class Tool
{
public:
	static string mysprintf(string& format, vector<CVariable>& vecArgs);
	static string readfile(const string& strFilename);
	static string url_escape(const string& URL);
	static string url_unescape(const string& URL);
	static int str_count(const string& text, const string& str);
	static void str_replace(string& text, const string& str_old, const string& str_new);
	static void str_split(const string& str, const string& splitstr, vector<string>& vecStr);
	static CVariable interpreter(const string& code, const string& outputfile, const vector<CVariable>& vecArgv, const CVariable& globalValue, string filename);
};

class File
{
public:
	void* open(const char *filename, const char *mode);
	void close(void* handle);
	bool read(void* handle, _INT size, CVariable& result);
	bool write(void* handle, CVariable& content);
	bool seek(void* handle, _INT pos);
	_INT size(void* handle);
};

class Dll
{
public:
	void* loadlib(const string& filename);
	void freelib(void* handle);
	bool calllib(const vector<CVariable>& args, CVariable& arrResult);
};

class Lock
{
public:
	Lock();
	~Lock();
	void* create_lock();
	void lock(void* handle);
	void unlock(void* handle);
private:
	vector<CLock*> m_vecLocks;
	CLock m_lock;
};

class Lib
{
public:
	enum LIBMEMBER{ SUBSTR, FIND, REPLACE, SIZE, RESIZE, APPEND, BEGIN, END, RBEGIN, REND, NEXT, GET, ERASE, INSERT, CLEAR, SPLIT, GETPTR, RESTORE, 
		RFIND, TRIM, LTRIM, RTRIM, LOWER, UPPER, SORT, SWAP, CREATE2D, CREATE3D };
	Lib(){}
	~Lib(){}
	virtual bool call_member(const string& name, CVariable& var, const vector<CVariable>& args, CVariable& ret){ return false; }
	void error(string err){ m_err = err; }
	string err_msg(){ return m_err; }
	static bool compare(const CVariable& a, const CVariable& b);
private:
	string m_err;
protected:
	map<string, LIBMEMBER> m_mapMembers;
};

class Lib_String: public Lib
{
public:
	Lib_String();
	~Lib_String();
	bool call_member(const string& name, CVariable& var, const vector<CVariable>& args, CVariable& ret);
};

class Lib_Array : public Lib
{
public:
	Lib_Array();
	~Lib_Array();
	bool call_member(const string& name, CVariable& var, vector<CVariable>& args, CVariable& ret);
};

class Lib_Dict : public Lib
{
public:
	Lib_Dict();
	~Lib_Dict();
	bool call_member(const string& name, CVariable& var, const vector<CVariable>& args, CVariable& ret);
};

class Lib_Class : public Lib
{
public:
	Lib_Class();
	~Lib_Class();
	bool call_member(const string& name, CVariable& var, const vector<CVariable>& args, CVariable& ret);
};

class Func
{
public:
	static bool call_func(const vector<CVariable>& args, CVariable& ret);

	static bool run_code(const string& code, const vector<CVariable>& args, CVariable& ret, const string& filename);
	static bool math_func(const vector<CVariable>& args, CVariable& ret);
	static bool regex_func(const vector<CVariable>& args, CVariable& ret);
	static bool set_func(const vector<CVariable>& args, CVariable& ret);
};
