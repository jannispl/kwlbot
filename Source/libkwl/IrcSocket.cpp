/*
kwlbot IRC bot


File:		IrcSocket.cpp
Purpose:	Class which manages a connection to an IRC server

*/

#include "StdInc.h"
#include "IrcSocket.h"

CIrcSocket::CIrcSocket(CBot *pParentBot)
	: m_bBlockQueue(false)
{
	m_pParentBot = pParentBot;
}

CIrcSocket::~CIrcSocket()
{
#ifndef SERVICE
	SendRawFormat("QUIT :%s", m_pParentBot->GetSettings()->GetQuitMessage());
#else
	SendRawFormat("SQUIT %s :%s", m_pParentBot->GetSettings()->GetServiceHost(), m_pParentBot->GetSettings()->GetQuitMessage());
#endif

	m_tcpSocket.Close();
}

bool CIrcSocket::Connect(const char *szHostname, int iPort, const char *szPassword)
{
	CIrcSettings *pSettings = m_pParentBot->GetSettings();

	if (!m_tcpSocket.Connect(szHostname, iPort))
	{
		printf("[%s] Failed to connect to %s:%d, retrying...\n", pSettings->GetNickname(), szHostname, iPort);
		m_pParentBot->Die(CBot::ConnectionError);
		return false;
	}

	m_tcpSocket.SetBlocking(false);

	printf("[%s] Connection to %s:%d established.\n", pSettings->GetNickname(), szHostname, iPort);

	if (szPassword != NULL && szPassword[0] != '\0')
	{
#if defined(SERVICE) && IRCD == HYBRID
		SendRawFormat("PASS %s :TS", szPassword);
#else
		SendRawFormat("PASS %s", szPassword);
#endif
	}

#ifndef SERVICE
	SendRawFormat("NICK %s", pSettings->GetNickname());
	SendRawFormat("USER %s \"\" \"%s\" :%s", pSettings->GetIdent(), szHostname, pSettings->GetRealname());
#else
#if IRCD == UNREAL
	// This is not what the RFC says at all
	SendRawStatic("PROTOCTL NICKv2 CLK");
	SendRawFormat("SERVER %s 1 123 :kwlbot service", pSettings->GetServiceHost());
	SendRawFormat("NICK %s 1 %u %s %s %s 0 0 %s * :kwlbot", pSettings->GetNickname(), (unsigned long)time(NULL), pSettings->GetIdent(), pSettings->GetServiceHost(), pSettings->GetServiceHost(), szHostname, pSettings->GetRealname());
#elif IRCD == INSPIRCD
	SendRawFormat("SERVER %s %s 0 123 :kwlbot service", pSettings->GetServiceHost(), szPassword);
	SendRawFormat("BURST %u", (unsigned long)time(NULL));
	SendRawStatic("VERSION :kwlbot " VERSION_STRING);
	SendRawFormat("UID 123" INSPIRCD_SID " %u %s %s %s %s 0.0.0.0 %u +i :%s", (unsigned long)time(NULL), pSettings->GetNickname(), pSettings->GetServiceHost(), pSettings->GetServiceHost(), pSettings->GetIdent(), (unsigned long)time(NULL), pSettings->GetRealname());
#elif IRCD == HYBRID
	SendRawFormat("SERVER %s 1 :kwlbot service", pSettings->GetServiceHost());
	SendRawFormat("SVINFO 5 5 0 :%u", (unsigned long)time(NULL));
#endif
#endif

	m_strCurrentNickname = pSettings->GetNickname();

	return true;
}

int CIrcSocket::SendRaw(const char *szData)
{
#ifdef _DEBUG
	printf("[out] %s\n", szData);
#endif

	size_t iLength = strlen(szData);
	char *pData = reinterpret_cast<char *>(malloc(iLength + sizeof(IRC_EOL)));
	strcpy(pData, szData);
	strcat(pData, IRC_EOL);
	m_sendQueue.Add(pData, iLength + sizeof(IRC_EOL) - 1, false, true);

	if (m_bBlockQueue)
	{
		return -1;
	}

	return m_sendQueue.Process(m_tcpSocket.GetSocket()) ? 0 : -1;
}

int CIrcSocket::SendRawStatic(const char *szData)
{
#ifdef _DEBUG
	printf("[out] %s\n", szData);
#endif

	m_sendQueue.Add(const_cast<char *>(szData), strlen(szData), false);
	m_sendQueue.Add(const_cast<char *>(IRC_EOL), sizeof(IRC_EOL) - 1, false);

	if (m_bBlockQueue)
	{
		return -1;
	}

	return m_sendQueue.Process(m_tcpSocket.GetSocket()) ? 0 : -1;
}

int CIrcSocket::SendRawFormat(const char *szFormat, ...)
{
	char szBuffer[IRC_MAX_LEN + 1];

	va_list vaArgs;
	va_start(vaArgs, szFormat);
	vsnprintf(szBuffer, IRC_MAX_LEN + 1, szFormat, vaArgs);
	va_end(vaArgs);

	return SendRaw(szBuffer);
}

int CIrcSocket::ReadRaw(char *pDest, size_t iSize)
{
	return m_tcpSocket.Read(pDest, iSize);
}

void CIrcSocket::Pulse()
{
	if (!m_bBlockQueue)
	{
		m_sendQueue.Process(m_tcpSocket.GetSocket());
	}

	char szBuffer[256];

#ifdef WIN32
	WSASetLastError(0);
#else
	errno = 0;
#endif

	int iSize = ReadRaw(szBuffer, 255);

	if (iSize == 0 || (iSize == -1 && 
#ifdef WIN32
		WSAGetLastError() == WSAECONNABORTED
#else
		errno == ECONNRESET
#endif
		))
	{
		// Connection closed
		printf("[%s] Connection closed\n", GetCurrentNickname());
		m_pParentBot->Die(CBot::ConnectionError);
	}
	else if (iSize != -1)
	{
		szBuffer[iSize] = '\0';

		char szPart[256] = { 0 };
		size_t iPos = 0;

		char *pCopy = szBuffer;
		while (*pCopy)
		{
			if (*pCopy == '\n')
			{
				szPart[iPos - 1] = '\0';
				if (m_strBuffer.empty())
				{
					HandleData(szPart);
				}
				else
				{
					HandleData((m_strBuffer + szPart).c_str());
					m_strBuffer.clear();
				}
				szPart[0] = 0;
				iPos = 0;
			}
			else
			{
				szPart[iPos++] = *pCopy;
			}
			++pCopy;
		}
		if (szPart[0] != 0)
		{
			szPart[iPos] = 0;
			m_strBuffer += szPart;
		}
	}
}

void CIrcSocket::HandleData(const char *szData)
{
#ifdef _DEBUG
	printf("[in] %s\n", szData);
#endif

	bool bPrefix = szData[0] == ':';

	std::string strData(szData);

	std::string strOrigin;
	std::string strCommand;
	std::string strParams;
	std::vector<std::string> vecParams;

	std::string::size_type iSeparator = strData.find(' ');
	if (iSeparator == std::string::npos)
	{
		if (bPrefix)
		{
			// There is no space in the message, and the messages starts with ':' and thus only consists of an origin.
			// We reject messages without any command.
			return;
		}

		strCommand = strData;
	}
	else
	{
		if (bPrefix)
		{
			strData = strData.substr(1);
			--iSeparator;

			strOrigin = strData.substr(0, iSeparator);
			strCommand = strData.substr(iSeparator + 1);
			if ((iSeparator = strCommand.find(' ')) != std::string::npos)
			{
				strCommand = strCommand.substr(0, iSeparator);
				strParams = strData.substr(strOrigin.length() + strCommand.length() + 2);
			}
		}
		else
		{
			strCommand = strData.substr(0, iSeparator);
			strParams = strData.substr(iSeparator + 1);
		}

		std::string::size_type iLastPos = strParams.find_first_not_of(' ', 0);
		std::string::size_type iPos = strParams.find_first_of(' ', iLastPos);
		while (iPos != std::string::npos || iLastPos != std::string::npos)
		{
			if (strParams[iLastPos != std::string::npos ? iLastPos : 0] == ':')
			{
				vecParams.push_back(strParams.substr(iLastPos + 1));

				iLastPos = strParams.find_first_not_of(' ', iPos);
				iPos = strParams.find_first_of(' ', iLastPos);
				break;
			}

			vecParams.push_back(strParams.substr(iLastPos, iPos - iLastPos));

			iLastPos = strParams.find_first_not_of(' ', iPos);
			iPos = strParams.find_first_of(' ', iLastPos);
		}
	}

	m_pParentBot->HandleData(strOrigin, strCommand, vecParams);

#ifdef ENABLE_RAW_EVENT
	for (CPool<CEventManager *>::iterator i = m_pParentBot->GetParentCore()->GetEventManagers()->begin(); i != m_pParentBot->GetParentCore()->GetEventManagers()->end(); ++i)
	{
		(*i)->OnBotReceivedRaw(m_pParentBot, szData);
	}
	m_pParentBot->GetParentCore()->GetScriptEventManager()->OnBotReceivedRaw(m_pParentBot, szData);
#endif

	if (strCommand == "PING" && vecParams.size() >= 1)
	{
		if (vecParams.size() == 2)
		{
			SendRawFormat("PONG %s %s", vecParams[1].c_str(), vecParams[0].c_str());
		}
		else
		{
			SendRawFormat("PONG %s", vecParams[0].c_str());
		}
	}
	else if (strCommand == "433" && vecParams.size() >= 1 && vecParams[0] == m_pParentBot->GetSettings()->GetNickname())
	{
		m_strCurrentNickname = m_pParentBot->GetSettings()->GetAlternativeNickname();
		SendRawFormat("NICK %s", m_strCurrentNickname.c_str());
	}
	else if (strCommand == "ERROR" && vecParams.size() >= 1)
	{
		printf("[%s] Error: %s\n", GetCurrentNickname(), vecParams[0].c_str());
	}
}

const char *CIrcSocket::GetCurrentNickname()
{
	return m_strCurrentNickname.c_str();
}
