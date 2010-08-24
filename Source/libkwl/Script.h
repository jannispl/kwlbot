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
#include "TimerManager.h"
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
	void Pulse();

	typedef struct
	{
		v8::Persistent<v8::FunctionTemplate> Bot;
		v8::Persistent<v8::FunctionTemplate> IrcUser;
		v8::Persistent<v8::FunctionTemplate> IrcChannel;
		v8::Persistent<v8::FunctionTemplate> Topic;
		v8::Persistent<v8::FunctionTemplate> File;
		v8::Persistent<v8::FunctionTemplate> ScriptModule;
		v8::Persistent<v8::FunctionTemplate> ScriptModuleProcedure;
		v8::Persistent<v8::FunctionTemplate> Timer;
	} ClassTemplates_t;
	static ClassTemplates_t m_classTemplates;

private:
	typedef struct
	{
		std::string strEvent;
		v8::Persistent<v8::Function> handlerFunction;
	} EventHandler;

	CBot *m_pParentBot;
	bool m_bLoaded;
	bool m_bCallingEvent;
	bool m_bCurrentEventCancelled;

#ifdef WIN32
	template class DLLEXPORT v8::Persistent<v8::Context>;
	template class DLLEXPORT v8::Persistent<v8::ObjectTemplate>;
	template class DLLEXPORT CPool<EventHandler *>;
#endif

	v8::Persistent<v8::Context> m_scriptContext;
	static v8::Persistent<v8::ObjectTemplate> m_globalTemplate;
	CPool<EventHandler *> m_plEventHandlers;

	CTimerManager *m_pTimerManager;
};

#endif
