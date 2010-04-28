class CScriptObject;

#ifndef _SCRIPTOBJECT_H
#define _SCRIPTOBJECT_H

class CScriptObject
{
public:
	enum eScriptType
	{
		None,
		Bot,
		IrcUser,
		IrcChannel
	};

	virtual eScriptType GetType() = 0;
};

#endif
