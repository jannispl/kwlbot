/*
kwlbot IRC bot


File:		IrcSocket.cpp
Purpose:	Class which manages a connection to an IRC server

*/

#include "StdInc.h"
#include "IrcSocket.h"

CIrcSocket::CIrcSocket(CBot *pParentBot)
{
	m_pParentBot = pParentBot;
}

CIrcSocket::~CIrcSocket()
{
	SendRawFormat("QUIT :%s", m_pParentBot->GetSettings()->GetQuitMessage());
	m_tcpSocket.Close();
}

bool CIrcSocket::Connect(const char *szHostname, int iPort, const char *szPassword)
{
	if (!m_tcpSocket.Connect(szHostname, iPort))
	{
		return false;
	}

	m_tcpSocket.SetBlocking(false);

	CIrcSettings *pSettings = m_pParentBot->GetSettings();
	if (szPassword != NULL && szPassword[0] != '\0')
	{
		SendRawFormat("PASS %s", szPassword);
	}
	SendRawFormat("NICK %s", pSettings->GetNickname());
	SendRawFormat("USER %s \"\" \"%s\" :%s", pSettings->GetIdent(), szHostname, pSettings->GetRealname());
	m_strCurrentNickname = pSettings->GetNickname();

	return true;
}

int CIrcSocket::SendRaw(const char *szData)
{
	size_t iLength = strlen(szData);
	char *pData = (char *)malloc(iLength + sizeof(IRC_EOL));
	strcpy(pData, szData);
	strcat(pData, IRC_EOL);
	m_sendQueue.Add(pData, iLength + sizeof(IRC_EOL) - 1, false, true);

	return m_sendQueue.Process(m_tcpSocket.GetSocket()) ? 0 : -1;
}

int CIrcSocket::SendRawStatic(const char *szData)
{
	m_sendQueue.Add((char *)szData, strlen(szData), false);
	m_sendQueue.Add(IRC_EOL, sizeof(IRC_EOL) - 1, false);

	return m_sendQueue.Process(m_tcpSocket.GetSocket()) ? 0 : -1;
}

int CIrcSocket::SendRawFormat(const char *szFormat, ...)
{
	char szBuffer[IRC_MAX_LEN + 1];

	va_list vaArgs;
	va_start(vaArgs, szFormat);
	vsprintf(szBuffer, szFormat, vaArgs);
	va_end(vaArgs);

	return SendRaw(szBuffer);
}

int CIrcSocket::ReadRaw(char *pDest, size_t iSize)
{
	return m_tcpSocket.Read(pDest, iSize);
}

void CIrcSocket::Pulse()
{
	m_sendQueue.Process(m_tcpSocket.GetSocket());

	char szBuffer[256];
	int iSize = ReadRaw(szBuffer, 255);
	if (iSize == 0)
	{
		// Connection closed
		printf("[%s] Connection closed\n", m_pParentBot->GetSettings()->GetNickname());
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
	printf("[in] %s\n", szData);

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

	m_pParentBot->GetParentCore()->GetScriptEventManager()->OnBotReceivedRaw(m_pParentBot, szData);
	for (CPool<CEventManager *>::iterator i = m_pParentBot->GetParentCore()->GetEventManagers()->begin(); i != m_pParentBot->GetParentCore()->GetEventManagers()->end(); ++i)
	{
		(*i)->OnBotReceivedRaw(m_pParentBot, szData);
	}

	if (strCommand == "PING" && vecParams.size() >= 1)
	{
		SendRawFormat("PONG %s", vecParams[0].c_str());
		return;
	}
	else if (strCommand == "433" && vecParams.size() >= 1 && vecParams[0] == m_pParentBot->GetSettings()->GetNickname())
	{
		m_strCurrentNickname = m_pParentBot->GetSettings()->GetAlternativeNickname();
		SendRawFormat("NICK %s", m_strCurrentNickname.c_str());
		return;
	}
}

const char *CIrcSocket::GetCurrentNickname()
{
	return m_strCurrentNickname.c_str();
}
