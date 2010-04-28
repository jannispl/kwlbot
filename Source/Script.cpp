/*
kwlbot IRC bot


File:		Script.cpp
Purpose:	Class which represents a script

*/

#include "StdInc.h"
#include "Script.h"
#include "ScriptFunctions.h"

v8::Persistent<v8::ObjectTemplate> CScript::m_GlobalTemplate;
CScript::ClassTemplates_t CScript::m_ClassTemplates;

CScript::CScript()
	: m_bLoaded(false)
{
}

CScript::~CScript()
{
}

bool CScript::Load(const char *szFilename)
{
	if (m_bLoaded)
	{
		return false;
	}

	v8::HandleScope handleScope;

	if (m_GlobalTemplate.IsEmpty())
	{
		// CBot
		m_ClassTemplates.Bot = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New());
		m_ClassTemplates.Bot->SetClassName(v8::String::New("Bot"));
		m_ClassTemplates.Bot->InstanceTemplate()->SetInternalFieldCount(1);
		v8::Handle<v8::ObjectTemplate> botProto = m_ClassTemplates.Bot->PrototypeTemplate();
		botProto->Set(v8::String::New("sendRaw"), v8::FunctionTemplate::New(CScriptFunctions::Bot__SendRaw));
		botProto->Set(v8::String::New("sendMessage"), v8::FunctionTemplate::New(CScriptFunctions::Bot__SendMessage));
		botProto->Set(v8::String::New("sendNotice"), v8::FunctionTemplate::New(CScriptFunctions::Bot__SendNotice));
		botProto->Set(v8::String::New("findUser"), v8::FunctionTemplate::New(CScriptFunctions::Bot__FindUser));
		botProto->Set(v8::String::New("findChannel"), v8::FunctionTemplate::New(CScriptFunctions::Bot__FindChannel));

		// CIrcUser
		m_ClassTemplates.IrcUser = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New());
		m_ClassTemplates.IrcUser->SetClassName(v8::String::New("IrcUser"));
		m_ClassTemplates.IrcUser->InstanceTemplate()->SetInternalFieldCount(1);
		v8::Handle<v8::ObjectTemplate> userProto = m_ClassTemplates.IrcUser->PrototypeTemplate();
		userProto->Set(v8::String::New("getNickname"), v8::FunctionTemplate::New(CScriptFunctions::IrcUser__GetNickname));
		userProto->Set(v8::String::New("hasChannel"), v8::FunctionTemplate::New(CScriptFunctions::IrcUser__HasChannel));

		// CIrcChannel
		m_ClassTemplates.IrcChannel = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New());
		m_ClassTemplates.IrcChannel->SetClassName(v8::String::New("IrcChannel"));
		m_ClassTemplates.IrcChannel->InstanceTemplate()->SetInternalFieldCount(1);
		v8::Handle<v8::ObjectTemplate> channelProto = m_ClassTemplates.IrcChannel->PrototypeTemplate();
		channelProto->Set(v8::String::New("getName"), v8::FunctionTemplate::New(CScriptFunctions::IrcChannel__GetName));
		channelProto->Set(v8::String::New("hasUser"), v8::FunctionTemplate::New(CScriptFunctions::IrcChannel__HasUser));

		// global
		m_GlobalTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
		m_GlobalTemplate->Set(v8::String::New("print"), v8::FunctionTemplate::New(CScriptFunctions::Print));
		m_GlobalTemplate->Set(v8::String::New("addEventHandler"), v8::FunctionTemplate::New(CScriptFunctions::AddEventHandler));
	}

	// create a new context
	m_ScriptContext = v8::Context::New(NULL, m_GlobalTemplate);

	v8::Context::Scope contextScope(m_ScriptContext);

	v8::Handle<v8::String> strFilename = v8::String::New(szFilename);

	FILE *pFile = fopen(szFilename, "rb");
	if (pFile == NULL)
	{
		return false;
	}
	fseek(pFile, 0, SEEK_END);
	size_t iSize = (size_t)ftell(pFile);
	rewind(pFile);

	char *pContent = new char[iSize + 1];
	pContent[iSize] = '\0';
	for (size_t i = 0; i < iSize;)
	{
		i += fread(&pContent[i], 1, iSize - i, pFile);
	}
	fclose(pFile);

	v8::Handle<v8::String> strSource = v8::String::New(pContent, iSize);
	delete[] pContent;

	if (strSource.IsEmpty())
	{
		printf("Error reading '%s'\n", szFilename);
		return false;
	}

	v8::TryCatch try_catch;
	v8::Handle<v8::Script> script = v8::Script::Compile(strSource, strFilename);

	if (script.IsEmpty())
	{
		//ReportException(&try_catch);
		return false;
	}

	CScriptFunctions::m_pCallingScript = this;
	v8::Handle<v8::Value> result = script->Run();
	CScriptFunctions::m_pCallingScript = NULL;

#if 0
	if (result.IsEmpty())
	{
		// Print errors that happened during execution.
		//ReportException(&try_catch);
	}
	else
	{
		/*if (print_result && !result->IsUndefined()) {
		// If all went well and the result wasn't undefined then print
		// the returned value.
		v8::String::Utf8Value str(result);
		const char* cstr = ToCString(str);
		printf("%s\n", cstr);
		}*/
	}
#endif

	m_bLoaded = true;
	return true;
}

v8::Handle<v8::Value> CScript::CallEvent(const char *szEventName, int iArgCount, v8::Handle<v8::Value> *pArgValues)
{
	v8::HandleScope handleScope;

	std::map<std::string, v8::Persistent<v8::Function>>::iterator i = m_mapEventFunctions.find(szEventName);

	if (i == m_mapEventFunctions.end())
	{
		return v8::Undefined();
	}

	v8::Handle<v8::Function> function = i->second;

	if (!function->IsFunction())
	{
		return v8::Undefined();
	}

	CScriptFunctions::m_pCallingScript = this;
	v8::Handle<v8::Value> retval = function->Call(function, iArgCount, pArgValues);
	CScriptFunctions::m_pCallingScript = NULL;

	return retval;
}

void CScript::EnterContext()
{
	m_ScriptContext->Enter();
}

void CScript::ExitContext()
{
	m_ScriptContext->Exit();
}
