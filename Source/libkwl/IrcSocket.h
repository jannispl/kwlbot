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

/**
 * Class which manages a connection to an IRC server.
 */
class DLLEXPORT CIrcSocket
{
	friend class CBot;

public:
	CIrcSocket(CBot *pParentBot);
	~CIrcSocket();

	/**
	 * Connects the socket to an IRC server.
	 * @param  szHostname  The server's hostname or IP address.
	 * @param  iPort       The server's port (optional)
	 * @param  szPassword  The server's password (optional)
	 * @return True if the connection was established, false otherwise.
	 */
	bool Connect(const char *szHostname, int iPort = 6667, const char *szPassword = NULL);

	/**
	 * Sends raw data to the server.
	 * @param szData The raw data to send.
	 * @return The amount of bytes transmitted, -1 incase of an error.
	 */
	int SendRaw(const char *szData);
	
	/**
	 * Sends raw data to the server (formatted).
	 * @param szData The formatted raw data to send.
	 * @param ... Argument list
	 * @see SendRaw()
	 * @return The amount of bytes transmitted, -1 incase of an error.
	 */
	int SendRawFormat(const char *szFormat, ...);
	
	/**
	 * Receives new data from the server socket, if available.
	 * @param pDest The destination string for the data.
	 * @param iSize The maximum amount of bytes to receive.
	 * @return The amount of bytes received, -1 incase of an error.
	 */
	int ReadRaw(char *pDest, size_t iSize);
	
	/**
	 * Pulses the socket instance.
	 */
	void Pulse();
	
	/**
	 * Parses incoming raw data from the server and redirects it to the parent bot.
	 */
	void HandleData(const char *szData);

	/**
	 * Gets the current nickname.
	 * @return The current nickname.
	 */
	const char *GetCurrentNickname();

private:
	CTcpSocket m_TcpSocket;
	CBot *m_pParentBot;

	std::string m_strBuffer;
	std::string m_strCurrentNickname;
};

#endif
