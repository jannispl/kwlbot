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
}

bool CIrcSocket::Connect(const char *szHostname, int iPort)
{
	TRACEFUNC("CIrcSocket::Connect");

	if (m_TcpSocket.Connect(szHostname, iPort) != 0)
	{
		return false;
	}

	m_TcpSocket.SetBlocking(false);

	CIrcSettings *pSettings = m_pParentBot->GetSettings();
	SendRawFormat("NICK %s", pSettings->GetNickname());
	SendRawFormat("USER %s \"\" \"%s\" :%s", pSettings->GetIdent(), szHostname, pSettings->GetRealname());

	return true;
}

int CIrcSocket::SendRaw(const char *szData)
{
	TRACEFUNC("CIrcSocket::SendRaw");

	int ret = m_TcpSocket.Write(szData);
	m_TcpSocket.Write(IRC_EOL, sizeof(IRC_EOL) - 1);
	return ret;
}

int CIrcSocket::SendRawFormat(const char *szFormat, ...)
{
	TRACEFUNC("CIrcSocket::SendRawFormat");

	char szBuffer[IRC_MAX_LEN + 1];

	va_list vaArgs;
	va_start(vaArgs, szFormat);
	vsprintf(szBuffer, szFormat, vaArgs);
	va_end(vaArgs);

	return SendRaw(szBuffer);
}

int CIrcSocket::ReadRaw(char *pDest, size_t iSize)
{
	TRACEFUNC("CIrcSocket::ReadRaw");

	return m_TcpSocket.Read(pDest, iSize);
}

void CIrcSocket::Pulse()
{
	TRACEFUNC("CIrcSocket::Pulse");

	static std::string strBuffer;

	char szBuffer[256];
	int i = ReadRaw(szBuffer, 255);
	if (i == 0)
	{
		// connection closed
	}
	else if (i != -1)
	{
		szBuffer[i] = '\0';

		char part[256] = { 0 };
		size_t pos = 0;

		char *cpy = szBuffer;
		while (*cpy)
		{
			if (*cpy == '\n')
			{
				part[pos - 1] = '\0';
				if (strBuffer.empty())
				{
					HandleData(part);
				}
				else
				{
					HandleData((strBuffer + part).c_str());
					strBuffer.clear();
				}
				part[0] = 0;
				pos = 0;
			}
			else
			{
				part[pos++] = *cpy;
			}
			++cpy;
		}
		if (part[0] != 0)
		{
			part[pos] = 0;
			strBuffer += part;
		}
	}
}

void CIrcSocket::HandleData(const char *szData)
{
	TRACEFUNC("CIrcSocket::HandleData");

	printf("[in] %s\n", szData);

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
	
	while (i < 4)
	{
		vecParts.push_back(std::string());
		++i;
	}

	m_pParentBot->HandleData(vecParts);

	if (vecParts[0] == "PING")
	{
		SendRawFormat("PONG %s", vecParts[1].c_str());
		return;
	}
}
