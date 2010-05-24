/*
kwlbot IRC bot


File:		ScriptModule.cpp
Purpose:	Class which represents a script module

*/

#include "StdInc.h"
#include "ScriptModule.h"

#ifdef WIN32
  #define GetLibraryProc(pLibrary, szProc) GetProcAddress(pLibrary, szProc)
#else
#endif

CScriptModule::CScriptModule(const char *szPath)
{
	char *szFullPath = new char[strlen(szPath) + 9];
	strcpy(szFullPath, "modules/");
	strcat(szFullPath, szPath);

#ifdef WIN32
	m_pLibrary = LoadLibrary(szFullPath);
#else
#endif

	delete[] szFullPath;

	if (m_pLibrary == NULL)
	{
		memset(&m_Functions, 0, sizeof(m_Functions));
		return;
	}

	m_Functions.pfnInitModule = (InitModule_t)GetLibraryProc(m_pLibrary, "InitModule");
	m_Functions.pfnExitModule = (ExitModule_t)GetLibraryProc(m_pLibrary, "ExitModule");

	if (m_Functions.pfnInitModule != NULL)
	{
		char szName[128] = "Unknown";
		if (!m_Functions.pfnInitModule(szName))
		{
			printf("Failed to load script module '%s'\n", szPath);
			FreeLibrary(m_pLibrary);
			m_pLibrary = NULL;
			return;
		}
	}
	else
	{
		printf("Warning: Script module '%s' does not have a \"InitModule\" export!\n", szPath);
	}
}

CScriptModule::~CScriptModule()
{
	if (m_pLibrary != NULL)
	{
		if (m_Functions.pfnExitModule != NULL)
		{
			m_Functions.pfnExitModule();
		}

#ifdef WIN32
		FreeLibrary(m_pLibrary);
#else
#endif
	}
}

CScriptModule::Procedure *CScriptModule::AllocateProcedure(const char *szName)
{
	return new Procedure(this, szName);
}

CScriptObject::eScriptType CScriptModule::GetType()
{
	return ScriptModule;
}

#ifdef WIN32
HMODULE CScriptModule::GetLibrary()
#else
void *CScriptModule::GetLibrary()
#endif
{
	return m_pLibrary;
}

CScriptModule::Procedure::Procedure(CScriptModule *pScriptModule, const char *szName)
{
	m_pfnPointer = (Procedure_t)GetLibraryProc(pScriptModule->GetLibrary(), szName);
}

CScriptModule::Procedure::~Procedure()
{
}

int CScriptModule::Procedure::Call(const CArgumentList &args)
{
	if (m_pfnPointer == NULL)
	{
		return -1;
	}

	return m_pfnPointer(args);
}

CScriptObject::eScriptType CScriptModule::Procedure::GetType()
{
	return ScriptModuleProcedure;
}
