/*
kwlbot IRC bot


File:		ScriptFunctions.cpp
Purpose:	Contains definitions for scripting functions

*/

#include "StdInc.h"
#include "ScriptFunctions.h"
#include "Bot.h"

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

	void *pPointer = v8::Local<v8::External>::Cast(args.Holder()->GetInternalField(0))->Value();

	v8::String::Utf8Value strRaw(args[0]);
	if (*strRaw)
	{
		((CBot *)pPointer)->SendRaw(*strRaw);
	}

	return v8::Boolean::New(true);
}
