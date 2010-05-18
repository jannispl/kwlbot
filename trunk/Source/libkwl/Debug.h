/*
kwlbot IRC bot


File:		Debug.h
Purpose:	Contains a crash handler used when compiling as Debug

*/

#ifdef _DEBUG

class CDebug;

#ifndef _DEBUG_H
#define _DEBUG_H

class DLLEXPORT CDebug
{
public:
	CDebug();
	~CDebug();

	void SetLastFunction(const char *szFunction);
	const char *GetLastFunction();
	static long ExceptionHandler(_EXCEPTION_POINTERS exceptionPointers);

private:
	char m_szLastFunction[64];
};

#endif

#endif