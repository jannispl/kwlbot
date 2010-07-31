/*
kwlbot IRC bot


File:		ScriptObject.h
Purpose:	Abstract class which is used for script objects

*/

class CScriptObject;

#ifndef _SCRIPTOBJECT_H
#define _SCRIPTOBJECT_H

/**
 * @brief Abstract class which is used for script objects.
 */
class CScriptObject
{
public:
	enum eScriptType
	{
		None,
		Bot,
		IrcUser,
		IrcChannel,
		File,
		ScriptModule,
		ScriptModuleProcedure
	};

	virtual eScriptType GetType() = 0;
};

#endif
