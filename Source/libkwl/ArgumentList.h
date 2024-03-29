/*
kwlbot IRC bot


File:		ArgumentList.h
Purpose:	Utility class for a list of arguments

*/

#ifndef _ARGUMENTLIST_H
#define _ARGUMENTLIST_H

#include <vector>

/**
 * @brief Utility class for a list of arguments.
 */
class CArgumentList
{
public:
	union ArgumentValue
	{
		int iValue;
		float fValue;
		bool bValue;
		void *pPointer;
	};

	enum ArgumentType
	{
		Dummy,
		Integer,
		Float,
		Boolean,
		Pointer
	};

	typedef struct
	{
		char cType;
		ArgumentValue argValue;

		ArgumentType GetType() const
		{
			return (ArgumentType)cType;
		}
	} Argument;

	void Add(int iValue)
	{
		Argument arg;
		arg.cType = 1;
		arg.argValue.iValue = iValue;
		m_vecArguments.push_back(arg);
	}

	void Add(float fValue)
	{
		Argument arg;
		arg.cType = 2;
		arg.argValue.fValue = fValue;
		m_vecArguments.push_back(arg);
	}

	void Add(bool bValue)
	{
		Argument arg;
		arg.cType = 3;
		arg.argValue.bValue = bValue;
		m_vecArguments.push_back(arg);
	}

	void Add(void *pPointer)
	{
		Argument arg;
		arg.cType = 4;
		arg.argValue.pPointer = pPointer;
		m_vecArguments.push_back(arg);
	}

	const Argument &operator [](size_t iIndex) const
	{
		return m_vecArguments[iIndex];
	}

	size_t Size() const
	{
		return m_vecArguments.size();
	}

private:
	std::vector<Argument> m_vecArguments;
};

#endif
