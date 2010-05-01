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
	static FuncReturn CancelEvent(const Arguments &args);

	static FuncReturn Bot__SendRaw(const Arguments &args);
	static FuncReturn Bot__SendMessage(const Arguments &args);
	static FuncReturn Bot__SendNotice(const Arguments &args);
	static FuncReturn Bot__FindUser(const Arguments &args);
	static FuncReturn Bot__FindChannel(const Arguments &args);
	static FuncReturn Bot__JoinChannel(const Arguments &args);
	static FuncReturn Bot__LeaveChannel(const Arguments &args);

	static FuncReturn IrcUser__GetNickname(const Arguments &args);
	static FuncReturn IrcUser__HasChannel(const Arguments &args);

	static FuncReturn IrcChannel__GetName(const Arguments &args);
	static FuncReturn IrcChannel__HasUser(const Arguments &args);

	static FuncReturn File__constructor(const Arguments &args);
	static FuncReturn File__Destroy(const Arguments &args);
	static FuncReturn File__IsValid(const Arguments &args);
	static FuncReturn File__Read(const Arguments &args);
	static FuncReturn File__Write(const Arguments &args);
	static FuncReturn File__Eof(const Arguments &args);
};

#endif
