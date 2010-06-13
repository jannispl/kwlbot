/*
kwlbot IRC bot


File:		IrcSocket.h
Purpose:	Class which manages a connection to an IRC server

*/

class CIrcSocket;

#ifndef _IRCSOCKET_H
#define _IRCSOCKET_H

#include "TcpSocket.h"
#include "IrcSettings.h"
#include "Bot.h"

class DLLEXPORT CIrcSocket
{
	friend class CBot;

public:
	CIrcSocket(CBot *pParentBot);
	~CIrcSocket();

	bool Connect(const char *szHostname, int iPort = 6667);

	int SendRaw(const char *szData);
	int SendRawFormat(const char *szFormat, ...);
	int ReadRaw(char *pDest, size_t iSize);
	void Pulse();
	void HandleData(const char *szData);

	const char *GetCurrentNickname();

private:
	CTcpSocket m_TcpSocket;
	CBot *m_pParentBot;

	std::string m_strBuffer;
	std::string m_strCurrentNickname;
};

#endif