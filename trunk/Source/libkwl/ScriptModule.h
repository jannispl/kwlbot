/*
kwlbot IRC bot


File:		ScriptModule.h
Purpose:	Class which represents a script module

*/

class CScriptModule;

#ifndef _SCRIPTMODULE_H
#define _SCRIPTMODULE_H

#include "ScriptObject.h"
#include "ArgumentList.h"

class CScriptModule : public CScriptObject
{
public:
	typedef bool (* InitModule_t)(char *szName);
	typedef void (* ExitModule_t)();
	typedef int (* Procedure_t)(const CArgumentList &args);

	class Procedure : public CScriptObject
	{
	public:
		Procedure(CScriptModule *pScriptModule, const char *szName);
		~Procedure();

		int Call(const CArgumentList &args);

		CScriptObject::eScriptType GetType();

	private:
		Procedure_t m_pfnPointer;
	};

	friend class Procedure;

	CScriptModule(const char *szPath);
	~CScriptModule();

	Procedure *AllocateProcedure(const char *szName);

	CScriptObject::eScriptType GetType();

private:
	struct
	{
		InitModule_t pfnInitModule;
		ExitModule_t pfnExitModule;
	} m_Functions;

#ifdef WIN32
	HMODULE m_pLibrary;
	HMODULE GetLibrary();
#else
	void *m_pLibrary;
	void *GetLibrary();
#endif

};

#endif
