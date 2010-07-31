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

CScript::CScript(CBot *pParentBot)
	: m_pParentBot(pParentBot), m_bLoaded(false), m_bCurrentEventCancelled(false), m_bCallingEvent(false)
{
}

CScript::~CScript()
{
}

bool CScript::Load(CCore *pCore, const char *szFilename)
{
	if (m_bLoaded)
	{
		return false;
	}

	v8::HandleScope handleScope;

	if (m_GlobalTemplate.IsEmpty())
	{
		// CBot
		m_ClassTemplates.Bot = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New(CScriptFunctions::InternalConstructor));
		m_ClassTemplates.Bot->SetClassName(v8::String::New("Bot"));
		m_ClassTemplates.Bot->InstanceTemplate()->SetInternalFieldCount(1);
		v8::Handle<v8::ObjectTemplate> botProto = m_ClassTemplates.Bot->PrototypeTemplate();
		botProto->Set(v8::String::New("sendRaw"), v8::FunctionTemplate::New(CScriptFunctions::Bot__SendRaw));
		botProto->Set(v8::String::New("sendMessage"), v8::FunctionTemplate::New(CScriptFunctions::Bot__SendMessage));
		botProto->Set(v8::String::New("sendNotice"), v8::FunctionTemplate::New(CScriptFunctions::Bot__SendNotice));
		botProto->Set(v8::String::New("findUser"), v8::FunctionTemplate::New(CScriptFunctions::Bot__FindUser));
		botProto->Set(v8::String::New("findChannel"), v8::FunctionTemplate::New(CScriptFunctions::Bot__FindChannel));
		botProto->Set(v8::String::New("joinChannel"), v8::FunctionTemplate::New(CScriptFunctions::Bot__JoinChannel));
		botProto->Set(v8::String::New("leaveChannel"), v8::FunctionTemplate::New(CScriptFunctions::Bot__LeaveChannel));

		botProto->SetAccessor(v8::String::New("nickname"), CScriptFunctions::Bot__getterNickname);
		botProto->SetAccessor(v8::String::New("numAccessLevels"), CScriptFunctions::Bot__getterNumAccessLevels);

		// CIrcUser
		m_ClassTemplates.IrcUser = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New(CScriptFunctions::InternalConstructor));
		m_ClassTemplates.IrcUser->SetClassName(v8::String::New("IrcUser"));
		m_ClassTemplates.IrcUser->InstanceTemplate()->SetInternalFieldCount(1);
		v8::Handle<v8::ObjectTemplate> userProto = m_ClassTemplates.IrcUser->PrototypeTemplate();
		userProto->Set(v8::String::New("hasChannel"), v8::FunctionTemplate::New(CScriptFunctions::IrcUser__HasChannel));
		userProto->Set(v8::String::New("sendMessage"), v8::FunctionTemplate::New(CScriptFunctions::IrcUser__SendMessage));
		userProto->Set(v8::String::New("testAccessLevel"), v8::FunctionTemplate::New(CScriptFunctions::IrcUser__TestAccessLevel));
		userProto->Set(v8::String::New("getModeOnChannel"), v8::FunctionTemplate::New(CScriptFunctions::IrcUser__GetModeOnChannel));

		userProto->SetAccessor(v8::String::New("nickname"), CScriptFunctions::IrcUser__getterNickname);
		userProto->SetAccessor(v8::String::New("ident"), CScriptFunctions::IrcUser__getterIdent);
		userProto->SetAccessor(v8::String::New("hostname"), CScriptFunctions::IrcUser__getterHostname);
		userProto->SetAccessor(v8::String::New("temporary"), CScriptFunctions::IrcUser__getterTemporary);

		// CIrcChannel
		m_ClassTemplates.IrcChannel = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New(CScriptFunctions::InternalConstructor));
		m_ClassTemplates.IrcChannel->SetClassName(v8::String::New("IrcChannel"));
		m_ClassTemplates.IrcChannel->InstanceTemplate()->SetInternalFieldCount(1);
		v8::Handle<v8::ObjectTemplate> channelProto = m_ClassTemplates.IrcChannel->PrototypeTemplate();
		channelProto->Set(v8::String::New("findUser"), v8::FunctionTemplate::New(CScriptFunctions::IrcChannel__FindUser));
		channelProto->Set(v8::String::New("hasUser"), v8::FunctionTemplate::New(CScriptFunctions::IrcChannel__HasUser));
		channelProto->Set(v8::String::New("setTopic"), v8::FunctionTemplate::New(CScriptFunctions::IrcChannel__SetTopic));
		channelProto->Set(v8::String::New("sendMessage"), v8::FunctionTemplate::New(CScriptFunctions::IrcChannel__SendMessage));

		channelProto->SetAccessor(v8::String::New("name"), CScriptFunctions::IrcChannel__getterName);

		// CScriptModule
		m_ClassTemplates.ScriptModule = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New(CScriptFunctions::ScriptModule__constructor));
		m_ClassTemplates.ScriptModule->SetClassName(v8::String::New("ScriptModule"));
		m_ClassTemplates.ScriptModule->InstanceTemplate()->SetInternalFieldCount(1);
		v8::Handle<v8::ObjectTemplate> scriptModuleProto = m_ClassTemplates.ScriptModule->PrototypeTemplate();
		scriptModuleProto->Set(v8::String::New("getProcedure"), v8::FunctionTemplate::New(CScriptFunctions::ScriptModule__GetProcedure));

		// CScriptModule::Procedure
		m_ClassTemplates.ScriptModuleProcedure = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New());
		m_ClassTemplates.ScriptModuleProcedure->SetClassName(v8::String::New("ScriptModuleProcedure"));
		m_ClassTemplates.ScriptModuleProcedure->InstanceTemplate()->SetInternalFieldCount(1);
		v8::Handle<v8::ObjectTemplate> scriptModuleProcedureProto = m_ClassTemplates.ScriptModuleProcedure->PrototypeTemplate();
		scriptModuleProcedureProto->Set(v8::String::New("call"), v8::FunctionTemplate::New(CScriptFunctions::ScriptModuleProcedure__Call));

		// Global
		m_GlobalTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
		m_GlobalTemplate->Set(v8::String::New("print"), v8::FunctionTemplate::New(CScriptFunctions::Print));
		m_GlobalTemplate->Set(v8::String::New("addEventHandler"), v8::FunctionTemplate::New(CScriptFunctions::AddEventHandler));
		m_GlobalTemplate->Set(v8::String::New("removeEventHandler"), v8::FunctionTemplate::New(CScriptFunctions::RemoveEventHandler));
		m_GlobalTemplate->Set(v8::String::New("getEventHandlers"), v8::FunctionTemplate::New(CScriptFunctions::GetEventHandlers));
		m_GlobalTemplate->Set(v8::String::New("cancelEvent"), v8::FunctionTemplate::New(CScriptFunctions::CancelEvent));

		m_GlobalTemplate->SetAccessor(v8::String::New("memusage"), CScriptFunctions::getterMemoryUsage);

		m_GlobalTemplate->Set(v8::String::New("Bot"), m_ClassTemplates.Bot);
		m_GlobalTemplate->Set(v8::String::New("IrcUser"), m_ClassTemplates.IrcUser);
		m_GlobalTemplate->Set(v8::String::New("IrcChannel"), m_ClassTemplates.IrcChannel);
		m_GlobalTemplate->Set(v8::String::New("ScriptModule"), m_ClassTemplates.ScriptModule);

		// Ask the global modules if they have anything
		for (CPool<CGlobalModule *>::iterator i = pCore->GetGlobalModules()->begin(); i != pCore->GetGlobalModules()->end(); ++i)
		{
			(*i)->TemplateRequest(m_GlobalTemplate);
		}
	}

	// Create a new context
	m_ScriptContext = v8::Context::New(NULL, m_GlobalTemplate);

	v8::Context::Scope contextScope(m_ScriptContext);

	// Create the bot object
	v8::Local<v8::Function> ctor = CScript::m_ClassTemplates.Bot->GetFunction();
	CScriptFunctions::m_bAllowInternalConstructions = true;
	v8::Local<v8::Object> bot = ctor->NewInstance();
	CScriptFunctions::m_bAllowInternalConstructions = false;
	bot->SetInternalField(0, v8::External::New(m_pParentBot));
	m_ScriptContext->Global()->Set(v8::String::New("bot"), bot);

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
		ReportException(&try_catch);
		return false;
	}

	CScriptFunctions::m_pCallingScript = this;
	v8::Handle<v8::Value> result = script->Run();
	CScriptFunctions::m_pCallingScript = NULL;

	if (result.IsEmpty())
	{
		// Print errors that happened during execution.
		ReportException(&try_catch);
	}
#if 0
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

bool CScript::CallEvent(const char *szEventName, int iArgCount, v8::Handle<v8::Value> *pArgValues)
{
	v8::HandleScope handleScope;

	// Reset it, to be on the safe side
	m_bCurrentEventCancelled = false;

	m_bCallingEvent = true;

	CScriptFunctions::m_pCallingScript = this;
	for (CPool<EventHandler *>::iterator i = m_lstEventHandlers.begin(); i != m_lstEventHandlers.end(); ++i)
	{
		if ((*i)->strEvent == szEventName && (*i)->handlerFunction->IsFunction())
		{
			(*i)->handlerFunction->Call((*i)->handlerFunction, iArgCount, pArgValues);
		}
	}
	CScriptFunctions::m_pCallingScript = NULL;

	m_bCallingEvent = false;

	bool bCancelled = m_bCurrentEventCancelled;
	m_bCurrentEventCancelled = false;
	return !bCancelled;
}

void CScript::ReportException(v8::TryCatch *pTryCatch)
{
	v8::String::Utf8Value exception(pTryCatch->Exception());
	const char* exception_string = *exception != NULL ? *exception : "(null)";
	v8::Handle<v8::Message> message = pTryCatch->Message();
	if (message.IsEmpty())
	{
		// V8 didn't provide any extra information about this error; just
		// print the exception.
		printf("%s\n", exception_string);
	}
	else
	{
		// Print (filename):(line number): (message).
		v8::String::Utf8Value filename(message->GetScriptResourceName());
		const char *filename_string = *filename != NULL ? *filename : "(null)";
		int linenum = message->GetLineNumber();
		printf("%s:%i: %s\n", filename_string, linenum, exception_string);
		// Print line of source code.
		v8::String::Utf8Value sourceline(message->GetSourceLine());
		const char *sourceline_string = *sourceline != NULL ? *sourceline : "(null)";
		printf("%s\n", sourceline_string);

		/*
		// Print wavy underline (GetUnderline is deprecated).
		int start = message->GetStartColumn();
		for (int i = 0; i < start; i++)
		{
			printf(" ");
		}
		int end = message->GetEndColumn();
		for (int i = start; i < end; i++)
		{
			printf("^");
		}
		printf("\n");
		*/

		v8::String::Utf8Value stack_trace(pTryCatch->StackTrace());
		if (stack_trace.length() > 0)
		{
			const char *stack_trace_string = *stack_trace != NULL ? *stack_trace : "(null)";
			printf("%s\n", stack_trace_string);
		}
		printf("\n");
	}
}


void CScript::EnterContext()
{
	m_ScriptContext->Enter();
}

void CScript::ExitContext()
{
	m_ScriptContext->Exit();
}