/*
kwlbot IRC bot


File:		Script.cpp
Purpose:	Class which represents a script

*/

#include "StdInc.h"
#include "Script.h"
#include "ScriptFunctions.h"

v8::Persistent<v8::ObjectTemplate> CScript::m_globalTemplate;
CScript::ClassTemplates_t CScript::m_classTemplates;

CScript::CScript(CBot *pParentBot)
	: m_pParentBot(pParentBot), m_bLoaded(false), m_bCurrentEventCancelled(false), m_bCallingEvent(false)
{
	m_pTimerManager = new CTimerManager(this);
}

CScript::~CScript()
{
	v8::HandleScope handleScope;

	m_scriptContext->DetachGlobal();
	m_scriptContext.Dispose();

	delete m_pTimerManager;
}

bool CScript::Load(CCore *pCore, const char *szFilename)
{
	if (m_bLoaded)
	{
		return false;
	}

	v8::HandleScope handleScope;

	if (m_globalTemplate.IsEmpty())
	{
		// CBot
		m_classTemplates.Bot = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New(CScriptFunctions::InternalConstructor));
		m_classTemplates.Bot->SetClassName(v8::String::New("Bot"));
		m_classTemplates.Bot->InstanceTemplate()->SetInternalFieldCount(1);
		v8::Handle<v8::ObjectTemplate> botProto = m_classTemplates.Bot->PrototypeTemplate();
		botProto->Set(v8::String::New("sendRaw"), v8::FunctionTemplate::New(CScriptFunctions::Bot__SendRaw));
		botProto->Set(v8::String::New("sendMessage"), v8::FunctionTemplate::New(CScriptFunctions::Bot__SendMessage));
		botProto->Set(v8::String::New("sendNotice"), v8::FunctionTemplate::New(CScriptFunctions::Bot__SendNotice));
		botProto->Set(v8::String::New("findUser"), v8::FunctionTemplate::New(CScriptFunctions::Bot__FindUser));
		botProto->Set(v8::String::New("findChannel"), v8::FunctionTemplate::New(CScriptFunctions::Bot__FindChannel));
		botProto->Set(v8::String::New("joinChannel"), v8::FunctionTemplate::New(CScriptFunctions::Bot__JoinChannel));
		botProto->Set(v8::String::New("leaveChannel"), v8::FunctionTemplate::New(CScriptFunctions::Bot__LeaveChannel));
		botProto->Set(v8::String::New("die"), v8::FunctionTemplate::New(CScriptFunctions::Bot__Die));
		botProto->Set(v8::String::New("restart"), v8::FunctionTemplate::New(CScriptFunctions::Bot__Restart));
		botProto->Set(v8::String::New("subscribeCommand"), v8::FunctionTemplate::New(CScriptFunctions::Bot__SubscribeCommand));
		botProto->Set(v8::String::New("unsubscribeCommand"), v8::FunctionTemplate::New(CScriptFunctions::Bot__UnsubscribeCommand));
		botProto->Set(v8::String::New("toString"), v8::FunctionTemplate::New(CScriptFunctions::Bot__ToString));

		botProto->SetAccessor(v8::String::New("nickname"), CScriptFunctions::Bot__getterNickname);
		botProto->SetAccessor(v8::String::New("channels"), CScriptFunctions::Bot__getterChannels);
		botProto->SetAccessor(v8::String::New("numAccessLevels"), CScriptFunctions::Bot__getterNumAccessLevels);
		botProto->SetAccessor(v8::String::New("modeFlags"), CScriptFunctions::Bot__getterModeFlags);
		botProto->SetAccessor(v8::String::New("queueBlocked"), CScriptFunctions::Bot__getterQueueBlocked, CScriptFunctions::Bot__setterQueueBlocked);

		// CIrcUser
		m_classTemplates.IrcUser = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New(CScriptFunctions::InternalConstructor));
		m_classTemplates.IrcUser->SetClassName(v8::String::New("IrcUser"));
		m_classTemplates.IrcUser->InstanceTemplate()->SetInternalFieldCount(1);
		v8::Handle<v8::ObjectTemplate> userProto = m_classTemplates.IrcUser->PrototypeTemplate();
		userProto->Set(v8::String::New("hasChannel"), v8::FunctionTemplate::New(CScriptFunctions::IrcUser__HasChannel));
		userProto->Set(v8::String::New("sendMessage"), v8::FunctionTemplate::New(CScriptFunctions::IrcUser__SendMessage));
		userProto->Set(v8::String::New("testAccessLevel"), v8::FunctionTemplate::New(CScriptFunctions::IrcUser__TestAccessLevel));
		userProto->Set(v8::String::New("getModeOnChannel"), v8::FunctionTemplate::New(CScriptFunctions::IrcUser__GetModeOnChannel));
		userProto->Set(v8::String::New("testLeastModeOnChannel"), v8::FunctionTemplate::New(CScriptFunctions::IrcUser__TestLeastModeOnChannel));
		userProto->Set(v8::String::New("toString"), v8::FunctionTemplate::New(CScriptFunctions::IrcUser__ToString));

		userProto->Set(v8::String::New("say"), v8::FunctionTemplate::New(CScriptFunctions::IrcUser__SendMessage)); // alias for sendMessage

		userProto->SetAccessor(v8::String::New("nickname"), CScriptFunctions::IrcUser__getterNickname);
		userProto->SetAccessor(v8::String::New("ident"), CScriptFunctions::IrcUser__getterIdent);
		userProto->SetAccessor(v8::String::New("hostname"), CScriptFunctions::IrcUser__getterHostname);
		userProto->SetAccessor(v8::String::New("realname"), CScriptFunctions::IrcUser__getterRealname);
		userProto->SetAccessor(v8::String::New("temporary"), CScriptFunctions::IrcUser__getterTemporary);
#ifdef SERVICE
		userProto->SetAccessor(v8::String::New("userModes"), CScriptFunctions::IrcUser__getterUserModes);
#if IRCD == UNREAL
		userProto->SetAccessor(v8::String::New("cloakedHost"), CScriptFunctions::IrcUser__getterCloakedHost);
		userProto->SetAccessor(v8::String::New("virtualHost"), CScriptFunctions::IrcUser__getterVirtualHost);
#elif IRCD == INSPIRCD
		userProto->SetAccessor(v8::String::New("shownHost"), CScriptFunctions::IrcUser__getterCloakedHost);
#endif
#endif

		// CIrcChannel
		m_classTemplates.IrcChannel = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New(CScriptFunctions::InternalConstructor));
		m_classTemplates.IrcChannel->SetClassName(v8::String::New("IrcChannel"));
		m_classTemplates.IrcChannel->InstanceTemplate()->SetInternalFieldCount(1);
		v8::Handle<v8::ObjectTemplate> channelProto = m_classTemplates.IrcChannel->PrototypeTemplate();
		channelProto->Set(v8::String::New("findUser"), v8::FunctionTemplate::New(CScriptFunctions::IrcChannel__FindUser));
		channelProto->Set(v8::String::New("hasUser"), v8::FunctionTemplate::New(CScriptFunctions::IrcChannel__HasUser));
		channelProto->Set(v8::String::New("sendMessage"), v8::FunctionTemplate::New(CScriptFunctions::IrcChannel__SendMessage));
		channelProto->Set(v8::String::New("toString"), v8::FunctionTemplate::New(CScriptFunctions::IrcChannel__ToString));

		channelProto->Set(v8::String::New("say"), v8::FunctionTemplate::New(CScriptFunctions::IrcChannel__SendMessage)); // alias for sendMessage

		channelProto->SetAccessor(v8::String::New("name"), CScriptFunctions::IrcChannel__getterName);
		channelProto->SetAccessor(v8::String::New("users"), CScriptFunctions::IrcChannel__getterUsers);
		channelProto->SetAccessor(v8::String::New("topic"), CScriptFunctions::IrcChannel__getterTopic, CScriptFunctions::IrcChannel__setterTopic);

		// Topic
		m_classTemplates.Topic = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New(CScriptFunctions::InternalConstructor));
		m_classTemplates.Topic->SetClassName(v8::String::New("Topic"));
		m_classTemplates.Topic->InstanceTemplate()->SetInternalFieldCount(1);
		v8::Handle<v8::ObjectTemplate> topicProto = m_classTemplates.Topic->PrototypeTemplate();

		topicProto->Set(v8::String::New("toString"), v8::FunctionTemplate::New(CScriptFunctions::Topic__ToString));
		topicProto->SetAccessor(v8::String::New("setBy"), CScriptFunctions::Topic__getterSetBy);
		topicProto->SetAccessor(v8::String::New("setOn"), CScriptFunctions::Topic__getterSetOn);

		// CScriptModule
		m_classTemplates.ScriptModule = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New(CScriptFunctions::ScriptModule__constructor));
		m_classTemplates.ScriptModule->SetClassName(v8::String::New("ScriptModule"));
		m_classTemplates.ScriptModule->InstanceTemplate()->SetInternalFieldCount(1);
		v8::Handle<v8::ObjectTemplate> scriptModuleProto = m_classTemplates.ScriptModule->PrototypeTemplate();
		scriptModuleProto->Set(v8::String::New("getProcedure"), v8::FunctionTemplate::New(CScriptFunctions::ScriptModule__GetProcedure));

		// CScriptModule::Procedure
		m_classTemplates.ScriptModuleProcedure = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New(CScriptFunctions::InternalConstructor));
		m_classTemplates.ScriptModuleProcedure->SetClassName(v8::String::New("ScriptModuleProcedure"));
		m_classTemplates.ScriptModuleProcedure->InstanceTemplate()->SetInternalFieldCount(1);
		v8::Handle<v8::ObjectTemplate> scriptModuleProcedureProto = m_classTemplates.ScriptModuleProcedure->PrototypeTemplate();
		scriptModuleProcedureProto->Set(v8::String::New("call"), v8::FunctionTemplate::New(CScriptFunctions::ScriptModuleProcedure__Call));

		// Timer
		m_classTemplates.Timer = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New(CScriptFunctions::Timer__constructor));
		m_classTemplates.Timer->SetClassName(v8::String::New("Timer"));
		m_classTemplates.Timer->InstanceTemplate()->SetInternalFieldCount(1);
		v8::Handle<v8::ObjectTemplate> timerProto = m_classTemplates.Timer->PrototypeTemplate();
		timerProto->Set(v8::String::New("kill"), v8::FunctionTemplate::New(CScriptFunctions::Timer__Kill));

		// Global
		m_globalTemplate = v8::Persistent<v8::ObjectTemplate>::New(v8::ObjectTemplate::New());
		m_globalTemplate->Set(v8::String::New("print"), v8::FunctionTemplate::New(CScriptFunctions::Print));
		m_globalTemplate->Set(v8::String::New("addEventHandler"), v8::FunctionTemplate::New(CScriptFunctions::AddEventHandler));
		m_globalTemplate->Set(v8::String::New("removeEventHandler"), v8::FunctionTemplate::New(CScriptFunctions::RemoveEventHandler));
		m_globalTemplate->Set(v8::String::New("getEventHandlers"), v8::FunctionTemplate::New(CScriptFunctions::GetEventHandlers));
		m_globalTemplate->Set(v8::String::New("cancelEvent"), v8::FunctionTemplate::New(CScriptFunctions::CancelEvent));

		m_globalTemplate->Set(v8::String::New("Bot"), m_classTemplates.Bot);
		m_globalTemplate->Set(v8::String::New("IrcUser"), m_classTemplates.IrcUser);
		m_globalTemplate->Set(v8::String::New("IrcChannel"), m_classTemplates.IrcChannel);
		m_globalTemplate->Set(v8::String::New("Topic"), m_classTemplates.Topic);
		m_globalTemplate->Set(v8::String::New("ScriptModule"), m_classTemplates.ScriptModule);
		m_globalTemplate->Set(v8::String::New("Timer"), m_classTemplates.Timer);

		m_globalTemplate->Set(v8::String::New("_service_"), 
#ifdef SERVICE
			v8::True(),
#else
			v8::False(),
#endif
			static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete));

#ifdef SERVICE
		m_globalTemplate->Set(v8::String::New("_ircd_"), v8::Int32::New(IRCD), static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontDelete));
#endif

		// Ask the global modules if they have anything
		for (CPool<CGlobalModule *>::iterator i = pCore->GetGlobalModules()->begin(); i != pCore->GetGlobalModules()->end(); ++i)
		{
			(*i)->TemplateRequest(m_globalTemplate);
		}
	}

	// Create a new context
	m_scriptContext = v8::Context::New(NULL, m_globalTemplate);

	v8::Context::Scope contextScope(m_scriptContext);

	// Create the bot object
	v8::Local<v8::Function> ctor = CScript::m_classTemplates.Bot->GetFunction();
	CScriptFunctions::m_bAllowInternalConstructions = true;
	v8::Local<v8::Object> bot = ctor->NewInstance();
	CScriptFunctions::m_bAllowInternalConstructions = false;
	bot->SetInternalField(0, v8::External::New(m_pParentBot));
	m_scriptContext->Global()->Set(v8::String::New("bot"), bot);

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
	for (CPool<EventHandler *>::iterator i = m_plEventHandlers.begin(); i != m_plEventHandlers.end(); ++i)
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
	m_scriptContext->Enter();
}

void CScript::ExitContext()
{
	m_scriptContext->Exit();
}

void CScript::Pulse()
{
	m_pTimerManager->Pulse();
}
