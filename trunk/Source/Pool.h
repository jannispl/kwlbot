/*
kwlbot IRC bot


File:		Pool.h
Purpose:	Template class which represents a pool for elements

*/

#include <list>
#define CPool std::list

/*
template <typename T> class CPool;

#ifndef _POOL_H
#define _POOL_H

#include <vector>

template <typename T>
class CPool
{
public:
	typedef unsigned int index_t;
	typedef T elem_t;

	index_t Add(const elem_t &Elem)
	{
		m_lstElements.push_back(Elem);
		return m_lstElements.size() - 1;
	}

	const T& Get(index_t iIndex)
	{
		return m_lstElements[iIndex];
	}

	const T& operator[](index_t iIndex)
	{
		return Get(iIndex);
	}

	void Remove(index_t iIndex)
	{
		m_lstElements.erase(m_lstElements.begin() + iIndex);
	}

	bool Remove(const elem_t &Elem)
	{
		for (std::vector<T>::iterator i = m_lstElements.begin(); i != m_lstElements.end(); ++i)
		{
			if (*i == Elem)
			{
				m_lstElements.erase(i);
				return true;
			}
		}
		return false;
	}

	index_t GetSize()
	{
		return m_lstElements.size();
	}

private:
	std::vector<T> m_lstElements;
};

#endif
*/
