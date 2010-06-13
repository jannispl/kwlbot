/*
kwlbot IRC bot


File:		Bot.h
Purpose:	Class which represents an IRC bot

*/

class CBot;

#ifndef _BOT_H
#define _BOT_H

#include "ScriptObject.h"
#include "Core.h"
#include "IrcSocket.h"
#include "Pool.h"
#include "IrcChannel.h"
#include "IrcUser.h"
#include "Script.h"
#include "EventManager.h"
#include <vector>
#include <string>

class DLLEXPORT CBot : public CScriptObject
{
public:
	CBot(CCore *pParentCore, CConfig *pConfig);
	~CBot();

	CCore *GetParentCore();
	CIrcSettings *GetSettings();
	CIrcSocket *GetSocket();

	int SendRaw(const char *szData);
	int SendRawFormat(const char *szFormat, ...);
	int ReadRaw(char *pDest, size_t iSize);
	void Pulse();
	CIrcChannel *FindChannel(const char *szName);
	CIrcUser *FindUser(const char *szName);
	char ModeToPrefix(char cMode);
	char PrefixToMode(char cPrefix);
	bool IsPrefixMode(char cMode);
	char GetModeGroup(char cMode);
	void HandleData(const std::vector<std::string> &vecParts);

	bool TestAccessLevel(CIrcUser *pUser, int iLevel);

	CScript *CreateScript(const char *szFilename);
	bool DeleteScript(CScript *pScript);
	CPool<CScript *> *GetScripts();

	void JoinChannel(const char *szChannel);
	bool LeaveChannel(CIrcChannel *pChannel, const char *szReason = NULL);
	void SendMessage(const char *szTarget, const char *szMessage);
	void SendNotice(const char *szTarget, const char *szMessage);

	CScriptObject::eScriptType GetType();

private:
	bool m_bGotMotd;
	CCore *m_pParentCore;
	CIrcSocket *m_pIrcSocket;
	CIrcSettings m_IrcSettings;

#ifdef WIN32
	template class DLLEXPORT CPool<CIrcChannel *>;
	template class DLLEXPORT CPool<CIrcUser *>;
	template class DLLEXPORT CPool<CScript *>;
#endif

	CPool<CIrcChannel *> m_plIrcChannels;
	CPool<CIrcChannel *> *m_pIrcChannelQueue;

	CPool<CIrcUser *> m_plGlobalUsers;
	CPool<CScript *> m_plScripts;

	struct
	{
		std::string strChanmodes[4];
		std::string strPrefixes[2];
	} m_SupportSettings;

	std::string m_strAutoMode;

	typedef struct
	{
		std::string strRequireHost;
		std::string strRequireNickname;
		std::string strRequireIdent;
	} AccessRules;
	std::vector<AccessRules> m_vecAccessRules;
};

#endif
