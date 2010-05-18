/*
kwlbot IRC bot


File:		Debug.cpp
Purpose:	Contains a crash handler used when compiling as Debug

*/

#include "StdInc.h"

#ifdef _DEBUG

#include "Debug.h"

CDebug::CDebug()
{
	SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)CDebug::ExceptionHandler);
}

CDebug::~CDebug()
{
}

long CDebug::ExceptionHandler(_EXCEPTION_POINTERS exceptionPointers)
{	
	PCONTEXT pContext = exceptionPointers.ContextRecord;
	/*char tmp[512];
	sprintf(tmp, "EAX 0x%p   EBP 0x%p   EBX 0x%p   ECX 0x%p\n"
				 "EDI 0x%p   EDX 0x%p   EIP 0x%p",
		pContext->Eax,
		pContext->Ebp,
		pContext->Ebx,
		pContext->Ecx,
		pContext->Edi,
		pContext->Edx,
		pContext->Eip);*/

	char tmp[512];
	sprintf(tmp, "Exception at 0x%p (0x%p)\n\nLast function called: %s",
		exceptionPointers.ExceptionRecord->ExceptionAddress,
		exceptionPointers.ExceptionRecord->ExceptionCode,
		g_pDebug->GetLastFunction());

	MessageBoxA(NULL, tmp, "An exception has occurred!", MB_OK | MB_ICONERROR);

	return 1;
}

void CDebug::SetLastFunction(const char *szFunction)
{
	strcpy(m_szLastFunction, szFunction);
}

const char *CDebug::GetLastFunction()
{
	return m_szLastFunction;
}

#endif
