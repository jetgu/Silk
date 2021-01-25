#include "CallStack.h"
#include "Token.h"

static CVariable _emptyVar = CVariable();

ActivationRecord::ActivationRecord()
{

}
ActivationRecord::ActivationRecord(string name, string type, int level)
{
	m_name = name;
	m_type = type;
	m_level = level;
}
void ActivationRecord::set_value(const string& key, const CVariable& value)
{ 
	m_members[key] = value; 
}
CVariable& ActivationRecord::get_value(const string& key)
{
	if (m_members.find(key) != m_members.end())
		return m_members[key];
    return _emptyVar;
}
void ActivationRecord::set_array_value(const string& key, const CVariable& value, _INT arr_index)
{
	if (arr_index >= 0)
	{
		if (m_members.find(key) != m_members.end())
		{
			if (m_members[key].type() == CVariable::ARRAY)
			{
				vector<CVariable>* arr = m_members[key].arrValue();
				if (arr == NULL)
					return;
				if (arr_index >= 0 && (size_t)arr_index < arr->size())
					(*arr)[arr_index] = value;
			}
			else if (m_members[key].type() == CVariable::STRING)
			{
				string& str = m_members[key].strValue();
				if (arr_index >= 0 && (size_t)arr_index < str.size())
				{
					if (value.type() == CVariable::STRING && value.strValue().size() == 1)
						str[arr_index] = value.strValue()[0];
				}
			}
		}
	}
}
CVariable& ActivationRecord::get_array_value(const string& key, _INT arr_index)
{
	if (arr_index >= 0)
	{
		if (m_members.find(key) != m_members.end())
		{
			if (m_members[key].type() == CVariable::STRING)
			{
				const string& strValue = m_members[key].strValue();
				if (arr_index >= 0 && (size_t)arr_index < strValue.size())
				{
					static CVariable charVar = CVariable();
					charVar = CVariable(string(1, strValue[arr_index]));
					return charVar;
				}
			}
			else if (m_members[key].type() == CVariable::ARRAY)
			{
				vector<CVariable>* arr = m_members[key].arrValue();
				if (arr == NULL)
					return _emptyVar;
				if (arr_index >= 0 && (size_t)arr_index < arr->size())
					return (*arr)[arr_index];
			}
		}
	}

	return _emptyVar;
}
CVariable::VARTYPE ActivationRecord::get_vartype(const string& key)
{
	if (m_members.find(key) != m_members.end())
		return m_members[key].type();
	return CVariable::EMPTY;
}

void ActivationRecord::set_dict_value(const string& key, const CVariable& value, const CVariable& dict_index)
{
	if (m_members.find(key) != m_members.end())
	{
		map<CVariable, CVariable>* dict = m_members[key].dictValue();
		if (dict == NULL)
			return;
		(*dict)[dict_index] = value;
	}
}
CVariable& ActivationRecord::get_dict_value(const string& key, const CVariable& dict_index)
{
	if (m_members.find(key) != m_members.end())
	{
		map<CVariable, CVariable>* dict = m_members[key].dictValue();
		if (dict == NULL)
			return _emptyVar;
		if (dict->find(dict_index) != dict->end())
			return (*dict)[dict_index];
	}

	return _emptyVar;
}
void ActivationRecord::create_global(const CVariable& globalValue, const vector<CVariable>& vecArgv, const string& argvName)
{
	CVariable global_dict;
	global_dict.setDict();
	set_value(GLOBAL_DICT_NAME, global_dict);
	if (globalValue.type() == CVariable::DICT)
		set_value(GLOBAL_DICT_NAME, globalValue);

	CVariable argv_array;
	argv_array.setArray(0);
	for (size_t i = 0; i < vecArgv.size(); i++)
		argv_array.arrValue()->push_back(vecArgv[i]);
	if (argvName.size()>0)
		set_global_value(argvName, argv_array);
	else
		set_global_value(ARGV_ARRAY_NAME, argv_array);
}
CVariable& ActivationRecord::get__global_value(const string& var_name)
{
	CVariable& global_dict = get_value(GLOBAL_DICT_NAME);
	if (global_dict.type() == CVariable::DICT)
	{
		if (global_dict.dictValue()->find(var_name) != global_dict.dictValue()->end())
			return (*global_dict.dictValue())[var_name];
	}

	return _emptyVar;
}
void ActivationRecord::set_global_value(const string& var_name, const CVariable& value)
{
	set_dict_value(GLOBAL_DICT_NAME, value, var_name);
}
CVariable::VARTYPE ActivationRecord::get_global_vartype(const string& var_name)
{
	CVariable& global_value = get__global_value(var_name);
	return global_value.type();
}


CCallStack::CCallStack()
{
}


CCallStack::~CCallStack()
{
}

void CCallStack::pop()
{ 
	if (m_stack.size() == 0)
		return;

	m_stack.pop_back(); 
}
void CCallStack::push(const ActivationRecord& ar)
{ 
	m_stack.push_back(ar); 
}
ActivationRecord& CCallStack::peek()
{
	static ActivationRecord emptyAR;
	if (m_stack.size() == 0)
		return emptyAR;

	return m_stack[m_stack.size() - 1];
}
ActivationRecord& CCallStack::base()
{
	static ActivationRecord emptyAR;
	if (m_stack.size() == 0)
		return emptyAR;

	return m_stack[0];
}
