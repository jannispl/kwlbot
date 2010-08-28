/*
kwlbot IRC bot


File:		ScriptFunctions.cpp
Purpose:	Contains definitions for scripting functions

*/

#include "StdInc.h"
#include "ScriptFunctions.h"
#include "Bot.h"
#include "ScriptObject.h"
#include "File.h"
#include "ScriptModule.h"
#include "ArgumentList.h"
#include "IrcMessages.h"
#include <math.h>

#ifdef WIN32
#include <psapi.h>
#endif

CScript *CScriptFunctions::m_pCallingScript = NULL;
bool CScriptFunctions::m_bAllowInternalConstructions = false;

FuncReturn CScriptFunctions::Print(const Arguments &args)
{
	int iParams = args.Length();
	if (iParams < 1)
	{
		return v8::False();
	}

	for (int i = 0; i < iParams; ++i)
	{
		//v8::HandleScope handleScope; // do I need this??
		if (i > 0)
		{
			printf(" ");
		}
		v8::String::Utf8Value str(args[i]);
		printf("%s", *str ? *str : "(null)");
	}
	printf("\n");
	fflush(stdout);

	return v8::True();
}

FuncReturn CScriptFunctions::AddEventHandler(const Arguments &args)
{
	if (args.Length() < 2 || m_pCallingScript == NULL)
	{
		return v8::False();
	}

	v8::String::Utf8Value strEvent(args[0]);

	if (*strEvent == NULL || !args[1]->IsFunction())
	{
		return v8::False();
	}

	v8::Handle<v8::Function> function = v8::Handle<v8::Function>::Cast(args[1]);

	CScript::EventHandler *pEventHandler = new CScript::EventHandler;
	pEventHandler->strEvent = *strEvent;
	pEventHandler->handlerFunction = v8::Persistent<v8::Function>::New(function);
	m_pCallingScript->m_plEventHandlers.push_front(pEventHandler);

	return v8::True();
}

FuncReturn CScriptFunctions::RemoveEventHandler(const Arguments &args)
{
	if (args.Length() < 1 || m_pCallingScript == NULL)
	{
		return v8::False();
	}

	v8::String::Utf8Value strEvent(args[0]);

	if (*strEvent == NULL)
	{
		return v8::False();
	}

	for (CPool<CScript::EventHandler *>::iterator i = m_pCallingScript->m_plEventHandlers.begin(); i != m_pCallingScript->m_plEventHandlers.end(); ++i)
	{
		if ((*i)->strEvent == *strEvent)
		{
			if (args.Length() >= 2 && args[1]->IsFunction())
			{
				if ((*i)->handlerFunction->Equals(v8::Handle<v8::Function>::Cast(args[1])))
				{
					delete *i;
					i = m_pCallingScript->m_plEventHandlers.erase(i);
					if (i == m_pCallingScript->m_plEventHandlers.end())
					{
						break;
					}
				}
			}
			else
			{
				delete *i;
				i = m_pCallingScript->m_plEventHandlers.erase(i);
				if (i == m_pCallingScript->m_plEventHandlers.end())
				{
					break;
				}
			}
		}
	}

	return v8::True();
}

FuncReturn CScriptFunctions::GetEventHandlers(const Arguments &args)
{
	if (m_pCallingScript == NULL)
	{
		return v8::False();
	}

	if (args.Length() >= 1)
	{
		v8::String::Utf8Value strEvent(args[0]);
		if (*strEvent == NULL)
		{
			return v8::False();
		}

		v8::Local<v8::Array> eventHandlers = v8::Array::New();
		int iNum = 0;
		for (CPool<CScript::EventHandler *>::iterator i = m_pCallingScript->m_plEventHandlers.begin(); i != m_pCallingScript->m_plEventHandlers.end(); ++i)
		{
			if ((*i)->strEvent == *strEvent)
			{
				eventHandlers->Set(iNum, (*i)->handlerFunction);
				++iNum;
			}
		}

		return eventHandlers;
	}

	v8::Local<v8::Array> eventHandlers = v8::Array::New(m_pCallingScript->m_plEventHandlers.size());
	int iNum = 0;
	for (CPool<CScript::EventHandler *>::iterator i = m_pCallingScript->m_plEventHandlers.begin(); i != m_pCallingScript->m_plEventHandlers.end(); ++i)
	{
		v8::Local<v8::Object> eventHandler = v8::Object::New();
		eventHandler->Set(v8::String::New("event"), v8::String::New((*i)->strEvent.c_str()));
		eventHandler->Set(v8::String::New("handler"), (*i)->handlerFunction);

		eventHandlers->Set(iNum, eventHandler);
		++iNum;
	}

	return eventHandlers;
}

FuncReturn CScriptFunctions::CancelEvent(const Arguments &args)
{
	if (m_pCallingScript == NULL)
	{
		return v8::False();
	}

	if (!m_pCallingScript->m_bCallingEvent)
	{
		return v8::False();
	}

	m_pCallingScript->m_bCurrentEventCancelled = true;

	return v8::True();
}

FuncReturn CScriptFunctions::Bot__SendRaw(const Arguments &args)
{
	if (args.Length() < 1)
	{
		return v8::False();
	}

	v8::String::Utf8Value strRaw(args[0]);
	if (*strRaw == NULL)
	{
		return v8::False();
	}

	CBot *pBot = (CBot *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	pBot->SendRaw(*strRaw);

	return v8::True();
}

FuncReturn CScriptFunctions::Bot__SendMessage(const Arguments &args)
{
	if (args.Length() < 2)
	{
		return v8::False();
	}

	CBot *pBot = (CBot *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	std::string strTarget;

	if (args[0]->IsObject())
	{
		CScriptObject *pTarget = (CScriptObject *)v8::Local<v8::External>::Cast(args[0]->ToObject()->GetInternalField(0))->Value();

		switch (pTarget->GetType())
		{
		case CScriptObject::IrcUser:
			if (((CIrcUser *)pTarget)->GetParentBot() != pBot)
			{
				return v8::False();
			}
			strTarget = ((CIrcUser *)pTarget)->GetNickname();
			break;

		case CScriptObject::IrcChannel:
			if (((CIrcChannel *)pTarget)->GetParentBot() != pBot)
			{
				return v8::False();
			}
			strTarget = ((CIrcChannel *)pTarget)->GetName();
			break;

		default:
			return v8::False();
		}
	}
	else
	{
		v8::String::Utf8Value strTarget_(args[0]);
		if (*strTarget_ == NULL)
		{
			return v8::False();
		}

		strTarget = *strTarget_;
	}

	v8::String::Utf8Value strMessage(args[1]);
	if (*strMessage == NULL)
	{
		return v8::False();
	}

	pBot->SendMessage(CPrivateMessage(strTarget.c_str(), *strMessage));

	return v8::True();
}

FuncReturn CScriptFunctions::Bot__SendNotice(const Arguments &args)
{
	if (args.Length() < 2)
	{
		return v8::False();
	}

	CBot *pBot = (CBot *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	std::string strTarget;

	if (args[0]->IsObject())
	{
		CScriptObject *pTarget = (CScriptObject *)v8::Local<v8::External>::Cast(args[0]->ToObject()->GetInternalField(0))->Value();

		switch (pTarget->GetType())
		{
		case CScriptObject::IrcUser:
			if (((CIrcUser *)pTarget)->GetParentBot() != pBot)
			{
				return v8::False();
			}
			strTarget = ((CIrcUser *)pTarget)->GetNickname();
			break;

		case CScriptObject::IrcChannel:
			if (((CIrcChannel *)pTarget)->GetParentBot() != pBot)
			{
				return v8::False();
			}
			strTarget = ((CIrcChannel *)pTarget)->GetName();
			break;

		default:
			return v8::False();
		}
	}
	else
	{
		v8::String::Utf8Value strTarget_(args[0]);
		if (*strTarget_ == NULL)
		{
			return v8::False();
		}

		strTarget = *strTarget_;
	}

	v8::String::Utf8Value strMessage(args[1]);
	if (*strMessage == NULL)
	{
		return v8::False();
	}

	pBot->SendMessage(CNoticeMessage(strTarget.c_str(), *strMessage));

	return v8::True();
}

FuncReturn CScriptFunctions::Bot__FindUser(const Arguments &args)
{
	if (args.Length() < 1)
	{
		return v8::False();
	}

	v8::String::Utf8Value strUser(args[0]);
	if (*strUser == NULL)
	{
		return v8::False();
	}

	CBot *pBot = (CBot *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	bool bCaseSensitive = args.Length() >= 2 && args[1]->IsBoolean() ? args[1]->ToBoolean()->BooleanValue() : true;

	CIrcUser *pUser = pBot->FindUser(*strUser, bCaseSensitive);
	if (pUser == NULL)
	{
		return v8::False();
	}

	m_bAllowInternalConstructions = true;
	v8::Local<v8::Object> obj = CScript::m_classTemplates.IrcUser->GetFunction()->NewInstance();
	m_bAllowInternalConstructions = false;

	obj->SetInternalField(0, v8::External::New(pUser));
	return obj;
}

FuncReturn CScriptFunctions::Bot__FindChannel(const Arguments &args)
{
	if (args.Length() < 1)
	{
		return v8::False();
	}

	v8::String::Utf8Value strChannel(args[0]);
	if (*strChannel == NULL)
	{
		return v8::False();
	}

	CBot *pBot = (CBot *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	CIrcChannel *pChannel = pBot->FindChannel(*strChannel);
	if (pChannel == NULL)
	{
		return v8::False();
	}

	m_bAllowInternalConstructions = true;
	v8::Local<v8::Object> obj = CScript::m_classTemplates.IrcChannel->GetFunction()->NewInstance();
	m_bAllowInternalConstructions = false;
	obj->SetInternalField(0, v8::External::New(pChannel));
	return obj;
}

FuncReturn CScriptFunctions::Bot__JoinChannel(const Arguments &args)
{
	if (args.Length() < 1)
	{
		return v8::False();
	}

	v8::String::Utf8Value strChannel(args[0]);
	if (*strChannel == NULL)
	{
		return v8::False();
	}

	CBot *pBot = (CBot *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	pBot->JoinChannel(*strChannel);

	return v8::True();
}

FuncReturn CScriptFunctions::Bot__LeaveChannel(const Arguments &args)
{
	if (args.Length() < 1)
	{
		return v8::False();
	}

	CBot *pBot = (CBot *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	CIrcChannel *pChannel = NULL;
	if (args[0]->IsObject())
	{
		CScriptObject *pChannel_ = (CScriptObject *)v8::Local<v8::External>::Cast(args[0]->ToObject()->GetInternalField(0))->Value();
		if (pChannel_->GetType() != CScriptObject::IrcChannel)
		{
			return v8::False();
		}

		if (((CIrcChannel *)pChannel_)->GetParentBot() != pBot)
		{
			return v8::False();
		}

		pChannel = (CIrcChannel *)pChannel_;
	}
	else
	{
		v8::String::Utf8Value strChannel_(args[0]);
		if (*strChannel_ == NULL)
		{
			return v8::False();
		}

		pChannel = pBot->FindChannel(*strChannel_);
	}

	if (pChannel == NULL)
	{
		return v8::False();
	}

	std::string strReason;
	if (args.Length() >= 2)
	{
		v8::String::Utf8Value strReason_(args[1]);
		if (*strReason_ != NULL)
		{
			strReason = *strReason_;
		}
	}

	return v8::Boolean::New(pBot->LeaveChannel(pChannel, strReason.empty() ? NULL : strReason.c_str()));
}

FuncReturn CScriptFunctions::Bot__Die(const Arguments &args)
{
	CBot *pBot = (CBot *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	pBot->Die(CBot::UserRequest);
	
	return v8::True();
}

FuncReturn CScriptFunctions::Bot__Restart(const Arguments &args)
{
	CBot *pBot = (CBot *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	pBot->Die(CBot::Restart);
	
	return v8::True();
}

FuncReturn CScriptFunctions::Bot__ToString(const Arguments &args)
{
	CBot *pBot = (CBot *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	std::string strObjectName;

	if (pBot == NULL || pBot == (void *)0x1)
	{
		strObjectName = "[object Bot]";	
	}
	else
	{
		strObjectName = "Bot(" + std::string(pBot->GetSocket()->GetCurrentNickname()) + ")";
	}

	return v8::String::New(strObjectName.c_str(), strObjectName.length());
}

FuncReturn CScriptFunctions::Bot__getterNickname(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo)
{
	CBot *pBot = (CBot *)v8::Local<v8::External>::Cast(accessorInfo.This()->GetInternalField(0))->Value();

	return v8::String::New(pBot->GetSocket()->GetCurrentNickname());
}

FuncReturn CScriptFunctions::Bot__getterChannels(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo)
{
	CBot *pBot = (CBot *)v8::Local<v8::External>::Cast(accessorInfo.This()->GetInternalField(0))->Value();
	
	CPool<CIrcChannel *> *pChannels = pBot->GetChannels();

	v8::Local<v8::Object> channels = v8::Object::New();

	m_bAllowInternalConstructions = true;
	for (CPool<CIrcChannel *>::iterator i = pChannels->begin(); i != pChannels->end(); ++i)
	{
		v8::Local<v8::Object> objChannel = CScript::m_classTemplates.IrcChannel->GetFunction()->NewInstance();
		objChannel->SetInternalField(0, v8::External::New(*i));

		channels->Set(v8::String::New((*i)->GetName()), objChannel);
	}
	m_bAllowInternalConstructions = false;

	return channels;
}

FuncReturn CScriptFunctions::Bot__getterNumAccessLevels(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo)
{
	CBot *pBot = (CBot *)v8::Local<v8::External>::Cast(accessorInfo.This()->GetInternalField(0))->Value();

	return v8::Integer::New(pBot->GetNumAccessLevels());
}

FuncReturn CScriptFunctions::Bot__getterModeFlags(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo)
{
	CBot *pBot = (CBot *)v8::Local<v8::External>::Cast(accessorInfo.This()->GetInternalField(0))->Value();

	v8::Local<v8::Object> objFlags = v8::Object::New();

	char *szModePrefixes = (char *)pBot->GetModePrefixes();
	int iModeFlag = 1;
	while (*szModePrefixes)
	{
		iModeFlag *= 2;

		char szPrefix[] = { *szModePrefixes, '\0' };
		objFlags->Set(v8::String::New(szPrefix, 1), v8::Int32::New(iModeFlag));

		++szModePrefixes;
	}

	return objFlags;
}

FuncReturn CScriptFunctions::IrcUser__HasChannel(const Arguments &args)
{
	if (args.Length() < 1)
	{
		return v8::False();
	}

	if (!args[0]->IsObject())
	{
		return v8::False();
	}

	CIrcUser *pUser = (CIrcUser *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	CScriptObject *pChannel = (CScriptObject *)v8::Local<v8::External>::Cast(args[0]->ToObject()->GetInternalField(0))->Value();
	if (pChannel->GetType() != CScriptObject::IrcChannel)
	{
		return v8::False();
	}

	if (((CIrcChannel *)pChannel)->GetParentBot() != pUser->GetParentBot())
	{
		return v8::False();
	}

	return v8::Boolean::New(pUser->HasChannel((CIrcChannel *)pChannel));
}

FuncReturn CScriptFunctions::IrcUser__SendMessage(const Arguments &args)
{
	if (args.Length() < 1)
	{
		return v8::False();
	}

	v8::String::Utf8Value strMessage(args[0]);
	if (*strMessage == NULL)
	{
		return v8::False();
	}

	CIrcUser *pUser = (CIrcUser *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	pUser->GetParentBot()->SendMessage(CPrivateMessage(pUser->GetNickname().c_str(), *strMessage));

	return v8::True();
}

FuncReturn CScriptFunctions::IrcUser__TestAccessLevel(const Arguments &args)
{
	if (args.Length() < 1)
	{
		return v8::False();
	}

	if (!args[0]->IsInt32())
	{
		return v8::False();
	}

	CIrcUser *pUser = (CIrcUser *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	int iLevel = (int)args[0]->ToInt32()->NumberValue();

	return v8::Boolean::New(pUser->GetParentBot()->TestAccessLevel(pUser, iLevel));
}

FuncReturn CScriptFunctions::IrcUser__GetModeOnChannel(const Arguments &args)
{
	if (args.Length() < 1)
	{
		return v8::False();
	}

	if (!args[0]->IsObject())
	{
		return v8::False();
	}

	CIrcUser *pUser = (CIrcUser *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	CScriptObject *pChannel = (CScriptObject *)v8::Local<v8::External>::Cast(args[0]->ToObject()->GetInternalField(0))->Value();
	if (pChannel->GetType() != CScriptObject::IrcChannel)
	{
		return v8::False();
	}

	if (((CIrcChannel *)pChannel)->GetParentBot() != pUser->GetParentBot())
	{
		return v8::False();
	}

	return v8::Integer::New((int)pUser->GetModeOnChannel((CIrcChannel *)pChannel));
}

FuncReturn CScriptFunctions::IrcUser__TestLeastModeOnChannel(const Arguments &args)
{
	if (args.Length() < 1)
	{
		return v8::False();
	}

	if (!args[0]->IsObject())
	{
		return v8::False();
	}

	CIrcUser *pUser = (CIrcUser *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	CScriptObject *pChannel = (CScriptObject *)v8::Local<v8::External>::Cast(args[0]->ToObject()->GetInternalField(0))->Value();
	if (pChannel->GetType() != CScriptObject::IrcChannel)
	{
		return v8::False();
	}

	if (((CIrcChannel *)pChannel)->GetParentBot() != pUser->GetParentBot())
	{
		return v8::False();
	}

	int iFlag;
	if (args[1]->IsString())
	{
		int iLength = args[1]->ToString()->Length();
		if (iLength == 1)
		{
			v8::String::Utf8Value strPrefix(args[1]);
			char cPrefix = (*strPrefix)[0];

			char *szModePrefixes = (char *)pUser->GetParentBot()->GetModePrefixes();
			iFlag = 1;
			while (*szModePrefixes)
			{
				iFlag *= 2;

				if (*szModePrefixes == cPrefix)
				{
					break;
				}
				else if (*(szModePrefixes + 1) == '\0')
				{
					return v8::False();
				}

				++szModePrefixes;
			}
		}
		else if (iLength == 0)
		{
			// Just to please IJzerenRita!
			return v8::True();
		}
		else
		{
			return v8::False();
		}
	}
	else
	{
		iFlag = args[1]->ToInt32()->Value();
	}

	return v8::Boolean::New((pUser->GetModeOnChannel((CIrcChannel *)pChannel) >> (int)sqrt((float)iFlag)) ? true : false);
}

FuncReturn CScriptFunctions::IrcUser__ToString(const Arguments &args)
{
	CIrcUser *pUser = (CIrcUser *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	std::string strObjectName;

	if (pUser == NULL || pUser == (void *)0x1)
	{
		strObjectName = "[object IrcUser]";
	}
	else
	{
		strObjectName = "IrcUser(" + std::string(pUser->GetNickname()) + ")";
	}

	return v8::String::New(strObjectName.c_str(), strObjectName.length());
}

FuncReturn CScriptFunctions::IrcUser__getterNickname(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo)
{
	CIrcUser *pUser = (CIrcUser *)v8::Local<v8::External>::Cast(accessorInfo.This()->GetInternalField(0))->Value();

	return v8::String::New(pUser->GetNickname().c_str());
}

FuncReturn CScriptFunctions::IrcUser__getterIdent(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo)
{
	CIrcUser *pUser = (CIrcUser *)v8::Local<v8::External>::Cast(accessorInfo.This()->GetInternalField(0))->Value();

	return v8::String::New(pUser->GetIdent().c_str());
}

FuncReturn CScriptFunctions::IrcUser__getterHostname(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo)
{	
	CIrcUser *pUser = (CIrcUser *)v8::Local<v8::External>::Cast(accessorInfo.This()->GetInternalField(0))->Value();

	return v8::String::New(pUser->GetHostname().c_str());
}

FuncReturn CScriptFunctions::IrcUser__getterTemporary(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo)
{
	CIrcUser *pUser = (CIrcUser *)v8::Local<v8::External>::Cast(accessorInfo.This()->GetInternalField(0))->Value();

	return v8::Boolean::New(pUser->IsTemporary());
}

FuncReturn CScriptFunctions::IrcChannel__GetName(const Arguments &args)
{
	CIrcChannel *pChannel = (CIrcChannel *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	return v8::String::New(pChannel->GetName());
}

FuncReturn CScriptFunctions::IrcChannel__FindUser(const Arguments &args)
{
	if (args.Length() < 1)
	{
		return v8::False();
	}

	v8::String::Utf8Value strUser(args[0]);
	if (*strUser == NULL)
	{
		return v8::False();
	}

	CIrcChannel *pChannel = (CIrcChannel *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	bool bCaseSensitive = args.Length() >= 2 && args[1]->IsBoolean() ? args[1]->ToBoolean()->BooleanValue() : true;

	CIrcUser *pUser = pChannel->FindUser(*strUser, bCaseSensitive);
	if (pUser == NULL)
	{
		return v8::False();
	}

	m_bAllowInternalConstructions = true;
	v8::Local<v8::Object> obj = CScript::m_classTemplates.IrcUser->GetFunction()->NewInstance();
	m_bAllowInternalConstructions = false;

	obj->SetInternalField(0, v8::External::New(pUser));
	return obj;
}

FuncReturn CScriptFunctions::IrcChannel__HasUser(const Arguments &args)
{
	if (args.Length() < 1)
	{
		return v8::False();
	}

	if (!args[0]->IsObject())
	{
		return v8::False();
	}

	CIrcChannel *pChannel = (CIrcChannel *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	CScriptObject *pUser = (CScriptObject *)v8::Local<v8::External>::Cast(args[0]->ToObject()->GetInternalField(0))->Value();
	if (pUser->GetType() != CScriptObject::IrcUser)
	{
		return v8::False();
	}

	if (((CIrcUser *)pUser)->GetParentBot() != pChannel->GetParentBot())
	{
		return v8::False();
	}

	return v8::Boolean::New(pChannel->HasUser((CIrcUser *)pUser));
}


FuncReturn CScriptFunctions::IrcChannel__SetTopic(const Arguments &args)
{
	if (args.Length() < 1)
	{
		return v8::False();
	}

	v8::String::Utf8Value strTopic(args[0]);
	if (*strTopic == NULL)
	{
		return v8::False();
	}

	CIrcChannel *pChannel = (CIrcChannel *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	pChannel->GetParentBot()->SendMessage(CTopicMessage(pChannel->GetName(), *strTopic));

	return v8::True();
}

FuncReturn CScriptFunctions::IrcChannel__SendMessage(const Arguments &args)
{
	if (args.Length() < 1)
	{
		return v8::False();
	}

	v8::String::Utf8Value strMessage(args[0]);
	if (*strMessage == NULL)
	{
		return v8::False();
	}

	CIrcChannel *pChannel = (CIrcChannel *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	pChannel->GetParentBot()->SendMessage(CPrivateMessage(pChannel->GetName(), *strMessage));

	return v8::True();
}

FuncReturn CScriptFunctions::IrcChannel__ToString(const Arguments &args)
{
	CIrcChannel *pChannel = (CIrcChannel *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	std::string strObjectName;

	if (pChannel == NULL || pChannel == (void *)0x1)
	{
		strObjectName = "[object IrcChannel]";
	}
	else
	{
		strObjectName = "IrcChannel(" + std::string(pChannel->GetName()) + ")";
	}

	return v8::String::New(strObjectName.c_str(), strObjectName.length());
}

FuncReturn CScriptFunctions::IrcChannel__getterName(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo)
{
	CIrcChannel *pChannel = (CIrcChannel *)v8::Local<v8::External>::Cast(accessorInfo.This()->GetInternalField(0))->Value();

	return v8::String::New(pChannel->GetName());
}

FuncReturn CScriptFunctions::IrcChannel__getterUsers(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo)
{
	CIrcChannel *pChannel = (CIrcChannel *)v8::Local<v8::External>::Cast(accessorInfo.This()->GetInternalField(0))->Value();

	CPool<CIrcUser *> *pUsers = pChannel->GetUsers();

	v8::Local<v8::Object> users = v8::Object::New();

	m_bAllowInternalConstructions = true;
	for (CPool<CIrcUser *>::iterator i = pUsers->begin(); i != pUsers->end(); ++i)
	{
		v8::Local<v8::Object> objUser = CScript::m_classTemplates.IrcUser->GetFunction()->NewInstance();
		objUser->SetInternalField(0, v8::External::New(*i));

		users->Set(v8::String::New((*i)->GetNickname().c_str()), objUser);
	}
	m_bAllowInternalConstructions = false;

	return users;
}

FuncReturn CScriptFunctions::IrcChannel__getterTopic(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo)
{
	CIrcChannel *pChannel = (CIrcChannel *)v8::Local<v8::External>::Cast(accessorInfo.This()->GetInternalField(0))->Value();
	
	m_bAllowInternalConstructions = true;
	v8::Local<v8::Object> obj = CScript::m_classTemplates.Topic->GetFunction()->NewInstance();
	m_bAllowInternalConstructions = false;

	obj->SetInternalField(0, v8::External::New(pChannel));
	return obj;
}

void CScriptFunctions::IrcChannel__setterTopic(v8::Local<v8::String> strProperty, v8::Local<v8::Value> newValue, const v8::AccessorInfo &accessorInfo)
{
	CIrcChannel *pChannel = (CIrcChannel *)v8::Local<v8::External>::Cast(accessorInfo.This()->GetInternalField(0))->Value();

	v8::String::Utf8Value strTopic(newValue);
	if (*strTopic == NULL)
	{
		return;
	}

	pChannel->GetParentBot()->SendMessage(CTopicMessage(pChannel->GetName(), *strTopic));
}

FuncReturn CScriptFunctions::Topic__ToString(const Arguments &args)
{
	CIrcChannel *pChannel = (CIrcChannel *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	std::string strObjectName;

	if (pChannel == NULL || pChannel == (void *)0x1)
	{
		strObjectName = "[object Topic]";
	}
	else
	{
		strObjectName = pChannel->m_topicInfo.strTopic;
	}

	return v8::String::New(strObjectName.c_str(), strObjectName.length());
}

FuncReturn CScriptFunctions::Topic__getterSetBy(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo)
{
	CIrcChannel *pChannel = (CIrcChannel *)v8::Local<v8::External>::Cast(accessorInfo.This()->GetInternalField(0))->Value();

	return v8::String::New(pChannel->m_topicInfo.strTopicSetBy.c_str());
}

FuncReturn CScriptFunctions::Topic__getterSetOn(v8::Local<v8::String> strProperty, const v8::AccessorInfo &accessorInfo)
{
	CIrcChannel *pChannel = (CIrcChannel *)v8::Local<v8::External>::Cast(accessorInfo.This()->GetInternalField(0))->Value();
	return v8::Date::New((double)pChannel->m_topicInfo.ullTopicSetDate * 1000);
}

void CScriptFunctions::WeakCallbackUsingDelete(v8::Persistent<v8::Value> pv, void *nobj)
{
	v8::HandleScope handle_scope;

	v8::Local<v8::Object> jobj(v8::Object::Cast(*pv));
	//if( jobj->InternalFieldCount() != (FieldCount) ) return; // warn about this?
	v8::Local<v8::Value> lv(jobj->GetInternalField(0));
	if (lv.IsEmpty() || !lv->IsExternal())
	{
		return;
	}

	v8::V8::AdjustAmountOfExternalAllocatedMemory(-5000);

	delete v8::Local<v8::External>::Cast(lv)->Value();

	jobj->SetInternalField(0, v8::Null());
	pv.Dispose();
	pv.Clear();
}

void CScriptFunctions::WeakCallbackUsingFree(v8::Persistent<v8::Value> pv, void *nobj)
{
	v8::HandleScope handle_scope;

	v8::Local<v8::Object> jobj(v8::Object::Cast(*pv));
	//if( jobj->InternalFieldCount() != (FieldCount) ) return; // warn about this?
	v8::Local<v8::Value> lv(jobj->GetInternalField(0));
	if (lv.IsEmpty() || !lv->IsExternal())
	{
		return;
	}

	v8::V8::AdjustAmountOfExternalAllocatedMemory(-5000);

	free(v8::Local<v8::External>::Cast(lv)->Value());

	jobj->SetInternalField(0, v8::Null());
	pv.Dispose();
	pv.Clear();
}

FuncReturn CScriptFunctions::ScriptModule__constructor(const v8::Arguments &args)
{
	if (m_bAllowInternalConstructions)
	{
		return v8::Undefined();
	}

	if (!args.IsConstructCall() || args.Length() < 1)
	{
		return v8::False();
	}

	v8::String::Utf8Value strPath(args[0]);
	if (*strPath == NULL)
	{
		return v8::False();
	}

	CScriptModule *pScriptModule = new CScriptModule(*strPath);

	v8::V8::AdjustAmountOfExternalAllocatedMemory(5000);

	v8::Local<v8::Function> ctor = CScript::m_classTemplates.ScriptModule->GetFunction();
	m_bAllowInternalConstructions = true;
	v8::Persistent<v8::Object> obj = v8::Persistent<v8::Object>::New(ctor->NewInstance());
	m_bAllowInternalConstructions = false;
	obj.MakeWeak(pScriptModule, WeakCallbackUsingDelete);
	obj->SetInternalField(0, v8::External::New(pScriptModule));
	return obj;
}

FuncReturn CScriptFunctions::ScriptModule__GetProcedure(const v8::Arguments &args)
{
	if (args.Holder()->GetInternalField(0) == v8::Null() || args.Length() < 1)
	{
		return v8::False();
	}

	v8::String::Utf8Value strName(args[0]);
	if (*strName == NULL)	
	{
		return v8::False();
	}

	CScriptModule *pScriptModule = (CScriptModule *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	CScriptModule::Procedure *pProcedure = pScriptModule->AllocateProcedure(*strName);

	v8::Local<v8::Function> ctor = CScript::m_classTemplates.ScriptModuleProcedure->GetFunction();
	m_bAllowInternalConstructions = true;
	v8::Persistent<v8::Object> obj = v8::Persistent<v8::Object>::New(ctor->NewInstance());
	m_bAllowInternalConstructions = false;
	obj.MakeWeak(pProcedure, WeakCallbackUsingDelete);
	obj->SetInternalField(0, v8::External::New(pProcedure));

	return obj;
}

FuncReturn CScriptFunctions::ScriptModuleProcedure__Call(const v8::Arguments &args)
{
	if (args.Holder()->GetInternalField(0) == v8::Null())
	{
		return v8::False();
	}

	CScriptModule::Procedure *pProcedure = (CScriptModule::Procedure *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	CArgumentList list;

	for (int i = 0; i < args.Length(); ++i)
	{
		if (args[i]->IsInt32())
		{
			list.Add(args[i]->ToInt32()->Value());
		}
		else if (args[i]->IsNumber())
		{
			list.Add((float)args[i]->ToNumber()->Value());
		}
		else if (args[i]->IsBoolean())
		{
			list.Add(args[i]->ToBoolean()->Value());
		}
		else if (args[i]->IsObject())
		{
			list.Add(v8::Local<v8::External>::Cast(args[i]->ToObject()->GetInternalField(0))->Value());
		}
		else
		{
			list.Add((void *)NULL);
		}
	}

	return v8::Integer::New(pProcedure->Call(list));
}

FuncReturn CScriptFunctions::Timer__constructor(const Arguments &args)
{
	if (m_bAllowInternalConstructions)
	{
		return v8::Undefined();
	}

	if (args.Length() < 3)
	{
		return v8::False();
	}

	if (!args[0]->IsFunction())
	{
		return v8::False();
	}

	v8::Local<v8::Function> timerFunction = v8::Local<v8::Function>::Cast(args[0]);
	unsigned long ulInterval = args[1]->ToInteger()->Uint32Value();
	unsigned int uiNumCalls = args[2]->ToInt32()->Uint32Value();

	v8::Persistent<v8::Value> *pArgValues = NULL;
	if (args.Length() > 3)
	{
		pArgValues = new v8::Persistent<v8::Value>[args.Length() - 3];
		for (int i = 3; i < args.Length(); ++i)
		{
			pArgValues[i - 3] = v8::Persistent<v8::Value>::New(args[i]);
		}
	}

	CTimerManager::Timer *pTimer = m_pCallingScript->m_pTimerManager->SetTimer(timerFunction, ulInterval, uiNumCalls, args.Length() - 3, pArgValues);

	v8::Local<v8::Function> ctor = CScript::m_classTemplates.Timer->GetFunction();
	m_bAllowInternalConstructions = true;
	v8::Persistent<v8::Object> obj = v8::Persistent<v8::Object>::New(ctor->NewInstance());
	m_bAllowInternalConstructions = false;
	//obj.MakeWeak(pTimer, WeakCallbackUsingDelete);
	obj->SetInternalField(0, v8::External::New(pTimer));
	
	return obj;
}

FuncReturn CScriptFunctions::Timer__Kill(const Arguments &args)
{
	CTimerManager::Timer *pTimer = (CTimerManager::Timer *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	args.Holder()->SetInternalField(0, v8::Null());

	return v8::Boolean::New(m_pCallingScript->m_pTimerManager->KillTimer(pTimer));
}

FuncReturn CScriptFunctions::InternalConstructor(const Arguments &args)
{
	if (!m_bAllowInternalConstructions)
	{
		return v8::ThrowException(v8::Exception::Error(v8::String::New("Internal object")));
	}

	return v8::Undefined();
}
