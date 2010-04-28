/*
kwlbot IRC bot


File:		Script.h
Purpose:	Class which represents a script

*/

class CScript;

#ifndef _SCRIPT_H
#define _SCRIPT_H

#include "v8/v8.h"
#include <map>
#include <string>

class CScript
{
public:
	CScript();
	~CScript();

	bool Load(const char *szFilename);
	v8::Handle<v8::Value> CallEvent(const char *szEventName, int iArgCount = 0, v8::Handle<v8::Value> *pArgValues = NULL);

	void EnterContext();
	void ExitContext();

	std::map<std::string, v8::Persistent<v8::Function>> m_mapEventFunctions;

	typedef struct
	{
		v8::Persistent<v8::FunctionTemplate> Bot;
		v8::Persistent<v8::FunctionTemplate> IrcUser;
		v8::Persistent<v8::FunctionTemplate> IrcChannel;
	} ClassTemplates_t;

	static ClassTemplates_t m_ClassTemplates;

private:
	bool m_bLoaded;
	v8::Persistent<v8::Context> m_ScriptContext;

	static v8::Persistent<v8::ObjectTemplate> m_GlobalTemplate;
};

#endif
