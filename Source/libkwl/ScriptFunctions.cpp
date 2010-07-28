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
	m_pCallingScript->m_lstEventHandlers.push_front(pEventHandler);

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

	for (CPool<CScript::EventHandler *>::iterator i = m_pCallingScript->m_lstEventHandlers.begin(); i != m_pCallingScript->m_lstEventHandlers.end(); ++i)
	{
		if ((*i)->strEvent == *strEvent)
		{
			if (args.Length() >= 2 && args[1]->IsFunction())
			{
				if ((*i)->handlerFunction->Equals(v8::Handle<v8::Function>::Cast(args[1])))
				{
					delete *i;
					if ((i = m_pCallingScript->m_lstEventHandlers.erase(i)) == m_pCallingScript->m_lstEventHandlers.end())
					{
						break;
					}
				}
			}
			else
			{
				delete *i;
				if ((i = m_pCallingScript->m_lstEventHandlers.erase(i)) == m_pCallingScript->m_lstEventHandlers.end())
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
		for (CPool<CScript::EventHandler *>::iterator i = m_pCallingScript->m_lstEventHandlers.begin(); i != m_pCallingScript->m_lstEventHandlers.end(); ++i)
		{
			if ((*i)->strEvent == *strEvent)
			{
				eventHandlers->Set(iNum, (*i)->handlerFunction);
				++iNum;
			}
		}

		return eventHandlers;
	}

	v8::Local<v8::Array> eventHandlers = v8::Array::New(m_pCallingScript->m_lstEventHandlers.size());
	int iNum = 0;
	for (CPool<CScript::EventHandler *>::iterator i = m_pCallingScript->m_lstEventHandlers.begin(); i != m_pCallingScript->m_lstEventHandlers.end(); ++i)
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

FuncReturn CScriptFunctions::getterMemoryUsage(v8::Local<v8::String> strProperty, const v8::AccessorInfo& accessorInfo)
{
#ifdef WIN32
	PROCESS_MEMORY_COUNTERS memoryCounters;
	if (!GetProcessMemoryInfo(GetCurrentProcess(), &memoryCounters, sizeof(PROCESS_MEMORY_COUNTERS)))
	{
		return v8::False();
	}

	return v8::Integer::NewFromUnsigned(memoryCounters.WorkingSetSize);
#else
	return v8::Integer::NewFromUnsigned(0);
#endif
}

FuncReturn CScriptFunctions::Bot__GetNickname(const Arguments &args)
{
	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::Bot)
	{
		return v8::False();
	}

	return v8::String::New(((CBot *)pObject)->GetSocket()->GetCurrentNickname());
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

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::Bot)
	{
		return v8::False();
	}

	((CBot *)pObject)->SendRaw(*strRaw);

	return v8::True();
}

FuncReturn CScriptFunctions::Bot__SendMessage(const Arguments &args)
{
	if (args.Length() < 2)
	{
		return v8::False();
	}

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::Bot)
	{
		return v8::False();
	}

	std::string strTarget;

	if (args[0]->IsObject())
	{
		CScriptObject *pTarget = (CScriptObject *)v8::Local<v8::External>::Cast(args[0]->ToObject()->GetInternalField(0))->Value();

		switch (pTarget->GetType())
		{
		case CScriptObject::IrcUser:
			if (((CIrcUser *)pTarget)->GetParentBot() != (CBot *)pObject)
			{
				return v8::False();
			}
			strTarget = ((CIrcUser *)pTarget)->GetNickname();
			break;

		case CScriptObject::IrcChannel:
			if (((CIrcChannel *)pTarget)->GetParentBot() != (CBot *)pObject)
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

	((CBot *)pObject)->SendMessage(strTarget.c_str(), *strMessage);

	return v8::True();
}

FuncReturn CScriptFunctions::Bot__SendNotice(const Arguments &args)
{
	if (args.Length() < 2)
	{
		return v8::False();
	}

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::Bot)
	{
		return v8::False();
	}

	std::string strTarget;

	if (args[0]->IsObject())
	{
		CScriptObject *pTarget = (CScriptObject *)v8::Local<v8::External>::Cast(args[0]->ToObject()->GetInternalField(0))->Value();

		switch (pTarget->GetType())
		{
		case CScriptObject::IrcUser:
			if (((CIrcUser *)pTarget)->GetParentBot() != (CBot *)pObject)
			{
				return v8::False();
			}
			strTarget = ((CIrcUser *)pTarget)->GetNickname();
			break;

		case CScriptObject::IrcChannel:
			if (((CIrcChannel *)pTarget)->GetParentBot() != (CBot *)pObject)
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

	((CBot *)pObject)->SendNotice(strTarget.c_str(), *strMessage);

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

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::Bot)
	{
		return v8::False();
	}

	bool bCaseSensitive = args.Length() >= 2 && args[1]->IsBoolean() ? args[1]->ToBoolean()->BooleanValue() : true;

	CIrcUser *pUser = ((CBot *)pObject)->FindUser(*strUser, bCaseSensitive);
	if (pUser == NULL)
	{
		return v8::False();
	}

	m_bAllowInternalConstructions = true;
	v8::Local<v8::Object> obj = CScript::m_ClassTemplates.IrcUser->GetFunction()->NewInstance();
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

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::Bot)
	{
		return v8::False();
	}

	CIrcChannel *pChannel = ((CBot *)pObject)->FindChannel(*strChannel);
	if (pChannel == NULL)
	{
		return v8::False();
	}

	m_bAllowInternalConstructions = true;
	v8::Local<v8::Object> obj = CScript::m_ClassTemplates.IrcChannel->GetFunction()->NewInstance();
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

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::Bot)
	{
		return v8::False();
	}

	((CBot *)pObject)->JoinChannel(*strChannel);

	return v8::True();
}

FuncReturn CScriptFunctions::Bot__LeaveChannel(const Arguments &args)
{
	if (args.Length() < 1)
	{
		return v8::False();
	}

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::Bot)
	{
		return v8::False();
	}

	CIrcChannel *pChannel = NULL;
	if (args[0]->IsObject())
	{
		CScriptObject *pChannel_ = (CScriptObject *)v8::Local<v8::External>::Cast(args[0]->ToObject()->GetInternalField(0))->Value();
		if (pChannel_->GetType() != CScriptObject::IrcChannel)
		{
			return v8::False();
		}

		if (((CIrcChannel *)pChannel_)->GetParentBot() != (CBot *)pObject)
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

		pChannel = ((CBot *)pObject)->FindChannel(*strChannel_);
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

	return v8::Boolean::New(((CBot *)pObject)->LeaveChannel(pChannel, strReason.empty() ? NULL : strReason.c_str()));
}

FuncReturn CScriptFunctions::IrcUser__GetNickname(const Arguments &args)
{
	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::IrcUser)
	{
		return v8::False();
	}

	return v8::String::New(((CIrcUser *)pObject)->GetNickname());
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

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::IrcUser)
	{
		return v8::False();
	}

	CScriptObject *pChannel = (CScriptObject *)v8::Local<v8::External>::Cast(args[0]->ToObject()->GetInternalField(0))->Value();
	if (pChannel->GetType() != CScriptObject::IrcChannel)
	{
		return v8::False();
	}

	if (((CIrcChannel *)pChannel)->GetParentBot() != ((CIrcUser *)pObject)->GetParentBot())
	{
		return v8::False();
	}

	return v8::Boolean::New(((CIrcUser *)pObject)->HasChannel((CIrcChannel *)pChannel));
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

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::IrcUser)
	{
		return v8::False();
	}

	((CIrcUser *)pObject)->GetParentBot()->SendMessage(((CIrcUser *)pObject)->GetNickname(), *strMessage);

	return v8::True();
}

FuncReturn CScriptFunctions::IrcUser__GetIdent(const v8::Arguments &args)
{
	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::IrcUser)
	{
		return v8::False();
	}

	return v8::String::New(((CIrcUser *)pObject)->GetIdent());
}

FuncReturn CScriptFunctions::IrcUser__GetHost(const v8::Arguments &args)
{
	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::IrcUser)
	{
		return v8::False();
	}

	return v8::String::New(((CIrcUser *)pObject)->GetHost());
}

FuncReturn CScriptFunctions::IrcUser__TestAccessLevel(const v8::Arguments &args)
{
	if (args.Length() < 1)
	{
		return v8::False();
	}

	if (!args[0]->IsInt32())
	{
		return v8::False();
	}

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::IrcUser)
	{
		return v8::False();
	}

	int iLevel = (int)args[0]->ToInt32()->NumberValue();

	return v8::Boolean::New(((CIrcUser *)pObject)->GetParentBot()->TestAccessLevel((CIrcUser *)pObject, iLevel));
}

FuncReturn CScriptFunctions::IrcUser__IsTemporary(const v8::Arguments &args)
{
	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::IrcUser)
	{
		return v8::False();
	}

	return v8::Boolean::New(((CIrcUser *)pObject)->IsTemporary());
}

FuncReturn CScriptFunctions::IrcUser__GetModeOnChannel(const v8::Arguments &args)
{
	if (args.Length() < 1)
	{
		return v8::False();
	}

	if (!args[0]->IsObject())
	{
		return v8::False();
	}

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::IrcUser)
	{
		return v8::False();
	}

	CScriptObject *pChannel = (CScriptObject *)v8::Local<v8::External>::Cast(args[0]->ToObject()->GetInternalField(0))->Value();
	if (pChannel->GetType() != CScriptObject::IrcChannel)
	{
		return v8::False();
	}

	if (((CIrcChannel *)pChannel)->GetParentBot() != ((CIrcUser *)pObject)->GetParentBot())
	{
		return v8::False();
	}

	return v8::Integer::New((int)((CIrcUser *)pObject)->GetModeOnChannel((CIrcChannel *)pChannel));
}

FuncReturn CScriptFunctions::IrcChannel__GetName(const Arguments &args)
{
	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::IrcChannel)
	{
		return v8::False();
	}

	return v8::String::New(((CIrcChannel *)pObject)->GetName());
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

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::IrcChannel)
	{
		return v8::False();
	}

	bool bCaseSensitive = args.Length() >= 2 && args[1]->IsBoolean() ? args[1]->ToBoolean()->BooleanValue() : true;

	CIrcUser *pUser = ((CIrcChannel *)pObject)->FindUser(*strUser, bCaseSensitive);
	if (pUser == NULL)
	{
		return v8::False();
	}

	m_bAllowInternalConstructions = true;
	v8::Local<v8::Object> obj = CScript::m_ClassTemplates.IrcUser->GetFunction()->NewInstance();
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

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::IrcChannel)
	{
		return v8::False();
	}

	CScriptObject *pUser = (CScriptObject *)v8::Local<v8::External>::Cast(args[0]->ToObject()->GetInternalField(0))->Value();
	if (pUser->GetType() != CScriptObject::IrcUser)
	{
		return v8::False();
	}

	if (((CIrcUser *)pUser)->GetParentBot() != ((CIrcChannel *)pObject)->GetParentBot())
	{
		return v8::False();
	}

	return v8::Boolean::New(((CIrcChannel *)pObject)->HasUser((CIrcUser *)pUser));
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

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::IrcChannel)
	{
		return v8::False();
	}

	((CIrcChannel *)pObject)->SetTopic(*strTopic);

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

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::IrcChannel)
	{
		return v8::False();
	}

	((CIrcChannel *)pObject)->GetParentBot()->SendMessage(((CIrcChannel *)pObject)->GetName(), *strMessage);

	return v8::True();
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

	v8::Local<v8::Function> ctor = CScript::m_ClassTemplates.ScriptModule->GetFunction();
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

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::ScriptModule)
	{
		return v8::False();
	}

	CScriptModule::Procedure *pProcedure = ((CScriptModule *)pObject)->AllocateProcedure(*strName);

	v8::Local<v8::Function> ctor = CScript::m_ClassTemplates.ScriptModuleProcedure->GetFunction();
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

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::ScriptModuleProcedure)
	{
		return v8::False();
	}

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
	}

	return v8::Integer::New(((CScriptModule::Procedure *)pObject)->Call(list));
}

FuncReturn CScriptFunctions::InternalConstructor(const Arguments &args)
{
	if (!m_bAllowInternalConstructions)
	{
		return v8::ThrowException(v8::Exception::Error(v8::String::New("Internal object")));
	}

	return v8::Undefined();
}
