/*
kwlbot IRC bot


File:		Bot.h
Purpose:	Class which represents an IRC bot

*/

class CBot;

#ifndef _BOT_H
#define _BOT_H

#include "IrcSocket.h"
#include "Pool.h"
#include "IrcChannel.h"
#include "IrcUser.h"
#include <vector>
#include <string>

class CBot
{
public:
	CBot();
	~CBot();

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
	void JoinChannel(const char *szChannel);

private:
	bool m_bGotMotd;
	CIrcSocket *m_pIrcSocket;
	CIrcSettings m_IrcSettings;
	CPool<CIrcChannel *> m_plIrcChannels;
	CPool<CIrcChannel *> *m_pIrcChannelQueue;
	CPool<CIrcUser *> m_plGlobalUsers;

	struct
	{
		std::string strChanmodes[4];
		std::string strPrefixes[2];
	} m_SupportSettings;
};

#endif
