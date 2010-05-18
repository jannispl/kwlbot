/*
kwlbot IRC bot


File:		Script.h
Purpose:	Class which represents a script

*/

class CScript;

#ifndef _SCRIPT_H
#define _SCRIPT_H

#include "Core.h"
#include "v8/v8.h"
#include <list>
#include <string>

class DLLEXPORT CScript
{
public:
	CScript(CCore *pParentCore);
	~CScript();

	bool Load(const char *szFilename);
	bool CallEvent(const char *szEventName, int iArgCount = 0, v8::Handle<v8::Value> *pArgValues = NULL);

	void EnterContext();
	void ExitContext();

	typedef struct
	{
		std::string strEvent;
		v8::Persistent<v8::Function> handlerFunction;
	} EventHandler;
	std::list<EventHandler> m_lstEventHandlers;

	typedef struct
	{
		v8::Persistent<v8::FunctionTemplate> Bot;
		v8::Persistent<v8::FunctionTemplate> IrcUser;
		v8::Persistent<v8::FunctionTemplate> IrcChannel;
		v8::Persistent<v8::FunctionTemplate> File;
	} ClassTemplates_t;
	static ClassTemplates_t m_ClassTemplates;

	bool m_bCallingEvent;
	bool m_bCurrentEventCancelled;

private:
	CCore *m_pParentCore;

	bool m_bLoaded;

	template class DLLEXPORT v8::Persistent<v8::Context>;
	v8::Persistent<v8::Context> m_ScriptContext;

	static v8::Persistent<v8::ObjectTemplate> m_GlobalTemplate;
};

#endif
