/*
kwlbot IRC bot


File:		IrcChannel.cpp
Purpose:	Class which represents a remote IRC channel

*/

#include "StdInc.h"
#include "IrcChannel.h"
#include <cstdlib>

CIrcChannel::CIrcChannel(CBot *pParentBot, const std::string &strName)
	: m_bHasDetailedUsers(false)
{
	m_pParentBot = pParentBot;

	m_strName = strName;

	for (CPool<CScript *>::iterator i = pParentBot->GetScripts()->begin(); i != pParentBot->GetScripts()->end(); ++i)
	{
		NewScript(*i);
	}
}

CIrcChannel::~CIrcChannel()
{
	for (std::map<CScript *, v8::Persistent<v8::Object> >::iterator i = m_mapScriptObjects.begin(); i != m_mapScriptObjects.end(); ++i)
	{
		i->second.Dispose();
		i->second.Clear();

		i = m_mapScriptObjects.erase(i);
		if (i == m_mapScriptObjects.end())
		{
			break;
		}
	}
}

const std::string &CIrcChannel::GetName()
{
	return m_strName;
}

CIrcUser *CIrcChannel::FindUser(const char *szNickname, bool bCaseSensitive)
{
	typedef int (* Compare_t)(const char *, const char *);
	Compare_t pfnCompare = bCaseSensitive ? strcmp : stricmp;

	for (CPool<CIrcUser *>::iterator i = m_plIrcUsers.begin(); i != m_plIrcUsers.end(); ++i)
	{
		if (pfnCompare(szNickname, (*i)->GetNickname().c_str()) == 0)
		{
			return *i;
		}
	}

	return NULL;
}

bool CIrcChannel::HasUser(CIrcUser *pUser)
{
	for (CPool<CIrcUser *>::iterator i = m_plIrcUsers.begin(); i != m_plIrcUsers.end(); ++i)
	{
		if (*i == pUser)
		{
			return true;
		}
	}
	return false;
}

CPool<CIrcUser *> *CIrcChannel::GetUsers()
{
	return &m_plIrcUsers;
}

CBot *CIrcChannel::GetParentBot()
{
	return m_pParentBot;
}

void CIrcChannel::NewScript(CScript *pScript)
{
	v8::HandleScope handleScope;

	CScriptFunctions::m_bAllowInternalConstructions = true;

	pScript->EnterContext();
		
	v8::Local<v8::Function> ctor = CScript::m_classTemplates.IrcChannel->GetFunction();

	v8::Local<v8::Object> channel = ctor->NewInstance();
	channel->SetInternalField(0, v8::External::New(this));

	m_mapScriptObjects[pScript] = v8::Persistent<v8::Object>::New(channel);

	pScript->ExitContext();

	CScriptFunctions::m_bAllowInternalConstructions = false;
}

void CIrcChannel::DeleteScript(CScript *pScript)
{
	v8::HandleScope handleScope;

	std::map<CScript *, v8::Persistent<v8::Object> >::iterator it = m_mapScriptObjects.find(pScript);
	if (it == m_mapScriptObjects.end())
	{
		return;
	}

	it->second.Dispose();
	it->second.Clear();

	m_mapScriptObjects.erase(it);
}

v8::Object *CIrcChannel::GetScriptObject(CScript *pScript)
{
	return *(m_mapScriptObjects[pScript]);
}

CScriptObject::eScriptType CIrcChannel::GetType()
{
	return IrcChannel;
}
