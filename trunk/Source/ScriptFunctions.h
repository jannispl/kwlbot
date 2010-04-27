/*
kwlbot IRC bot


File:		ScriptFunctions.h
Purpose:	Contains definitions for scripting functions

*/

class CScriptFunctions;

#ifndef _SCRIPTFUNCTIONS_H
#define _SCRIPTFUNCTIONS_H

#include "Script.h"
#include "v8/v8.h"

using v8::Arguments;
typedef v8::Handle<v8::Value> FuncReturn;

class CScriptFunctions
{
public:
	static CScript *m_pCallingScript;

	static FuncReturn Print(const Arguments &args);
	static FuncReturn AddEventHandler(const Arguments &args);

	static FuncReturn Bot__SendRaw(const Arguments &args);
};

#endif
