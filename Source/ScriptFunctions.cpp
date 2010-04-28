/*
kwlbot IRC bot


File:		ScriptFunctions.cpp
Purpose:	Contains definitions for scripting functions

*/

#include "StdInc.h"
#include "ScriptFunctions.h"
#include "Bot.h"
#include "ScriptObject.h"

CScript *CScriptFunctions::m_pCallingScript = NULL;

FuncReturn CScriptFunctions::Print(const Arguments &args)
{
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

	m_pCallingScript->m_mapEventFunctions[std::string(*strEvent)] = v8::Persistent<v8::Function>::New(function);

	return v8::Boolean::New(true);
}

FuncReturn CScriptFunctions::Bot__SendRaw(const Arguments &args)
{
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
	if (args.Length() < 2)
	{
		return v8::Boolean::New(false);
	}

	if (!args[0]->IsObject() || !args[1]->IsString())
	{
		return v8::Boolean::New(false);
	}

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::Bot)
	{
		return v8::Boolean::New(false);
	}

	CScriptObject *pTarget = (CScriptObject *)v8::Local<v8::External>::Cast(args[0]->ToObject()->GetInternalField(0))->Value();

	const char *szTarget = NULL;

	switch (pTarget->GetType())
	{
	case CScriptObject::IrcUser:
		szTarget = ((CIrcUser *)pTarget)->GetName();
		break;
		
	case CScriptObject::IrcChannel:
		szTarget = ((CIrcChannel *)pTarget)->GetName();
		break;

	default:
		return v8::Boolean::New(false);
	}

	v8::String::Utf8Value strMessage(args[1]);
	if (*strMessage == NULL)
	{
		return v8::Boolean::New(false);
	}

	((CBot *)pObject)->SendMessage(szTarget, *strMessage);

	return v8::Boolean::New(true);
}

FuncReturn CScriptFunctions::Bot__SendNotice(const Arguments &args)
{
	if (args.Length() < 2)
	{
		return v8::Boolean::New(false);
	}

	if (!args[0]->IsObject() || !args[1]->IsString())
	{
		return v8::Boolean::New(false);
	}

	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::Bot)
	{
		return v8::Boolean::New(false);
	}

	CScriptObject *pTarget = (CScriptObject *)v8::Local<v8::External>::Cast(args[0]->ToObject()->GetInternalField(0))->Value();

	const char *szTarget = NULL;

	switch (pTarget->GetType())
	{
	case CScriptObject::IrcUser:
		szTarget = ((CIrcUser *)pTarget)->GetName();
		break;

	case CScriptObject::IrcChannel:
		szTarget = ((CIrcChannel *)pTarget)->GetName();
		break;

	default:
		return v8::Boolean::New(false);
	}

	v8::String::Utf8Value strMessage(args[1]);
	if (*strMessage == NULL)
	{
		return v8::Boolean::New(false);
	}

	((CBot *)pObject)->SendNotice(szTarget, *strMessage);

	return v8::Boolean::New(true);
}

FuncReturn CScriptFunctions::Bot__FindUser(const Arguments &args)
{
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

FuncReturn CScriptFunctions::IrcUser__GetNickname(const Arguments &args)
{
	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::IrcUser)
	{
		return v8::Boolean::New(false);
	}

	return v8::String::New(((CIrcUser *)pObject)->GetName());
}

FuncReturn CScriptFunctions::IrcUser__HasChannel(const Arguments &args)
{
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

	return v8::Boolean::New(((CIrcUser *)pObject)->HasChannel((CIrcChannel *)pChannel));
}

FuncReturn CScriptFunctions::IrcChannel__GetName(const Arguments &args)
{
	CScriptObject *pObject = (CScriptObject *)v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();
	if (pObject->GetType() != CScriptObject::IrcChannel)
	{
		return v8::Boolean::New(false);
	}

	return v8::String::New(((CIrcChannel *)pObject)->GetName());
}

FuncReturn CScriptFunctions::IrcChannel__HasUser(const Arguments &args)
{
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

	return v8::Boolean::New(((CIrcChannel *)pObject)->HasUser((CIrcUser *)pUser));
}
