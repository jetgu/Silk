#include "GlobalData.h"


CGlobalData::CGlobalData()
{
}


CGlobalData::~CGlobalData()
{
	clear_all_nodes();
	clear_globals();
}

void CGlobalData::clear_all_nodes()
{
	for (size_t i = 0; i < m_vecNodes.size(); i++)
		delete m_vecNodes[i];
	m_vecNodes.clear();
}
void CGlobalData::clear_globals()
{
	m_functions.clear();
	m_globals.clear();
}

void CCheckStack::push()
{
	CGlobalCheck check;
	m_stack.push_back(check);
}
void CCheckStack::pop()
{
	if (m_stack.size() == 0)
		return;
	m_stack.pop_back();
}
CGlobalCheck& CCheckStack::top()
{
	static CGlobalCheck emptyRes;
	if (m_stack.size() == 0)
		return emptyRes;

	return m_stack[m_stack.size() - 1];

}
void CCheckStack::add_node(AST* node)
{
	if (m_stack.size() == 0) return;
	m_stack[m_stack.size() - 1].nodes().push_back(node);
}
void CCheckStack::add_param(string name)
{
	if (m_stack.size() == 0) return;
	m_stack[m_stack.size() - 1].params()[name] = true;
}
void CCheckStack::add_assign(string name)
{
	if (m_stack.size() == 0) return;
	m_stack[m_stack.size() - 1].assigns()[name] = true;
}
