/*
kwlbot IRC bot


File:		Pool.h
Purpose:	Template class which represents a pool for elements

*/

#include <list>

#ifndef _POOL_H
#define _POOL_H

#include <malloc.h>

/**
 * @brief Template class which represents a pool for elements.
 */
template <typename T> class CPool
{
public:
	typedef struct
	{
		void *pPrevious;
		T This;
		void *pNext;
	} _elem;

	class iterator
	{
	public:
		iterator()
			: m_pCurr(NULL)
		{
		}

		iterator(_elem *it)
			: m_pCurr(it)
		{
		}

		inline _elem *ptr() const
		{
			return m_pCurr;
		}

		inline void operator ++()
		{
			m_pCurr = (_elem *)m_pCurr->pNext;
		}

		T operator *() const
		{
			return m_pCurr->This;
		}

		bool last()
		{
			return m_pCurr->pNext == NULL;
		}

		bool operator !=(const iterator &it)
		{
			return m_pCurr != it.ptr();
		}

		bool operator ==(const iterator &it)
		{
			return m_pCurr == it.ptr();
		}

		void assign(T elem)
		{
			m_pCurr->This = elem;
		}

	private:
		_elem *m_pCurr;
	};

	CPool()
		: m_pFirst(NULL), m_pLast(NULL), m_iNumElements(0)
	{
	}

	~CPool()
	{
		clear();
	}

	void push_front(T elem)
	{
		_elem *pElem = new _elem;
		pElem->This = elem;

		if (m_iNumElements == 0)
		{
			m_pLast = new _elem;
			m_pLast->pPrevious = pElem;
			m_pLast->pNext = NULL;
		}
		else
		{
			m_pFirst->pPrevious = pElem;
		}

		pElem->pPrevious = NULL;
		pElem->pNext = m_pFirst != NULL ? m_pFirst : m_pLast;
		m_pFirst = pElem;

		++m_iNumElements;
	}

	void push_back(T elem)
	{
		_elem *pElem = new _elem;
		pElem->This = elem;

		if (m_iNumElements == 0)
		{
			m_pLast = new _elem;
			m_pLast->pNext = NULL;

			pElem->pPrevious = NULL;
			pElem->pNext = m_pLast;
			m_pLast->pPrevious = pElem;
			m_pFirst = pElem;
		}
		else
		{
			pElem->pPrevious = m_pLast->pPrevious;
			m_pLast->pPrevious = pElem;
			((_elem *)pElem->pPrevious)->pNext = pElem;
			pElem->pNext = m_pLast;
		}

		++m_iNumElements;
	}

	iterator begin() const
	{
		if (m_pFirst == NULL)
		{
			return end();
		}
		return iterator(m_pFirst);
	}

	iterator end() const
	{
		return iterator(m_pLast);
	}

	inline size_t size() const
	{
		return m_iNumElements;
	}

	T front() const
	{
		return m_pFirst->This;
	}

	T back() const
	{
		return ((_elem *)m_pLast->pPrevious)->This;
	}

	iterator erase(iterator it)
	{
		if (m_iNumElements == 0)
		{
			return end();
		}

		--m_iNumElements;
		_elem *pToDelete = (_elem *)it.ptr();
		_elem *pPrevious = (_elem *)pToDelete->pPrevious;

		if (pPrevious != NULL)
		{
			pPrevious->pNext = pToDelete->pNext;
		}
		((_elem *)pToDelete->pNext)->pPrevious = pPrevious;

		if (pToDelete == m_pFirst)
		{
			m_pFirst = (_elem *)pToDelete->pNext;
		}

		delete pToDelete;

		if (m_iNumElements == 0 || pPrevious == NULL)
		{
			//m_pFirst = NULL;
			return end();
		}
		return iterator(pPrevious);
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
		_elem *tmp = m_pLast;
		while (tmp != NULL)
		{
			_elem *pNext = (_elem *)tmp->pNext;
			delete tmp;
			tmp = pNext;
		}
	}

private:
	size_t m_iNumElements;
	_elem *m_pFirst;
	_elem *m_pLast;
};

#endif
