#pragma once
#include <vector>
#include <map>
#include "Variable.h"

using namespace std;

class ActivationRecord
{
public:
	ActivationRecord();
	ActivationRecord(string name,string type,int level);
	string name() const{ return m_name; }
	string type() const{ return m_type; }
	void set_value(const string& key, const CVariable& value);
	CVariable& get_value(const string& key);
	void set_array_value(const string& key, const CVariable& value, _INT arr_index);
	CVariable& get_array_value(const string& key, _INT arr_index);
	void set_dict_value(const string& key, const CVariable& value, const CVariable& dict_index);
	CVariable& get_dict_value(const string& key, const CVariable& dict_index);
	CVariable::VARTYPE get_vartype(const string& key);
	void create_global(const CVariable& globalValue, const vector<CVariable>& vecArgv, const string& argvName);
	CVariable& get__global_value(const string& var_name);
	void set_global_value(const string& var_name, const CVariable& value);
	CVariable::VARTYPE get_global_vartype(const string& var_name);
private:
	map<string, CVariable> m_members;
	string m_name;
	string m_type;
	int m_level;
};

class CCallStack
{
public:
	CCallStack();
	~CCallStack();
	void pop();
	ActivationRecord& peek();
	ActivationRecord& base();
	void push(const ActivationRecord& ar);
private:
	vector<ActivationRecord> m_stack;
};

