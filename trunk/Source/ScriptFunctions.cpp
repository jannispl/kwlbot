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

CScript *CScriptFunctions::m_pCallingScript = NULL;

FuncReturn CScriptFunctions::Print(const Arguments &args)
{
	TRACEFUNC("CScriptFunctions::Print");

	int iParams = args.Length();
	if (iParams < 1)
	{
		return v8::Boolean::New(false);
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

	return v8::Boolean::New(true);
}

FuncReturn CScriptFunctions::AddEventHandler(const Arguments &args)
{
	TRACEFUNC("CScriptFunctions::AddEventHandler");

	if (args.Length() < 2 || m_pCallingScript == NULL)
	{
		return v8::Boolean::New(false);
	}

	if (!args[0]->IsString() || !args[1]->IsFunction())
	{
		return v8::Boolean::New(false);
	}

	v8::String::Utf8Value strEvent(args[0]);
	v8::Handle<v8::Function> function = v8::Handle<v8::Function>::Cast(args[1]);

	CScript::EventHandler eventHandler;
	eventHandler.strEvent = *strEvent;
	eventHandler.handlerFunction = v8::Persistent<v8::Function>::New(function);
	m_pCallingScript->m_lstEventHandlers.push_back(eventHandler);

	return v8::Boolean::New(true);
}

FuncReturn CScriptFunctions::CancelEvent(const Arguments &args)
{
	TRACEFUNC("CScriptFunctions::CancelEvent");

	if (m_pCallingScript == NULL)
	{
		return v8::Boolean::New(false);
	}

	if (!m_pCallingScript->m_bCallingEvent)
	{
		return v8::Boolean::New(false);
	}

	m_pCallingScript->m_bCurrentEventCancelled = true;

	return v8::Boolean::New(true);
}

FuncReturn CScriptFunctions::Bot__SendRaw(const Arguments &args)
{
	TRACEFUNC("CScriptFunctions::Bot__SendRaw");

	if (args.Length() < 1)
	{
		return v8::Boolean::New(false);
	}

	if (!args[0]->IsString())
	{
		return v8::Boolean::New(false);
	}

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::Bot)
	{
		return v8::Boolean::New(false);
	}

	v8::String::Utf8Value strRaw(args[0]);
	if (*strRaw == NULL)
	{
		return v8::Boolean::New(false);
	}

	((CBot *)pObject)->SendRaw(*strRaw);

	return v8::Boolean::New(true);
}

FuncReturn CScriptFunctions::Bot__SendMessage(const Arguments &args)
{
	TRACEFUNC("CScriptFunctions::Bot__SendMessage");

	if (args.Length() < 2)
	{
		return v8::Boolean::New(false);
	}

	if (!(args[0]->IsObject() || args[0]->IsString()) || !args[1]->IsString())
	{
		return v8::Boolean::New(false);
	}

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::Bot)
	{
		return v8::Boolean::New(false);
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
				return v8::Boolean::New(false);
			}
			strTarget = ((CIrcUser *)pTarget)->GetName();
			break;

		case CScriptObject::IrcChannel:
			if (((CIrcChannel *)pTarget)->GetParentBot() != (CBot *)pObject)
			{
				return v8::Boolean::New(false);
			}
			strTarget = ((CIrcChannel *)pTarget)->GetName();
			break;

		default:
			return v8::Boolean::New(false);
		}
	}
	else
	{
		v8::String::Utf8Value strTarget_(args[0]);
		strTarget = *strTarget_;
	}

	v8::String::Utf8Value strMessage(args[1]);
	if (*strMessage == NULL)
	{
		return v8::Boolean::New(false);
	}

	((CBot *)pObject)->SendMessage(strTarget.c_str(), *strMessage);

	return v8::Boolean::New(true);
}

FuncReturn CScriptFunctions::Bot__SendNotice(const Arguments &args)
{
	TRACEFUNC("CScriptFunctions::Bot__SendNotice");

	if (args.Length() < 2)
	{
		return v8::Boolean::New(false);
	}

	if (!(args[0]->IsObject() || args[0]->IsString()) || !args[1]->IsString())
	{
		return v8::Boolean::New(false);
	}

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::Bot)
	{
		return v8::Boolean::New(false);
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
				return v8::Boolean::New(false);
			}
			strTarget = ((CIrcUser *)pTarget)->GetName();
			break;

		case CScriptObject::IrcChannel:
			if (((CIrcChannel *)pTarget)->GetParentBot() != (CBot *)pObject)
			{
				return v8::Boolean::New(false);
			}
			strTarget = ((CIrcChannel *)pTarget)->GetName();
			break;

		default:
			return v8::Boolean::New(false);
		}
	}
	else
	{
		v8::String::Utf8Value strTarget_(args[0]);
		strTarget = *strTarget_;
	}

	v8::String::Utf8Value strMessage(args[1]);
	if (*strMessage == NULL)
	{
		return v8::Boolean::New(false);
	}

	((CBot *)pObject)->SendNotice(strTarget.c_str(), *strMessage);

	return v8::Boolean::New(true);
}

FuncReturn CScriptFunctions::Bot__FindUser(const Arguments &args)
{
	TRACEFUNC("CScriptFunctions::Bot__FindUser");

	if (args.Length() < 1)
	{
		return v8::Boolean::New(false);
	}

	if (!args[0]->IsString())
	{
		return v8::Boolean::New(false);
	}

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::Bot)
	{
		return v8::Boolean::New(false);
	}

	v8::String::Utf8Value strUser(args[0]);
	if (*strUser == NULL)
	{
		return v8::Boolean::New(false);
	}

	CIrcUser *pUser = ((CBot *)pObject)->FindUser(*strUser);
	if (pUser == NULL)
	{
		return v8::Boolean::New(false);
	}

	v8::Local<v8::Object> obj = CScript::m_ClassTemplates.IrcUser->GetFunction()->NewInstance();
	obj->SetInternalField(0, v8::External::New(pUser));
	return obj;
}

FuncReturn CScriptFunctions::Bot__FindChannel(const Arguments &args)
{
	TRACEFUNC("CScriptFunctions::Bot__FindChannel");

	if (args.Length() < 1)
	{
		return v8::Boolean::New(false);
	}

	if (!args[0]->IsString())
	{
		return v8::Boolean::New(false);
	}

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::Bot)
	{
		return v8::Boolean::New(false);
	}

	v8::String::Utf8Value strChannel(args[0]);
	if (*strChannel == NULL)
	{
		return v8::Boolean::New(false);
	}

	CIrcChannel *pChannel = ((CBot *)pObject)->FindChannel(*strChannel);
	if (pChannel == NULL)
	{
		return v8::Boolean::New(false);
	}

	v8::Local<v8::Object> obj = CScript::m_ClassTemplates.IrcChannel->GetFunction()->NewInstance();
	obj->SetInternalField(0, v8::External::New(pChannel));
	return obj;
}

FuncReturn CScriptFunctions::Bot__JoinChannel(const Arguments &args)
{
	TRACEFUNC("CScriptFunctions::Bot__JoinChannel");

	if (args.Length() < 1)
	{
		return v8::Boolean::New(false);
	}

	if (!args[0]->IsString())
	{
		return v8::Boolean::New(false);
	}

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::Bot)
	{
		return v8::Boolean::New(false);
	}

	v8::String::Utf8Value strChannel(args[0]);
	if (*strChannel == NULL)
	{
		return v8::Boolean::New(false);
	}

	((CBot *)pObject)->JoinChannel(*strChannel);

	return v8::Boolean::New(true);
}

FuncReturn CScriptFunctions::Bot__LeaveChannel(const Arguments &args)
{
	TRACEFUNC("CScriptFunctions::Bot__LeaveChannel");

	if (args.Length() < 1)
	{
		return v8::Boolean::New(false);
	}

	if (!(args[0]->IsString() || args[0]->IsObject()))
	{
		return v8::Boolean::New(false);
	}

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::Bot)
	{
		return v8::Boolean::New(false);
	}

	CIrcChannel *pChannel = NULL;
	if (args[0]->IsObject())
	{
		CScriptObject *pChannel_ = (CScriptObject *)v8::Local<v8::External>::Cast(args[0]->ToObject()->GetInternalField(0))->Value();
		if (pChannel_->GetType() != CScriptObject::IrcChannel)
		{
			return v8::Boolean::New(false);
		}

		if (((CIrcChannel *)pChannel_)->GetParentBot() != (CBot *)pObject)
		{
			return v8::Boolean::New(false);
		}

		pChannel = (CIrcChannel *)pChannel_;
	}
	else
	{
		v8::String::Utf8Value strChannel_(args[0]);
		pChannel = ((CBot *)pObject)->FindChannel(*strChannel_);
	}

	if (pChannel == NULL)
	{
		return v8::Boolean::New(false);
	}

	std::string strReason;
	if (args.Length() >= 2 && args[1]->IsString())
	{
		v8::String::Utf8Value strReason_(args[1]);
		strReason = *strReason_;
	}

	return v8::Boolean::New(((CBot *)pObject)->LeaveChannel(pChannel, strReason.empty() ? NULL : strReason.c_str()));
}

FuncReturn CScriptFunctions::IrcUser__GetNickname(const Arguments &args)
{
	TRACEFUNC("CScriptFunctions::IrcUser__GetNickname");

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::IrcUser)
	{
		return v8::Boolean::New(false);
	}

	return v8::String::New(((CIrcUser *)pObject)->GetName());
}

FuncReturn CScriptFunctions::IrcUser__HasChannel(const Arguments &args)
{
	TRACEFUNC("CScriptFunctions::IrcUser__HasChannel");

	if (args.Length() < 1)
	{
		return v8::Boolean::New(false);
	}

	if (!args[0]->IsObject())
	{
		return v8::Boolean::New(false);
	}

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::IrcUser)
	{
		return v8::Boolean::New(false);
	}

	CScriptObject *pChannel = (CScriptObject *)v8::Local<v8::External>::Cast(args[0]->ToObject()->GetInternalField(0))->Value();
	if (pChannel->GetType() != CScriptObject::IrcChannel)
	{
		return v8::Boolean::New(false);
	}

	if (((CIrcChannel *)pChannel)->GetParentBot() != ((CIrcUser *)pObject)->GetParentBot())
	{
		return v8::Boolean::New(false);
	}

	return v8::Boolean::New(((CIrcUser *)pObject)->HasChannel((CIrcChannel *)pChannel));
}

FuncReturn CScriptFunctions::IrcChannel__GetName(const Arguments &args)
{
	TRACEFUNC("CScriptFunctions::IrcChannel__GetName");

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::IrcChannel)
	{
		return v8::Boolean::New(false);
	}

	return v8::String::New(((CIrcChannel *)pObject)->GetName());
}

FuncReturn CScriptFunctions::IrcChannel__HasUser(const Arguments &args)
{
	TRACEFUNC("CScriptFunctions::IrcChannel__HasUser");

	if (args.Length() < 1)
	{
		return v8::Boolean::New(false);
	}

	if (!args[0]->IsObject())
	{
		return v8::Boolean::New(false);
	}

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::IrcChannel)
	{
		return v8::Boolean::New(false);
	}

	CScriptObject *pUser = (CScriptObject *)v8::Local<v8::External>::Cast(args[0]->ToObject()->GetInternalField(0))->Value();
	if (pUser->GetType() != CScriptObject::IrcUser)
	{
		return v8::Boolean::New(false);
	}

	if (((CIrcUser *)pUser)->GetParentBot() != ((CIrcChannel *)pObject)->GetParentBot())
	{
		return v8::Boolean::New(false);
	}

	return v8::Boolean::New(((CIrcChannel *)pObject)->HasUser((CIrcUser *)pUser));
}

FuncReturn CScriptFunctions::File__constructor(const Arguments &args)
{
	TRACEFUNC("CScriptFunctions::File__constructor");

	if (!args.IsConstructCall() || args.Length() < 2)
	{
		return v8::Boolean::New(false);
	}

	if (!args[0]->IsString() || !args[1]->IsString())
	{
		return v8::Boolean::New(false);
	}

	v8::String::Utf8Value strName(args[0]);
	if (*strName == NULL)
	{
		return v8::Boolean::New(false);
	}

	v8::String::Utf8Value strMode(args[1]);
	if (*strMode == NULL)
	{
		return v8::Boolean::New(false);
	}

	CFile *pFile = new CFile(*strName, *strMode);

	v8::Local<v8::Function> ctor = CScript::m_ClassTemplates.File->GetFunction();
	v8::Local<v8::Object> obj = ctor->NewInstance();
	obj->SetInternalField(0, v8::External::New(pFile));

	return obj;
}

FuncReturn CScriptFunctions::File__Destroy(const Arguments &args)
{
	if (args.Holder()->GetInternalField(0) == v8::Null())
	{
		return v8::Null();
	}

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::File)
	{
		return args.Holder()->GetInternalField(0);
	}

	delete pObject;

	args.Holder()->SetInternalField(0, v8::Null());

	v8::Persistent<v8::Object> persistent(v8::Persistent<v8::Object>::New(args.Holder()));
	persistent.ClearWeak();
	persistent.Dispose();
	persistent.Clear();

	return v8::Null();
}

FuncReturn CScriptFunctions::File__Read(const Arguments &args)
{
	if (args.Holder()->GetInternalField(0) == v8::Null())
	{
		return v8::Boolean::New(false);
	}

	if (args.Length() < 1)
	{
		return v8::Boolean::New(false);
	}

	if (!args[0]->IsInt32())
	{
		return v8::Boolean::New(false);
	}

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::File)
	{
		return v8::Boolean::New(false);
	}

	unsigned int iLength = args[0]->ToInt32()->Uint32Value();
	char *szData = new char[iLength + 1];

	size_t iSize = ((CFile *)pObject)->Read(szData, 1, iLength);
	szData[iSize] = '\0';

	v8::Handle<v8::String> strRet = v8::String::New(szData, iSize);

	delete[] szData;

	return strRet;
}

FuncReturn CScriptFunctions::File__Write(const Arguments &args)
{
	if (args.Holder()->GetInternalField(0) == v8::Null())
	{
		return v8::Boolean::New(false);
	}

	if (args.Length() < 1)
	{
		return v8::Boolean::New(false);
	}

	if (!args[0]->IsString())
	{
		return v8::Boolean::New(false);
	}

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::File)
	{
		return v8::Boolean::New(false);
	}

	v8::String::Utf8Value strData(args[0]);
	if (*strData == NULL)
	{
		return v8::Boolean::New(false);
	}

	return v8::Int32::New(((CFile *)pObject)->Write(*strData, 1, strData.length()));
}

FuncReturn CScriptFunctions::File__Eof(const Arguments &args)
{
	if (args.Holder()->GetInternalField(0) == v8::Null())
	{
		return v8::Boolean::New(false);
	}

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::File)
	{
		return v8::Boolean::New(false);
	}

	return v8::Int32::New(((CFile *)pObject)->Eof());
}