/*
kwlbot IRC bot


File:		EventManager.cpp
Purpose:	Handles calling events in scripts

*/

#include "StdInc.h"
#include "EventManager.h"

CEventManager::CEventManager(CCore *pParentCore)
{
	m_pParentCore = pParentCore;
}

CEventManager::~CEventManager()
{
}

void CEventManager::OnBotConnected(CBot *pBot)
{
	TRACEFUNC("CEventManager::OnBotConnected");

	v8::HandleScope handleScope;
	for (CPool<CScript *>::iterator i = m_pParentCore->GetScripts()->begin(); i != m_pParentCore->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();
		v8::Local<v8::Object> bot = ctor1->NewInstance();
		bot->SetInternalField(0, v8::External::New(pBot));

		v8::Handle<v8::Value> argValues[1] = { bot };
		(*i)->CallEvent("onBotConnected", 1, argValues);

		(*i)->ExitContext();
	}
}

void CEventManager::OnBotJoinedChannel(CBot *pBot, CIrcChannel *pChannel)
{
	TRACEFUNC("CEventManager::OnBotJoinedChannel");

	v8::HandleScope handleScope;
	for (CPool<CScript *>::iterator i = m_pParentCore->GetScripts()->begin(); i != m_pParentCore->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_ClassTemplates.IrcChannel->GetFunction();

		v8::Local<v8::Object> bot = ctor1->NewInstance();
		bot->SetInternalField(0, v8::External::New(pBot));

		v8::Local<v8::Object> channel = ctor2->NewInstance();
		channel->SetInternalField(0, v8::External::New(pChannel));

		v8::Handle<v8::Value> argValues[2] = { bot, channel };
		(*i)->CallEvent("onBotJoinedChannel", 2, argValues);

		(*i)->ExitContext();
	}
}

void CEventManager::OnUserJoinedChannel(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel)
{
	TRACEFUNC("CEventManager::OnUserJoinedChannel");

	v8::HandleScope handleScope;
	for (CPool<CScript *>::iterator i = m_pParentCore->GetScripts()->begin(); i != m_pParentCore->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_ClassTemplates.IrcUser->GetFunction();
		v8::Local<v8::Function> ctor3 = CScript::m_ClassTemplates.IrcChannel->GetFunction();

		v8::Local<v8::Object> bot = ctor1->NewInstance();
		bot->SetInternalField(0, v8::External::New(pBot));

		v8::Local<v8::Object> user = ctor2->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		v8::Local<v8::Object> channel = ctor3->NewInstance();
		channel->SetInternalField(0, v8::External::New(pChannel));

		v8::Handle<v8::Value> argValues[3] = { bot, user, channel };
		(*i)->CallEvent("onUserJoinedChannel", 3, argValues);

		(*i)->ExitContext();
	}
}

void CEventManager::OnUserLeftChannel(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel, const char *szReason)
{
	TRACEFUNC("CEventManager::OnUserLeftChannel");

	v8::HandleScope handleScope;
	for (CPool<CScript *>::iterator i = m_pParentCore->GetScripts()->begin(); i != m_pParentCore->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_ClassTemplates.IrcUser->GetFunction();
		v8::Local<v8::Function> ctor3 = CScript::m_ClassTemplates.IrcChannel->GetFunction();

		v8::Local<v8::Object> bot = ctor1->NewInstance();
		bot->SetInternalField(0, v8::External::New(pBot));

		v8::Local<v8::Object> user = ctor2->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		v8::Local<v8::Object> channel = ctor3->NewInstance();
		channel->SetInternalField(0, v8::External::New(pChannel));

		v8::Handle<v8::Value> argValues[4] = { bot, user, channel, v8::String::New(szReason) };
		(*i)->CallEvent("onUserLeftChannel", 4, argValues);

		(*i)->ExitContext();
	}
}

void CEventManager::OnUserKickedUser(CBot *pBot, CIrcUser *pUser, CIrcUser *pVictim, CIrcChannel *pChannel, const char *szReason)
{
	TRACEFUNC("CEventManager::OnUserKickedUser");

	v8::HandleScope handleScope;
	for (CPool<CScript *>::iterator i = m_pParentCore->GetScripts()->begin(); i != m_pParentCore->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_ClassTemplates.IrcUser->GetFunction();
		v8::Local<v8::Function> ctor3 = CScript::m_ClassTemplates.IrcChannel->GetFunction();

		v8::Local<v8::Object> bot = ctor1->NewInstance();
		bot->SetInternalField(0, v8::External::New(pBot));

		v8::Local<v8::Object> user = ctor2->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		v8::Local<v8::Object> victim = ctor2->NewInstance();
		victim->SetInternalField(0, v8::External::New(pVictim));

		v8::Local<v8::Object> channel = ctor3->NewInstance();
		channel->SetInternalField(0, v8::External::New(pChannel));

		v8::Handle<v8::Value> argValues[5] = { bot, user, victim, channel, v8::String::New(szReason) };
		(*i)->CallEvent("onUserKickedUser", 5, argValues);

		(*i)->ExitContext();
	}
}

void CEventManager::OnUserQuit(CBot *pBot, CIrcUser *pUser, const char *szReason)
{
	TRACEFUNC("CEventManager::OnUserQuit");

	v8::HandleScope handleScope;
	for (CPool<CScript *>::iterator i = m_pParentCore->GetScripts()->begin(); i != m_pParentCore->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_ClassTemplates.IrcUser->GetFunction();

		v8::Local<v8::Object> bot = ctor1->NewInstance();
		bot->SetInternalField(0, v8::External::New(pBot));

		v8::Local<v8::Object> user = ctor2->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		v8::Handle<v8::Value> argValues[3] = { bot, user, v8::String::New(szReason) };
		(*i)->CallEvent("onUserQuit", 3, argValues);

		(*i)->ExitContext();
	}
}

void CEventManager::OnUserChangedNickname(CBot *pBot, CIrcUser *pUser, const char *szOldNickname)
{
	TRACEFUNC("CEventManager::OnUserChangedNickname");

	v8::HandleScope handleScope;
	for (CPool<CScript *>::iterator i = m_pParentCore->GetScripts()->begin(); i != m_pParentCore->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_ClassTemplates.IrcUser->GetFunction();

		v8::Local<v8::Object> bot = ctor1->NewInstance();
		bot->SetInternalField(0, v8::External::New(pBot));

		v8::Local<v8::Object> user = ctor2->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		v8::Handle<v8::Value> argValues[3] = { bot, user, v8::String::New(szOldNickname) };
		(*i)->CallEvent("onUserChangedNickname", 3, argValues);

		(*i)->ExitContext();
	}
}

void CEventManager::OnUserPrivateMessage(CBot *pBot, CIrcUser *pUser, const char *szMessage)
{
	TRACEFUNC("CEventManager::OnUserPrivateMessage");

	v8::HandleScope handleScope;
	for (CPool<CScript *>::iterator i = m_pParentCore->GetScripts()->begin(); i != m_pParentCore->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_ClassTemplates.IrcUser->GetFunction();

		v8::Local<v8::Object> bot = ctor1->NewInstance();
		bot->SetInternalField(0, v8::External::New(pBot));

		v8::Local<v8::Object> user = ctor2->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		v8::Handle<v8::Value> argValues[3] = { bot, user, v8::String::New(szMessage) };
		(*i)->CallEvent("onUserPrivateMessage", 3, argValues);

		(*i)->ExitContext();
	}
}

void CEventManager::OnUserChannelMessage(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel, const char *szMessage)
{
	TRACEFUNC("CEventManager::OnUserChannelMessage");

	v8::HandleScope handleScope;
	for (CPool<CScript *>::iterator i = m_pParentCore->GetScripts()->begin(); i != m_pParentCore->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_ClassTemplates.IrcUser->GetFunction();
		v8::Local<v8::Function> ctor3 = CScript::m_ClassTemplates.IrcChannel->GetFunction();

		v8::Local<v8::Object> bot = ctor1->NewInstance();
		bot->SetInternalField(0, v8::External::New(pBot));

		v8::Local<v8::Object> user = ctor2->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		v8::Local<v8::Object> channel = ctor3->NewInstance();
		channel->SetInternalField(0, v8::External::New(pChannel));

		v8::Handle<v8::Value> argValues[4] = { bot, user, channel, v8::String::New(szMessage) };
		(*i)->CallEvent("onUserChannelMessage", 4, argValues);

		(*i)->ExitContext();
	}
}

void CEventManager::OnUserSetChannelModes(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel, const char *szModes, const char *szParams)
{
	TRACEFUNC("CEventManager::OnUserSetChannelModes");

	v8::HandleScope handleScope;
	for (CPool<CScript *>::iterator i = m_pParentCore->GetScripts()->begin(); i != m_pParentCore->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_ClassTemplates.IrcUser->GetFunction();
		v8::Local<v8::Function> ctor3 = CScript::m_ClassTemplates.IrcChannel->GetFunction();

		v8::Local<v8::Object> bot = ctor1->NewInstance();
		bot->SetInternalField(0, v8::External::New(pBot));

		v8::Local<v8::Object> user = ctor2->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		v8::Local<v8::Object> channel = ctor3->NewInstance();
		channel->SetInternalField(0, v8::External::New(pChannel));

		v8::Handle<v8::Value> argValues[5] = { bot, user, channel, v8::String::New(szModes), v8::String::New(szParams) };
		(*i)->CallEvent("onUserSetChannelModes", 5, argValues);

		(*i)->ExitContext();
	}
}