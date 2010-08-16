/*
kwlbot IRC bot


File:		ScriptEventManager.cpp
Purpose:	Handles calling events in scripts

*/

#include "StdInc.h"
#include "ScriptEventManager.h"

CScriptEventManager::CScriptEventManager(CCore *pParentCore)
{
	m_pParentCore = pParentCore;
}

CScriptEventManager::~CScriptEventManager()
{
}

void CScriptEventManager::OnBotCreated(CBot *pBot)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Handle<v8::Value> argValues[1] = { };
		(*i)->CallEvent("onBotCreated", 0, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnBotDestroyed(CBot *pBot)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Handle<v8::Value> argValues[1] = { };
		(*i)->CallEvent("onBotDestroyed", 0, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnBotConnected(CBot *pBot)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Handle<v8::Value> argValues[1] = { };
		(*i)->CallEvent("onBotConnected", 0, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnBotJoinedChannel(CBot *pBot, CIrcChannel *pChannel)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor = CScript::m_classTemplates.IrcChannel->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;

		v8::Local<v8::Object> channel = ctor->NewInstance();
		channel->SetInternalField(0, v8::External::New(pChannel));

		CScriptFunctions::m_bAllowInternalConstructions = false;

		v8::Handle<v8::Value> argValues[1] = { channel };
		(*i)->CallEvent("onBotJoinedChannel", 1, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnUserJoinedChannel(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_classTemplates.IrcUser->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_classTemplates.IrcChannel->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;

		v8::Local<v8::Object> user = ctor1->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		v8::Local<v8::Object> channel = ctor2->NewInstance();
		channel->SetInternalField(0, v8::External::New(pChannel));

		CScriptFunctions::m_bAllowInternalConstructions = false;

		v8::Handle<v8::Value> argValues[2] = { user, channel };
		(*i)->CallEvent("onUserJoinedChannel", 2, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnUserLeftChannel(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel, const char *szReason)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_classTemplates.IrcUser->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_classTemplates.IrcChannel->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;

		v8::Local<v8::Object> user = ctor1->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		v8::Local<v8::Object> channel = ctor2->NewInstance();
		channel->SetInternalField(0, v8::External::New(pChannel));

		CScriptFunctions::m_bAllowInternalConstructions = false;

		v8::Handle<v8::Value> argValues[3] = { user, channel, v8::String::New(szReason) };
		(*i)->CallEvent("onUserLeftChannel", 3, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnUserKickedUser(CBot *pBot, CIrcUser *pUser, CIrcUser *pVictim, CIrcChannel *pChannel, const char *szReason)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_classTemplates.IrcUser->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_classTemplates.IrcChannel->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;

		v8::Local<v8::Object> user = ctor1->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		v8::Local<v8::Object> victim = ctor1->NewInstance();
		victim->SetInternalField(0, v8::External::New(pVictim));

		v8::Local<v8::Object> channel = ctor2->NewInstance();
		channel->SetInternalField(0, v8::External::New(pChannel));

		CScriptFunctions::m_bAllowInternalConstructions = false;

		v8::Handle<v8::Value> argValues[4] = { user, victim, channel, v8::String::New(szReason) };
		(*i)->CallEvent("onUserKickedUser", 4, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnUserQuit(CBot *pBot, CIrcUser *pUser, const char *szReason)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor = CScript::m_classTemplates.IrcUser->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;

		v8::Local<v8::Object> user = ctor->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		CScriptFunctions::m_bAllowInternalConstructions = false;

		v8::Handle<v8::Value> argValues[2] = { user, v8::String::New(szReason) };
		(*i)->CallEvent("onUserQuit", 2, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnUserChangedNickname(CBot *pBot, CIrcUser *pUser, const char *szOldNickname)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor = CScript::m_classTemplates.IrcUser->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;

		v8::Local<v8::Object> user = ctor->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		CScriptFunctions::m_bAllowInternalConstructions = false;

		v8::Handle<v8::Value> argValues[2] = { user, v8::String::New(szOldNickname) };
		(*i)->CallEvent("onUserChangedNickname", 2, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnUserPrivateMessage(CBot *pBot, CIrcUser *pUser, const char *szMessage)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor = CScript::m_classTemplates.IrcUser->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;

		v8::Local<v8::Object> user = ctor->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		CScriptFunctions::m_bAllowInternalConstructions = false;

		v8::Handle<v8::Value> argValues[2] = { user, v8::String::New(szMessage) };
		(*i)->CallEvent("onUserPrivateMessage", 2, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnUserChannelMessage(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel, const char *szMessage)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_classTemplates.IrcUser->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_classTemplates.IrcChannel->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;

		v8::Local<v8::Object> user = ctor1->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		v8::Local<v8::Object> channel = ctor2->NewInstance();
		channel->SetInternalField(0, v8::External::New(pChannel));

		CScriptFunctions::m_bAllowInternalConstructions = false;

		v8::Handle<v8::Value> argValues[3] = { user, channel, v8::String::New(szMessage) };
		(*i)->CallEvent("onUserChannelMessage", 3, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnUserSetChannelModes(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel, const char *szModes, const char *szParams)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_classTemplates.IrcUser->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_classTemplates.IrcChannel->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;

		v8::Local<v8::Object> user = ctor1->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		v8::Local<v8::Object> channel = ctor2->NewInstance();
		channel->SetInternalField(0, v8::External::New(pChannel));

		CScriptFunctions::m_bAllowInternalConstructions = false;

		v8::Handle<v8::Value> argValues[4] = { user, channel, v8::String::New(szModes), v8::String::New(szParams) };
		(*i)->CallEvent("onUserSetChannelModes", 4, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnBotGotChannelUserList(CBot *pBot, CIrcChannel *pChannel)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor = CScript::m_classTemplates.IrcChannel->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;

		v8::Local<v8::Object> channel = ctor->NewInstance();
		channel->SetInternalField(0, v8::External::New(pChannel));

		CScriptFunctions::m_bAllowInternalConstructions = false;

		v8::Handle<v8::Value> argValues[1] = { channel };
		(*i)->CallEvent("onBotGotChannelUserList", 1, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnBotReceivedRaw(CBot *pBot, const char *szRaw)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor = CScript::m_classTemplates.Bot->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;

		v8::Local<v8::Object> bot = ctor->NewInstance();
		bot->SetInternalField(0, v8::External::New(pBot));

		CScriptFunctions::m_bAllowInternalConstructions = false;

		v8::Handle<v8::Value> argValues[1] = { v8::String::New(szRaw) };
		(*i)->CallEvent("onBotReceivedRaw", 1, argValues);

		(*i)->ExitContext();
	}
}
