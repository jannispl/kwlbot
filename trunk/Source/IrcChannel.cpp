/*
kwlbot IRC bot


File:		IrcChannel.cpp
Purpose:	Class which represents a remote IRC channel

*/

#include "StdInc.h"
#include "IrcChannel.h"

CIrcChannel::CIrcChannel(const char *szName)
{
	TRACEFUNC("CIrcChannel::CIrcChannel");

	strcpy(m_szName, szName);
}

CIrcChannel::~CIrcChannel()
{
}

void CIrcChannel::SetName(const char *szName)
{
	TRACEFUNC("CIrcChannel::SetName");

	strcpy(m_szName, szName);
}

const char *CIrcChannel::GetName()
{
	TRACEFUNC("CIrcChannel::GetName");

	return m_szName;
}

v8::Handle<v8::Value> CIrcChannel::GetScriptThis()
{
	v8::Local<v8::Object> obj = CScript::m_ClassTemplates.IrcChannel->GetFunction()->NewInstance();
	obj->SetInternalField(0, v8::External::New(this));
	return obj;
}