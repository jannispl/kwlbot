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
	m_TcpSocket.Close();
}

bool CIrcSocket::Connect(const char *szHostname, int iPort)
{
	if (!m_TcpSocket.Connect(szHostname, iPort))
	{
		return false;
	}

	m_TcpSocket.SetBlocking(false);

	CIrcSettings *pSettings = m_pParentBot->GetSettings();
	SendRawFormat("NICK %s", pSettings->GetNickname());
	SendRawFormat("USER %s \"\" \"%s\" :%s", pSettings->GetIdent(), szHostname, pSettings->GetRealname());
	m_strCurrentNickname = pSettings->GetNickname();

	return true;
}

int CIrcSocket::SendRaw(const char *szData)
{
	int iRet;
	do
	{
		iRet = m_TcpSocket.Write(szData);
	}
	while (iRet == -1);

	int iRet2;
	do
	{
		iRet2 = m_TcpSocket.Write(IRC_EOL, sizeof(IRC_EOL) - 1);
	}
	while (iRet2 == -1);

	return iRet + iRet2;
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
	return m_TcpSocket.Read(pDest, iSize);
}

void CIrcSocket::Pulse()
{
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
#ifdef _DEBUG
	printf("[in] %s\n", szData);
#endif

	// TODO: Parse prefix/command/params better

	std::string strData(szData);

	std::string::size_type iLastPos = strData.find_first_not_of(' ', 0);
	std::string::size_type iPos = strData.find_first_of(' ', iLastPos);
	std::vector<std::string> vecParts;
	int i = 0;
	while (iPos != std::string::npos || iLastPos != std::string::npos)
	{
		vecParts.push_back(strData.substr(iLastPos, i != 3 ? iPos - iLastPos : std::string::npos));
		if (i == 3)
		{
			break;
		}
		iLastPos = strData.find_first_not_of(' ', iPos);
		iPos = strData.find_first_of(' ', iLastPos);
		++i;
	}
	
	while (i <= 3)
	{
		vecParts.push_back(std::string());
		++i;
	}

	m_pParentBot->HandleData(vecParts);

	for (CPool<CEventManager *>::iterator i = m_pParentBot->GetParentCore()->GetEventManagers()->begin(); i != m_pParentBot->GetParentCore()->GetEventManagers()->end(); ++i)
	{
		(*i)->OnBotReceivedRaw(m_pParentBot, szData);
	}

	if (vecParts[0] == "PING")
	{
		SendRawFormat("PONG %s", vecParts[1].c_str());
		return;
	}

	if (vecParts[1] == "433" && vecParts[3].substr(0, vecParts[3].find(' ')) == m_pParentBot->GetSettings()->GetNickname())
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