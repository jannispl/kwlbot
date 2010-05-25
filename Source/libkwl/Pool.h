/*
kwlbot IRC bot


File:		Pool.h
Purpose:	Template class which represents a pool for elements

*/

#include <list>

#ifndef _POOL_H
#define _POOL_H

#include <malloc.h>

template <typename T> class CPool
{
public:
	class iterator
	{
	public:
		iterator()
			: m_pCurr(NULL)
		{
		}

		iterator(T *it)
			: m_pCurr(it)
		{
		}

		inline T *ptr() const
		{
			return m_pCurr;
		}

		inline void operator ++()
		{
			++m_pCurr;
		}

		T operator *() const
		{
			return *m_pCurr;
		}

		bool operator !=(const iterator &it)
		{
			return m_pCurr != it.ptr();
		}

		bool operator ==(const iterator &it)
		{
			return m_pCurr == it.ptr();
		}

	private:
		T *m_pCurr;
	};

	CPool()
		: m_pElements(NULL), m_iNumElements(0)
	{
	}

	~CPool()
	{
		if (m_pElements != NULL)
		{
			free(m_pElements);
		}
	}

	void push_back(T elem)
	{
		alloc_element();
		m_pElements[m_iNumElements] = elem;
		++m_iNumElements;
	}

	iterator begin() const
	{
		return iterator(m_pElements);
	}

	iterator end() const
	{
		return iterator(m_pElements + m_iNumElements);
	}

	inline size_t size() const
	{
		return m_iNumElements;
	}

	T front() const
	{
		return *m_pElements;
	}

	T back()
	{
		return *(T *)(end().ptr() - 1);
	}

	iterator erase(iterator it)
	{
		if (m_iNumElements == 0)
		{
			return end();
		}

		--m_iNumElements;
		if (m_iNumElements != 0)
		{
			T *last = (T *)end().ptr();
			if (it.ptr() != last)
			{
				memcpy(it.ptr(), last, sizeof(T));
			}

			m_pElements = (T *)realloc(m_pElements, m_iNumElements * sizeof(T));

			return iterator(last);
		}
		else
		{
			free(m_pElements);
			m_pElements = NULL;
		}
		return end();
	}

	bool remove(T elem)
	{
		for (iterator i = begin(); i != end(); ++i)
		{
			if (*i == elem)
			{
				erase(i);
				return true;
			}
		}
		return false;
	}

	void clear()
	{
		if (m_pElements != NULL)
		{
			free(m_pElements);
			m_pElements = 0;
			m_iNumElements = 0;
		}
	}

private:
	size_t m_iNumElements;
	T *m_pElements;

	void alloc_element()
	{
		if (m_pElements == NULL)
		{
			m_pElements = (T *)malloc(sizeof(T));
		}
		else
		{
			m_pElements = (T *)realloc(m_pElements, (m_iNumElements + 1) * sizeof(T));
		}
	}
};

#endif
