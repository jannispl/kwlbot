/*
kwlbot IRC bot


File:		IrcUser.cpp
Purpose:	Class which represents a remote IRC user

*/

#include "StdInc.h"
#include "IrcUser.h"
#include <cstdlib>

CIrcUser::CIrcUser(CBot *pParentBot, const std::string &strNickname, bool bTemporary)
{
	m_pParentBot = pParentBot;
	m_bTemporary = bTemporary;

	m_strNickname = strNickname;

	for (CPool<CScript *>::iterator i = pParentBot->GetScripts()->begin(); i != pParentBot->GetScripts()->end(); ++i)
	{
		NewScript(*i);
	}
}

CIrcUser::~CIrcUser()
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

const std::string &CIrcUser::GetNickname()
{
	return m_strNickname;
}

const std::string &CIrcUser::GetIdent()
{
	return m_strIdent;
}

const std::string &CIrcUser::GetRealname()
{
	return m_strRealname;
}

bool CIrcUser::IsTemporary()
{
	return m_bTemporary;
}

const std::string &CIrcUser::GetHostname()
{
	return m_strHostname;
}

bool CIrcUser::HasChannel(CIrcChannel *pChannel)
{
	for (CPool<CIrcChannel *>::iterator i = m_plIrcChannels.begin(); i != m_plIrcChannels.end(); ++i)
	{
		if (*i == pChannel)
		{
			return true;
		}
	}
	return false;
}

char CIrcUser::GetModeOnChannel(CIrcChannel *pChannel)
{
	return m_mapChannelModes[pChannel];
}

#ifdef SERVICE
const std::string &CIrcUser::GetUserModes()
{
	return m_strUserModes;
}

const std::string &CIrcUser::GetCloakedHost()
{
	return m_strCloakedHost;
}

const std::string &CIrcUser::GetVirtualHost()
{
	return m_strVirtualHost;
}
#endif

CBot *CIrcUser::GetParentBot()
{
	return m_pParentBot;
}

void CIrcUser::NewScript(CScript *pScript)
{
	v8::HandleScope handleScope;

	CScriptFunctions::m_bAllowInternalConstructions = true;

	pScript->EnterContext();
		
	v8::Local<v8::Function> ctor = CScript::m_classTemplates.IrcUser->GetFunction();

	v8::Local<v8::Object> user = ctor->NewInstance();
	user->SetInternalField(0, v8::External::New(this));

	m_mapScriptObjects[pScript] = v8::Persistent<v8::Object>::New(user);

	pScript->ExitContext();

	CScriptFunctions::m_bAllowInternalConstructions = false;
}

void CIrcUser::DeleteScript(CScript *pScript)
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

v8::Object *CIrcUser::GetScriptObject(CScript *pScript)
{
	return *(m_mapScriptObjects[pScript]);
}

CScriptObject::eScriptType CIrcUser::GetType()
{
	return IrcUser;
}

void CIrcUser::UpdateIfNecessary(const std::string &strIdent, const std::string &strHostname)
{
	if (strIdent[0] != '\0')
	{
		m_strIdent = strIdent;
	}

	if (strHostname[0] != '\0')
	{
		m_strHostname = strHostname;
	}
}
