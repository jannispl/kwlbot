/*
kwlbot IRC bot


File:		IrcUser.cpp
Purpose:	Class which represents a remote IRC user

*/

#include "StdInc.h"
#include "IrcUser.h"

CIrcUser::CIrcUser(const char *szName)
{
	TRACEFUNC("CIrcUser::CIrcUser");

	strcpy(m_szName, szName);
}

CIrcUser::~CIrcUser()
{
}

void CIrcUser::SetName(const char *szName)
{
	TRACEFUNC("CIrcUser::SetName");

	strcpy(m_szName, szName);
}

const char *CIrcUser::GetName()
{
	TRACEFUNC("CIrcUser::GetName");

	return m_szName;
}

v8::Handle<v8::Value> CIrcUser::GetScriptThis()
{
	v8::HandleScope handleScope;
	v8::Local<v8::Object> obj = CScript::m_ClassTemplates.IrcUser->GetFunction()->NewInstance();
	obj->SetInternalField(0, v8::External::New(this));
	return obj;
}
