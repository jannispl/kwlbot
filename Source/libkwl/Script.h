/*
kwlbot IRC bot


File:		Script.h
Purpose:	Class which represents a script

*/

class CScript;

#ifndef _SCRIPT_H
#define _SCRIPT_H

#include "Core.h"
#include "ScriptFunctions.h"
#include "v8/v8.h"
#include <list>
#include <string>

/**
 * @brief Class which represents a script.
 */
class DLLEXPORT CScript
{
	friend class CScriptFunctions;

public:
	CScript(CBot *pParentBot);
	~CScript();

	bool Load(CCore *pCore, const char *szFilename);
	bool CallEvent(const char *szEventName, int iArgCount = 0, v8::Handle<v8::Value> *pArgValues = NULL);

	void ReportException(v8::TryCatch *pTryCatch);

	void EnterContext();
	void ExitContext();

	typedef struct
	{
		v8::Persistent<v8::FunctionTemplate> Bot;
		v8::Persistent<v8::FunctionTemplate> IrcUser;
		v8::Persistent<v8::FunctionTemplate> IrcChannel;
		v8::Persistent<v8::FunctionTemplate> Topic;
		v8::Persistent<v8::FunctionTemplate> File;
		v8::Persistent<v8::FunctionTemplate> ScriptModule;
		v8::Persistent<v8::FunctionTemplate> ScriptModuleProcedure;
	} ClassTemplates_t;
	static ClassTemplates_t m_ClassTemplates;

	bool m_bCallingEvent;
	bool m_bCurrentEventCancelled;

private:
	CBot *m_pParentBot;

	bool m_bLoaded;

#ifdef WIN32
	template class DLLEXPORT v8::Persistent<v8::Context>;
#endif
	v8::Persistent<v8::Context> m_ScriptContext;

#ifdef WIN32
	template class DLLEXPORT v8::Persistent<v8::ObjectTemplate>;
#endif
	static v8::Persistent<v8::ObjectTemplate> m_GlobalTemplate;

	typedef struct
	{
		std::string strEvent;
		v8::Persistent<v8::Function> handlerFunction;
	} EventHandler;

#ifdef WIN32
	template class DLLEXPORT CPool<EventHandler *>;
#endif
	CPool<EventHandler *> m_lstEventHandlers;
};

#endif
