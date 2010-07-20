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

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;
		v8::Local<v8::Object> bot = ctor1->NewInstance();
		CScriptFunctions::m_bAllowInternalConstructions = false;

		bot->SetInternalField(0, v8::External::New(pBot));

		v8::Handle<v8::Value> argValues[1] = { bot };
		(*i)->CallEvent("onBotCreated", 1, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnBotDestroyed(CBot *pBot)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;
		v8::Local<v8::Object> bot = ctor1->NewInstance();
		CScriptFunctions::m_bAllowInternalConstructions = false;

		bot->SetInternalField(0, v8::External::New(pBot));

		v8::Handle<v8::Value> argValues[1] = { bot };
		(*i)->CallEvent("onBotDestroyed", 1, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnBotConnected(CBot *pBot)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;
		v8::Local<v8::Object> bot = ctor1->NewInstance();
		CScriptFunctions::m_bAllowInternalConstructions = false;

		bot->SetInternalField(0, v8::External::New(pBot));

		v8::Handle<v8::Value> argValues[1] = { bot };
		(*i)->CallEvent("onBotConnected", 1, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnBotJoinedChannel(CBot *pBot, CIrcChannel *pChannel)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_ClassTemplates.IrcChannel->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;

		v8::Local<v8::Object> bot = ctor1->NewInstance();
		bot->SetInternalField(0, v8::External::New(pBot));

		v8::Local<v8::Object> channel = ctor2->NewInstance();
		channel->SetInternalField(0, v8::External::New(pChannel));

		CScriptFunctions::m_bAllowInternalConstructions = false;

		v8::Handle<v8::Value> argValues[2] = { bot, channel };
		(*i)->CallEvent("onBotJoinedChannel", 2, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnUserJoinedChannel(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_ClassTemplates.IrcUser->GetFunction();
		v8::Local<v8::Function> ctor3 = CScript::m_ClassTemplates.IrcChannel->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;

		v8::Local<v8::Object> bot = ctor1->NewInstance();
		bot->SetInternalField(0, v8::External::New(pBot));

		v8::Local<v8::Object> user = ctor2->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		v8::Local<v8::Object> channel = ctor3->NewInstance();
		channel->SetInternalField(0, v8::External::New(pChannel));

		CScriptFunctions::m_bAllowInternalConstructions = false;

		v8::Handle<v8::Value> argValues[3] = { bot, user, channel };
		(*i)->CallEvent("onUserJoinedChannel", 3, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnUserLeftChannel(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel, const char *szReason)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_ClassTemplates.IrcUser->GetFunction();
		v8::Local<v8::Function> ctor3 = CScript::m_ClassTemplates.IrcChannel->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;

		v8::Local<v8::Object> bot = ctor1->NewInstance();
		bot->SetInternalField(0, v8::External::New(pBot));

		v8::Local<v8::Object> user = ctor2->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		v8::Local<v8::Object> channel = ctor3->NewInstance();
		channel->SetInternalField(0, v8::External::New(pChannel));

		CScriptFunctions::m_bAllowInternalConstructions = false;

		v8::Handle<v8::Value> argValues[4] = { bot, user, channel, v8::String::New(szReason) };
		(*i)->CallEvent("onUserLeftChannel", 4, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnUserKickedUser(CBot *pBot, CIrcUser *pUser, CIrcUser *pVictim, CIrcChannel *pChannel, const char *szReason)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_ClassTemplates.IrcUser->GetFunction();
		v8::Local<v8::Function> ctor3 = CScript::m_ClassTemplates.IrcChannel->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;

		v8::Local<v8::Object> bot = ctor1->NewInstance();
		bot->SetInternalField(0, v8::External::New(pBot));

		v8::Local<v8::Object> user = ctor2->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		v8::Local<v8::Object> victim = ctor2->NewInstance();
		victim->SetInternalField(0, v8::External::New(pVictim));

		v8::Local<v8::Object> channel = ctor3->NewInstance();
		channel->SetInternalField(0, v8::External::New(pChannel));

		CScriptFunctions::m_bAllowInternalConstructions = false;

		v8::Handle<v8::Value> argValues[5] = { bot, user, victim, channel, v8::String::New(szReason) };
		(*i)->CallEvent("onUserKickedUser", 5, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnUserQuit(CBot *pBot, CIrcUser *pUser, const char *szReason)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_ClassTemplates.IrcUser->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;

		v8::Local<v8::Object> bot = ctor1->NewInstance();
		bot->SetInternalField(0, v8::External::New(pBot));

		v8::Local<v8::Object> user = ctor2->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		CScriptFunctions::m_bAllowInternalConstructions = false;

		v8::Handle<v8::Value> argValues[3] = { bot, user, v8::String::New(szReason) };
		(*i)->CallEvent("onUserQuit", 3, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnUserChangedNickname(CBot *pBot, CIrcUser *pUser, const char *szOldNickname)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_ClassTemplates.IrcUser->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;

		v8::Local<v8::Object> bot = ctor1->NewInstance();
		bot->SetInternalField(0, v8::External::New(pBot));

		v8::Local<v8::Object> user = ctor2->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		CScriptFunctions::m_bAllowInternalConstructions = false;

		v8::Handle<v8::Value> argValues[3] = { bot, user, v8::String::New(szOldNickname) };
		(*i)->CallEvent("onUserChangedNickname", 3, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnUserPrivateMessage(CBot *pBot, CIrcUser *pUser, const char *szMessage)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_ClassTemplates.IrcUser->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;

		v8::Local<v8::Object> bot = ctor1->NewInstance();
		bot->SetInternalField(0, v8::External::New(pBot));

		v8::Local<v8::Object> user = ctor2->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		CScriptFunctions::m_bAllowInternalConstructions = false;

		v8::Handle<v8::Value> argValues[3] = { bot, user, v8::String::New(szMessage) };
		(*i)->CallEvent("onUserPrivateMessage", 3, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnUserChannelMessage(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel, const char *szMessage)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_ClassTemplates.IrcUser->GetFunction();
		v8::Local<v8::Function> ctor3 = CScript::m_ClassTemplates.IrcChannel->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;

		v8::Local<v8::Object> bot = ctor1->NewInstance();
		bot->SetInternalField(0, v8::External::New(pBot));

		v8::Local<v8::Object> user = ctor2->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		v8::Local<v8::Object> channel = ctor3->NewInstance();
		channel->SetInternalField(0, v8::External::New(pChannel));

		CScriptFunctions::m_bAllowInternalConstructions = false;

		v8::Handle<v8::Value> argValues[4] = { bot, user, channel, v8::String::New(szMessage) };
		(*i)->CallEvent("onUserChannelMessage", 4, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnUserSetChannelModes(CBot *pBot, CIrcUser *pUser, CIrcChannel *pChannel, const char *szModes, const char *szParams)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();
		v8::Local<v8::Function> ctor2 = CScript::m_ClassTemplates.IrcUser->GetFunction();
		v8::Local<v8::Function> ctor3 = CScript::m_ClassTemplates.IrcChannel->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;

		v8::Local<v8::Object> bot = ctor1->NewInstance();
		bot->SetInternalField(0, v8::External::New(pBot));

		v8::Local<v8::Object> user = ctor2->NewInstance();
		user->SetInternalField(0, v8::External::New(pUser));

		v8::Local<v8::Object> channel = ctor3->NewInstance();
		channel->SetInternalField(0, v8::External::New(pChannel));

		CScriptFunctions::m_bAllowInternalConstructions = false;

		v8::Handle<v8::Value> argValues[5] = { bot, user, channel, v8::String::New(szModes), v8::String::New(szParams) };
		(*i)->CallEvent("onUserSetChannelModes", 5, argValues);

		(*i)->ExitContext();
	}
}

void CScriptEventManager::OnBotReceivedRaw(CBot *pBot, const char *szRaw)
{
	v8::HandleScope handleScope;

	for (CPool<CScript *>::iterator i = pBot->GetScripts()->begin(); i != pBot->GetScripts()->end(); ++i)
	{
		(*i)->EnterContext();

		v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();

		CScriptFunctions::m_bAllowInternalConstructions = true;

		v8::Local<v8::Object> bot = ctor1->NewInstance();
		bot->SetInternalField(0, v8::External::New(pBot));

		CScriptFunctions::m_bAllowInternalConstructions = false;

		v8::Handle<v8::Value> argValues[2] = { bot, v8::String::New(szRaw) };
		(*i)->CallEvent("onBotReceivedRaw", 2, argValues);

		(*i)->ExitContext();
	}
}
