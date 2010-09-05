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

/**
 * @brief Contains definitions for scripting functions.
 */
class CScriptFunctions
{
public:
	static CScript *m_pCallingScript;
	static bool m_bAllowInternalConstructions;

	static FuncReturn Print(const Arguments &args);
	static FuncReturn AddEventHandler(const Arguments &args);
	static FuncReturn RemoveEventHandler(const Arguments &args);
	static FuncReturn GetEventHandlers(const Arguments &args);
	static FuncReturn CancelEvent(const Arguments &args);

	static FuncReturn Bot__SendRaw(const Arguments &args);
	static FuncReturn Bot__SendMessage(const Arguments &args);
	static FuncReturn Bot__SendNotice(const Arguments &args);
	static FuncReturn Bot__FindUser(const Arguments &args);
	static FuncReturn Bot__FindChannel(const Arguments &args);
	static FuncReturn Bot__JoinChannel(const Arguments &args);
	static FuncReturn Bot__LeaveChannel(const Arguments &args);
	static FuncReturn Bot__Die(const Arguments &args);
	static FuncReturn Bot__Restart(const Arguments &args);
	static FuncReturn Bot__ToString(const Arguments &args);

	static FuncReturn Bot__getterNickname(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo);
	static FuncReturn Bot__getterChannels(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo);
	static FuncReturn Bot__getterNumAccessLevels(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo);
	static FuncReturn Bot__getterModeFlags(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo);

	static FuncReturn IrcUser__HasChannel(const Arguments &args);
	static FuncReturn IrcUser__SendMessage(const Arguments &args);
	static FuncReturn IrcUser__TestAccessLevel(const Arguments &args);
	static FuncReturn IrcUser__GetModeOnChannel(const Arguments &args);
	static FuncReturn IrcUser__TestLeastModeOnChannel(const Arguments &args);
	static FuncReturn IrcUser__ToString(const Arguments &args);
	
	static FuncReturn IrcUser__getterNickname(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo);
	static FuncReturn IrcUser__getterIdent(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo);
	static FuncReturn IrcUser__getterHostname(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo);
	static FuncReturn IrcUser__getterTemporary(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo);
#ifdef SERVICE
	static FuncReturn IrcUser__getterUserModes(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo);
#endif

	static FuncReturn IrcChannel__GetName(const Arguments &args);
	static FuncReturn IrcChannel__FindUser(const Arguments &args);
	static FuncReturn IrcChannel__HasUser(const Arguments &args);
	static FuncReturn IrcChannel__SetTopic(const Arguments &args);
	static FuncReturn IrcChannel__SendMessage(const Arguments &args);
	static FuncReturn IrcChannel__ToString(const Arguments &args);

	static FuncReturn IrcChannel__getterName(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo);
	static FuncReturn IrcChannel__getterUsers(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo);
	static FuncReturn IrcChannel__getterTopic(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo);
	static void IrcChannel__setterTopic(v8::Local<v8::String> strProperty, v8::Local<v8::Value> newValue, const v8::AccessorInfo &accessorInfo);

	static FuncReturn Topic__ToString(const Arguments &args);
	static FuncReturn Topic__getterSetBy(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo);
	static FuncReturn Topic__getterSetOn(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo);

	static void WeakCallbackUsingDelete(v8::Persistent<v8::Value> pv, void *nobj);
	static void WeakCallbackUsingFree(v8::Persistent<v8::Value> pv, void *nobj);

	static FuncReturn ScriptModule__constructor(const Arguments &args);
	static FuncReturn ScriptModule__GetProcedure(const Arguments &args);

	static FuncReturn ScriptModuleProcedure__Call(const Arguments &args);

	static FuncReturn Timer__constructor(const Arguments &args);
	static FuncReturn Timer__Kill(const Arguments &args);

	static FuncReturn InternalConstructor(const Arguments &args);
};

#endif
