/*
kwlbot IRC bot


File:		GlobalModule.h
Purpose:	Class which represents a global module

*/

class CGlobalModule;

#ifndef _GLOBALMODULE_H
#define _GLOBALMODULE_H

#include "Core.h"
#include "v8/v8.h"

/**
 * @brief Class which represents a global module.
 */
class DLLEXPORT CGlobalModule
{
public:
	typedef bool (* InitModule_t)(CCore *pCore, char *szName);
	typedef void (* ExitModule_t)();
	typedef void (* TemplateRequest_t)(v8::Handle<v8::ObjectTemplate> &objectTemplate);
	typedef void (* Pulse_t)();

	CGlobalModule(CCore *pCore, const char *szPath);
	~CGlobalModule();

	void TemplateRequest(v8::Handle<v8::ObjectTemplate> &objectTemplate);
	void Pulse();

private:
	struct
	{
		InitModule_t pfnInitModule;
		ExitModule_t pfnExitModule;
		TemplateRequest_t pfnTemplateRequest;
		Pulse_t pfnPulse;
	} m_Functions;

#ifdef WIN32
	HMODULE m_pLibrary;
#else
	void *m_pLibrary;
#endif
};

#endif
