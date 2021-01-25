#pragma once
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>

#ifdef _WIN32
	#ifdef _WIN64
		#define _INT __int64
		#define _INTFMT "%lld "
		#define _ATOI(val)	_atoi64(val)
	#else
		#define _INT int
		#define _INTFMT "%d "
		#define _ATOI(val)	atoi(val)
	#endif
#else
	#ifdef __x86_64__
		#define _INT long
		#define _INTFMT "%lld "
	#elif __i386__
		#define _INT int
		#define _INTFMT "%d "
	#endif
	#define _ATOI(val)	strtoll(val, NULL, 10)
#endif

using namespace std;

//will get better performance by using vector if there is a little data
class CVecMap
{
public:
	void clear(){ m_vecInfo.clear(); }
	int size(){ return (int)m_vecInfo.size(); }
	int end(){ return -1; }
	int find(const string& key){
		size_t i = 0;
		for (; i < m_vecInfo.size(); i++)
		{
			if (m_vecInfo[i].first == key)
				break;
		}
		if (i < m_vecInfo.size())
			return (int)i;

		return -1;
	}
	string& operator [](const string& key){
		int i = find(key);
		if (i > -1)
		{
			return m_vecInfo[i].second;
		}
		else
		{
			pair<string, string> p(key, "");
			m_vecInfo.push_back(p);
			return m_vecInfo[m_vecInfo.size() - 1].second;
		}
	}
	vector<pair<string, string>>& map(){ return m_vecInfo; }
private:
	vector<pair<string, string>> m_vecInfo;
};

class CVariable
{
public:
	enum VARTYPE{ EMPTY, NONE, STRING, INT, FLOAT, ARRAY, DICT, POINTER, CLASS };
	enum TAGTYPE{ NORMAL, BREAK, RETURN, CONTINUE, ERR };
	CVariable();
	CVariable(const CVariable& cv);
	CVariable& operator =(const CVariable& cv);
	CVariable(const string& value);
	CVariable(_INT value);
	CVariable& setDouble(double value);
	CVariable& setError();
	~CVariable();

	void reset();
	void setTag(TAGTYPE tag){ m_tag = tag; }
	TAGTYPE tag() const { return m_tag; }
	void setType(VARTYPE type){ m_type = type; }
	VARTYPE type() const { return m_type; }
	_INT intValue() const { return m_nValue; }
	void setInt(_INT value){ m_type = INT; m_nValue = value; }
	string& strValue() { return m_strValue; }
	const string& strValue() const { return m_strValue; }
	double floatValue() const { return m_fValue; }
	vector<CVariable>* arrValue(){ return m_pVecValue; }
	void setArray(_INT arrSize, vector<CVariable>* pArray = NULL);
	map<CVariable, CVariable>* dictValue(){ return m_pMapValue; }
	void setDict(map<CVariable, CVariable>* pDict=NULL);
	CVecMap& info(){ return m_mapInfo; }
	void setInfo(CVecMap& info);
	vector<CVariable>& index(){ return m_vecIndex; }
	void setIndex(const vector<CVariable>& index){ m_vecIndex = index; }
	void* pointerValue() const { return m_pPointer; }
	void setPointer(void* pPointer){ m_pPointer = pPointer; }
	void initPointerRef(void* pPointer);

	CVariable operator +(const CVariable & right);
	CVariable operator -(const CVariable & right);
	CVariable operator *(const CVariable & right);
	CVariable operator /(const CVariable & right);
	CVariable operator %(const CVariable & right);
	CVariable operator ==(const CVariable & right);
	CVariable operator !=(const CVariable & right);
	CVariable operator >=(const CVariable & right);
	CVariable operator <=(const CVariable & right);
	CVariable operator >(const CVariable & right);
	CVariable operator <(const CVariable & right);
	CVariable operator &&(const CVariable & right);
	CVariable operator ||(const CVariable & right);
	bool operator<(const CVariable& right) const;

private:
	void IncreaseRefCount();
	void DecreaseRefCount();
	static map<vector<CVariable>*, int> _mapArraysRefCount;
	static map<map<CVariable, CVariable>*, int> _mapDictsRefCount;
	static map<void*, int> _mapPointersRefCount;

private:
	TAGTYPE m_tag;
	VARTYPE m_type;
	string m_strValue;
	_INT m_nValue;
	double m_fValue;
	void* m_pPointer;
	vector<CVariable>*  m_pVecValue;
	map<CVariable, CVariable>* m_pMapValue;
	vector<CVariable>  m_vecIndex;
	CVecMap m_mapInfo;

};
