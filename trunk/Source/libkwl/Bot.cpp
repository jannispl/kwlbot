/*
kwlbot IRC bot


File:		Bot.cpp
Purpose:	Class which represents an IRC bot

*/

#include "StdInc.h"
#include "Bot.h"
#include "WildcardMatch.h"

CBot::CBot(CCore *pParentCore, CConfig *pConfig)
	: m_bGotMotd(false), m_bUseNamesX(false), m_pCurrentUser(NULL)
{
	m_pParentCore = pParentCore;
	m_IrcSettings.LoadFromConfig(pConfig);

	m_pIrcSocket = new CIrcSocket(this);
	m_pIrcChannelQueue = new CPool<CIrcChannel *>();

	std::string strTemp;
	if (pConfig->StartValueList("channels"))
	{
		while (pConfig->GetNextValue(&strTemp))
		{
			JoinChannel(strTemp.c_str());
		}
	}

	if (pConfig->StartValueList("scripts"))
	{
		while (pConfig->GetNextValue(&strTemp))
		{
			CScript *pScript = CreateScript(("scripts/" + strTemp).c_str());
		}
	}
	
	if (!pConfig->GetSingleValue("automode", &m_strAutoMode))
	{
		m_strAutoMode.clear();
	}

	int i = 1;
	char szValue[64];
	while (true)
	{
		sprintf(szValue, "access-%d", i);
		if (!pConfig->StartValueList(szValue))
		{
			break;
		}

		AccessRules rules;

		std::string strTemp;
		while (pConfig->GetNextValue(&strTemp))
		{
			std::string::size_type iSeparator = strTemp.find(':');
			if (iSeparator != std::string::npos)
			{
				std::string strKey = strTemp.substr(0, iSeparator);
				std::string strValue = strTemp.substr(iSeparator + 1);

				std::string::size_type iOffset = 0;
				while (*(strValue.begin() + iOffset) == ' ')
				{
					++iOffset;
				}
				strValue = strValue.substr(iOffset);

				if (strKey == "require-host")
				{
					rules.strRequireHost = strValue;
				}
				else if (strKey == "require-nickname")
				{
					rules.strRequireNickname = strValue;
				}
				else if (strKey == "require-ident")
				{
					rules.strRequireIdent = strValue;
				}
			}
		}

		m_vecAccessRules.push_back(rules);

		++i;
	}

	m_bGotMotd = false;

	m_pParentCore->GetScriptEventManager()->OnBotCreated(this);
	for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
	{
		(*i)->OnBotCreated(this);
	}
}

CBot::~CBot()
{
	m_pParentCore->GetScriptEventManager()->OnBotDestroyed(this);
	for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
	{
		(*i)->OnBotDestroyed(this);
	}

	if (m_pIrcSocket != NULL)
	{
		delete m_pIrcSocket;
		m_pIrcSocket = NULL;
	}

	for (CPool<CIrcChannel *>::iterator i = m_plIrcChannels.begin(); i != m_plIrcChannels.end(); ++i)
	{
		delete *i;
	}

	for (CPool<CIrcUser *>::iterator i = m_plGlobalUsers.begin(); i != m_plGlobalUsers.end(); ++i)
	{
		delete *i;
	}

	if (m_pIrcChannelQueue != NULL)
	{
		delete m_pIrcChannelQueue;
		m_pIrcChannelQueue = NULL;
	}
}

CCore *CBot::GetParentCore()
{
	return m_pParentCore;
}

CIrcSettings *CBot::GetSettings()
{
	return &m_IrcSettings;
}

CIrcSocket *CBot::GetSocket()
{
	return m_pIrcSocket;
}

int CBot::SendRaw(const char *szData)
{
#ifdef _DEBUG
	printf("[out] %s\n", szData);
#endif
	return m_pIrcSocket->SendRaw(szData);
}

int CBot::SendRawStatic(const char *szData)
{
	return m_pIrcSocket->SendRawStatic(szData);
}

int CBot::SendRawFormat(const char *szFormat, ...)
{
	char szBuffer[IRC_MAX_LEN + 1];
	va_list vaArgs;
	va_start(vaArgs, szFormat);
	vsnprintf(szBuffer, IRC_MAX_LEN + 1, szFormat, vaArgs);
	va_end(vaArgs);

	return SendRaw(szBuffer);
}

void CBot::Pulse()
{
	m_pIrcSocket->Pulse();
}

CIrcChannel *CBot::FindChannel(const char *szName)
{
	for (CPool<CIrcChannel *>::iterator i = m_plIrcChannels.begin(); i != m_plIrcChannels.end(); ++i)
	{
		if (strcmp(szName, (*i)->GetName()) == 0)
		{
			return *i;
		}
	}
	return NULL;
}

CPool<CIrcChannel *> *CBot::GetChannels()
{
	return &m_plIrcChannels;
}

CIrcUser *CBot::FindUser(const char *szName, bool bCaseSensitive)
{
	typedef int (* Compare_t)(const char *, const char *);
	Compare_t pfnCompare = bCaseSensitive ? strcmp : stricmp;

	for (CPool<CIrcUser *>::iterator i = m_plGlobalUsers.begin(); i != m_plGlobalUsers.end(); ++i)
	{
		if (pfnCompare(szName, (*i)->GetNickname()) == 0)
		{
			return *i;
		}
	}
	return NULL;
}

char CBot::ModeToPrefix(char cMode)
{
	std::string::size_type iIndex = m_SupportSettings.strPrefixes[0].find(cMode);
	if (iIndex == std::string::npos)
	{
		return 0;
	}
	return m_SupportSettings.strPrefixes[1][iIndex];
}

char CBot::PrefixToMode(char cPrefix)
{
	std::string::size_type iIndex = m_SupportSettings.strPrefixes[1].find(cPrefix);
	if (iIndex == std::string::npos)
	{
		return 0;
	}
	return m_SupportSettings.strPrefixes[0][iIndex];
}

bool CBot::IsPrefixMode(char cMode)
{
	return ModeToPrefix(cMode) != 0;
}

char CBot::GetModeGroup(char cMode)
{
	for (int i = 0; i < 4; ++i)
	{
		std::string::size_type iPos = m_SupportSettings.strChanmodes[i].find(cMode);
		if (iPos != std::string::npos)
		{
			return i + 1;
		}
	}
	return 0;
}

void CBot::HandleData(const std::string &strOrigin, const std::string &strCommand, const std::vector<std::string> &vecParams)
{
	int iParamCount = vecParams.size();
	int iNumeric;

	if (strCommand.length() == 3)
	{
		iNumeric = 1;
		for (std::string::const_iterator i = strCommand.begin(); i != strCommand.end(); ++i)
		{
			if (*i < '0' || *i > '9')
			{
				iNumeric = 0;
				break;
			}
		}

		if (iNumeric == 1)
		{
			iNumeric = atoi(strCommand.c_str());
		}
	}
	else
	{
		iNumeric = 0;
	}

	if (!strOrigin.empty())
	{
		std::string::size_type iSeparator = strOrigin.find('!');
		if (iSeparator == std::string::npos)
		{
			m_strCurrentNickname = strOrigin;
			m_strCurrentIdent.clear();
			m_strCurrentHostname.clear();
		}
		else
		{
			m_strCurrentNickname = strOrigin.substr(0, iSeparator);
			std::string::size_type iSeparator2 = strOrigin.find('@', iSeparator);
			if (iSeparator2 == std::string::npos)
			{
				m_strCurrentIdent = strOrigin.substr(iSeparator + 1);
				m_strCurrentHostname.clear();
			}
			else
			{
				m_strCurrentIdent = strOrigin.substr(iSeparator + 1, iSeparator2 - iSeparator - 1);
				m_strCurrentHostname = strOrigin.substr(iSeparator2 + 1);
			}
		}

		if (iNumeric == 0) // A numeric reply is not allowed to originate from a client (RFC 2812)
		{
			m_pCurrentUser = FindUser(m_strCurrentNickname.c_str());
			if (m_pCurrentUser != NULL)
			{
				m_pCurrentUser->UpdateIfNecessary(m_strCurrentIdent.c_str(), m_strCurrentHostname.c_str());
			}
		}
	}

	if (!m_bGotMotd)
	{
		if (iNumeric == 376 || iNumeric == 422)
		{
			m_bGotMotd = true;

			if (!m_strAutoMode.empty())
			{
				SendRawFormat("MODE %s %s", m_pIrcSocket->GetCurrentNickname(), m_strAutoMode.c_str());
			}

			m_pParentCore->GetScriptEventManager()->OnBotConnected(this);
			for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
			{
				(*i)->OnBotConnected(this);
			}

			for (CPool<CIrcChannel *>::iterator i = m_pIrcChannelQueue->begin(); i != m_pIrcChannelQueue->end(); ++i)
			{
				SendRawFormat("JOIN %s", (*i)->GetName());
				delete *i;
			}
			delete m_pIrcChannelQueue;
			m_pIrcChannelQueue = NULL;

			return;
		}
	}

	/* When getting the sign that the bot joined a channel, the server should also
	send a NAMES reply afterwards. We urgently need a channel's user list before
	calling the onBotJoinedChannel event */

	if ((iNumeric == 0 || (iNumeric != 353 /* NAMES */ && iNumeric != 366 /* End of NAMES */ && iNumeric != 332 /* Topic */ && iNumeric != 333 /* Topic Timestamp */)) && m_plIncompleteChannels.size() != 0) // If we did not get a NAMES reply right after the JOIN message
	{
		for (CPool<CIrcChannel *>::iterator i = m_plIncompleteChannels.begin(); i != m_plIncompleteChannels.end(); ++i)
		{
			m_pParentCore->GetScriptEventManager()->OnBotJoinedChannel(this, *i);
			for (CPool<CEventManager *>::iterator j = m_pParentCore->GetEventManagers()->begin(); j != m_pParentCore->GetEventManagers()->end(); ++j)
			{
				(*j)->OnBotJoinedChannel(this, *i);
			}

			i = m_plIncompleteChannels.erase(i);
			if (i == m_plIncompleteChannels.end())
			{
				break;
			}
		}
	}

	if (iParamCount == 8)
	{
		// WHO reply
		//:ircd 352 botname #channel ident host ircd nickname IDK :hopcount realname
		if (iNumeric == 352)
		{
			Handle352(vecParams[5], vecParams[1], vecParams[2], vecParams[3]);
			return;
		}
	}
	else if (iParamCount == 4)
	{
		if (iNumeric == 353)
		{
			Handle353(vecParams[2], vecParams[3]);
			return;
		}

		if (iNumeric == 333)
		{
			Handle333(vecParams[1], vecParams[2],
#ifdef WIN32
				_atoi64(vecParams[3].c_str())
#else
				strtoll(vecParams[3].c_str(), NULL, 10)
#endif
				);
			return;
		}
	}
	else if (iParamCount == 3)
	{
		if (strCommand == "KICK")
		{
			HandleKICK(vecParams[0], vecParams[1], vecParams[2]);
			return;
		}

		if (iNumeric == 332)
		{
			std::string strChannel = vecParams[1];

			CIrcChannel *pChannel = FindChannel(strChannel.c_str());
			if (pChannel == NULL)
			{
				return;
			}

			pChannel->m_topicInfo.strTopicSetBy = m_strCurrentNickname;
			pChannel->m_topicInfo.strTopic = vecParams[2];
			return;
		}
	}
	else if (iParamCount == 2)
	{
		if (iNumeric == 366)
		{
			std::string strChannel = vecParams[0];

			CIrcChannel *pChannel = FindChannel(strChannel.c_str());
			if (pChannel != NULL)
			{		
				for (CPool<CIrcChannel *>::iterator i = m_plIncompleteChannels.begin(); i != m_plIncompleteChannels.end(); ++i)
				{
					if (*i == pChannel)
					{
						m_pParentCore->GetScriptEventManager()->OnBotJoinedChannel(this, pChannel);
						for (CPool<CEventManager *>::iterator j = m_pParentCore->GetEventManagers()->begin(); j != m_pParentCore->GetEventManagers()->end(); ++j)
						{
							(*j)->OnBotJoinedChannel(this, pChannel);
						}

						i = m_plIncompleteChannels.erase(i);
						break;
					}
				}
			}
			return;
		}

		if (strCommand == "PART")
		{
			HandlePART(vecParams[0], vecParams[1]);
			return;
		}

		if (strCommand == "KICK")
		{
			HandleKICK(vecParams[0], vecParams[1]);
			return;
		}

		if (strCommand == "PRIVMSG")
		{
			HandlePRIVMSG(vecParams[0], vecParams[1]);
			return;
		}

		if (strCommand == "TOPIC")
		{
			std::string strChannel = vecParams[0];

			CIrcChannel *pChannel = FindChannel(strChannel.c_str());
			if (pChannel == NULL)
			{
				return;
			}

			pChannel->m_topicInfo.ullTopicSetDate = time(NULL);
			pChannel->m_topicInfo.strTopicSetBy = m_strCurrentNickname;
			pChannel->m_topicInfo.strTopic = vecParams[1];
			return;
		}
	}
	else if (iParamCount == 1)
	{
		if (strCommand == "JOIN")
		{
			HandleJOIN(vecParams[0]);
			return;
		}

		if (strCommand == "PART")
		{
			HandlePART(vecParams[0]);
			return;
		}

		if (strCommand == "QUIT")
		{
			HandleQUIT(vecParams[0]);
			return;
		}

		if (strCommand == "NICK")
		{
			HandleNICK(vecParams[0]);
			return;
		}
	}
	else if (iParamCount == 0)
	{
		if (strCommand == "QUIT")
		{
			HandleQUIT();
			return;
		}
	}

	if (strCommand == "005")
	{
		for (int i = 1; i < iParamCount; ++i)
		{
			std::string strSupport = vecParams[i];
			if (strSupport == "NAMESX")
			{
				// Tell the server we support NAMESX
				SendRawStatic("PROTOCTL NAMESX");
				m_bUseNamesX = true;
			}

			std::string::size_type iEqual = strSupport.find('=');
			if (iEqual != std::string::npos)
			{
				std::string strValue = strSupport.substr(iEqual + 1);
				if (strSupport.length() > 10 && strSupport.substr(0, 10) == "CHANMODES=")
				{
					std::string::size_type iLastPos = strValue.find_first_not_of(',', 0);
					std::string::size_type iPos = strValue.find_first_of(',', iLastPos);
					int i = 0;
					while (i < 4 && (iPos != std::string::npos || iLastPos != std::string::npos))
					{
						m_SupportSettings.strChanmodes[i] = strValue.substr(iLastPos, iPos - iLastPos);

						iLastPos = strValue.find_first_not_of(',', iPos);
						iPos = strValue.find_first_of(',', iLastPos);
						++i;
					}
				}
				else if (strSupport.length() > 7 && strSupport.substr(0, 7) == "PREFIX=")
				{
					strValue = strValue.substr(1);
					std::string::size_type iEnd = strValue.find(')');
					m_SupportSettings.strPrefixes[0] = strValue.substr(0, iEnd);
					m_SupportSettings.strPrefixes[1] = strValue.substr(iEnd + 1);
				}
			}
		}
		return;
	}

	if (strCommand == "MODE")
	{
		HandleMODE(vecParams[0], vecParams[1], vecParams);
		return;
	}
}

void CBot::Handle352(const std::string &strNickname, const std::string &strChannel, const std::string &strIdent, const std::string &strHost)
{
	CIrcUser *pUser = FindUser(strNickname.c_str());
	if (pUser != NULL)
	{
		pUser->UpdateIfNecessary(m_strCurrentIdent.c_str(), m_strCurrentHostname.c_str());
	}

	CIrcChannel *pChannel = FindChannel(strChannel.c_str());
	if (pChannel != NULL && !pChannel->m_bHasDetailedUsers)
	{
		pChannel->m_bHasDetailedUsers = true;

		m_pParentCore->GetScriptEventManager()->OnBotGotChannelUserList(this, pChannel);
		for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
		{
			(*i)->OnBotGotChannelUserList(this, pChannel);
		}
	}
}

void CBot::Handle353(const std::string &strChannel, const std::string &strNames)
{
	printf("Handle353(\"%s\", \"%s\");\n", strChannel.c_str(), strNames.c_str());

	CIrcChannel *pChannel = FindChannel(strChannel.c_str());
	if (pChannel != NULL)
	{
		std::string::size_type iLastPos = strNames.find_first_not_of(' ', 0);
		std::string::size_type iPos = strNames.find_first_of(' ', iLastPos);
		while (iPos != std::string::npos || iLastPos != std::string::npos)
		{
			std::string strName = strNames.substr(iLastPos, iPos - iLastPos);
			
			char cMode = 0;
			bool bBreak = false;
			std::string::size_type iOffset = 0;
			for (std::string::iterator i = strName.begin(); i != strName.end(); ++i)
			{
				switch (*i)
				{
				case '~':
					cMode |= 32;
					++iOffset;
					break;
				case '&':
					cMode |= 16;
					++iOffset;
					break;
				case '@':
					cMode |= 8;
					++iOffset;
					break;
				case '%':
					cMode |= 4;
					++iOffset;
					break;
				case '+':
					cMode |= 2;
					++iOffset;
					break;
				default:
					bBreak = true;
					break;
				}

				// If the server does not support NAMESX, consider any other mode chars part of the nickname.
				if (!m_bUseNamesX || bBreak)
				{
					break;
				}
			}
			
			if (iOffset != 0)
			{
				strName = strName.substr(iOffset);
			}

			CIrcUser *pUser = FindUser(strName.c_str());
			if (strName != m_pIrcSocket->GetCurrentNickname())
			{
				if (pUser == NULL)
				{
					pUser = new CIrcUser(this, strName.c_str());
					m_plGlobalUsers.push_back(pUser);
				}
				pUser->m_plIrcChannels.push_back(pChannel);
				pUser->m_mapChannelModes[pChannel] = cMode;

				pChannel->m_plIrcUsers.push_back(pUser);
			}

			iLastPos = strNames.find_first_not_of(' ', iPos);
			iPos = strNames.find_first_of(' ', iLastPos);
		}
	}
}

void CBot::Handle333(const std::string &strChannel, const std::string &strTopicSetBy, time_t ullSetDate)
{
	CIrcChannel *pChannel = FindChannel(strChannel.c_str());
	if (pChannel == NULL)
	{
		return;
	}

	pChannel->m_topicInfo.strTopicSetBy = strTopicSetBy;
	pChannel->m_topicInfo.ullTopicSetDate = ullSetDate;
}

void CBot::HandleKICK(const std::string &strChannel, const std::string &strVictim, const std::string &strReason)
{
	CIrcChannel *pChannel = FindChannel(strChannel.c_str());
	if (pChannel != NULL)
	{
		if (strVictim == m_pIrcSocket->GetCurrentNickname())
		{
			for (CPool<CIrcUser *>::iterator i = pChannel->m_plIrcUsers.begin(); i != pChannel->m_plIrcUsers.end(); ++i)
			{
				if ((*i)->m_plIrcChannels.size() == 1)
				{
					m_plGlobalUsers.remove(*i);
					delete *i;
				}
				i = pChannel->m_plIrcUsers.erase(i);
				if (i == pChannel->m_plIrcUsers.end())
				{
					break;
				}
			}

			m_plIrcChannels.remove(pChannel);
			delete pChannel;
		}
		else
		{
			CIrcUser *pVictim = FindUser(strVictim.c_str());

			if (pVictim != NULL)
			{
				if (m_pCurrentUser != NULL)
				{
					m_pParentCore->GetScriptEventManager()->OnUserKickedUser(this, m_pCurrentUser, pVictim, pChannel, strReason.c_str());
					for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
					{
						(*i)->OnUserKickedUser(this, m_pCurrentUser, pVictim, pChannel, strReason.c_str());
					}
				}
				else
				{
					CIrcUser tempUser(this, m_strCurrentNickname.c_str(), true);
					tempUser.UpdateIfNecessary(m_strCurrentIdent.c_str(), m_strCurrentHostname.c_str());

					m_pParentCore->GetScriptEventManager()->OnUserKickedUser(this, &tempUser, pVictim, pChannel, strReason.c_str());
					for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
					{
						(*i)->OnUserKickedUser(this, &tempUser, pVictim, pChannel, strReason.c_str());
					}
				}

				pVictim->m_plIrcChannels.remove(pChannel);
				pChannel->m_plIrcUsers.remove(pVictim);
				if (pVictim->m_plIrcChannels.size() == 0)
				{
					m_plGlobalUsers.remove(pVictim);
					delete pVictim;
				}
			}
			/*
			else
			{
				// The victim *must* be on the channel to be kicked, so we'll not handle this
			}
			*/
		}
	}
}

void CBot::HandlePART(const std::string &strChannel, const std::string &strReason)
{
	CIrcChannel *pChannel = FindChannel(strChannel.c_str());
	if (pChannel != NULL)
	{
		if (m_strCurrentNickname == m_pIrcSocket->GetCurrentNickname())
		{
			for (CPool<CIrcUser *>::iterator i = pChannel->m_plIrcUsers.begin(); i != pChannel->m_plIrcUsers.end(); ++i)
			{
				if ((*i)->m_plIrcChannels.size() == 1)
				{
					m_plGlobalUsers.remove(*i);
					delete *i;
				}

				i = pChannel->m_plIrcUsers.erase(i);
				if (i == pChannel->m_plIrcUsers.end())
				{
					break;
				}
			}

			m_plIrcChannels.remove(pChannel);
			delete pChannel;
		}
		else
		{
			if (m_pCurrentUser != NULL)
			{
				m_pParentCore->GetScriptEventManager()->OnUserLeftChannel(this, m_pCurrentUser, pChannel, strReason.c_str());
				for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
				{
					(*i)->OnUserLeftChannel(this, m_pCurrentUser, pChannel, strReason.c_str());
				}

				m_pCurrentUser->m_plIrcChannels.remove(pChannel);
				pChannel->m_plIrcUsers.remove(m_pCurrentUser);
				if (m_pCurrentUser->m_plIrcChannels.size() == 0)
				{
					m_plGlobalUsers.remove(m_pCurrentUser);
					delete m_pCurrentUser;
				}
			}
			else
			{
				CIrcUser tempUser(this, m_strCurrentNickname.c_str(), true);
				tempUser.UpdateIfNecessary(m_strCurrentIdent.c_str(), m_strCurrentHostname.c_str());

				m_pParentCore->GetScriptEventManager()->OnUserLeftChannel(this, &tempUser, pChannel, strReason.c_str());
				for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
				{
					(*i)->OnUserLeftChannel(this, &tempUser, pChannel, strReason.c_str());
				}
			}
		}
	}
}

void CBot::HandlePRIVMSG(const std::string &strTarget, const std::string &strMessage)
{
	if (m_pCurrentUser != NULL)
	{
		if (strTarget == m_pIrcSocket->GetCurrentNickname())
		{
			// privmsg to bot
			m_pParentCore->GetScriptEventManager()->OnUserPrivateMessage(this, m_pCurrentUser, strMessage.c_str());
			for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
			{
				(*i)->OnUserPrivateMessage(this, m_pCurrentUser, strMessage.c_str());
			}
		}
		else
		{
			CIrcChannel *pChannel = FindChannel(strTarget.c_str());
			if (pChannel != NULL)
			{
				m_pParentCore->GetScriptEventManager()->OnUserChannelMessage(this, m_pCurrentUser, pChannel, strMessage.c_str());
				for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
				{
					(*i)->OnUserChannelMessage(this, m_pCurrentUser, pChannel, strMessage.c_str());
				}
			}
		}
	}
	else
	{
		CIrcUser tempUser(this, m_strCurrentNickname.c_str(), true);
		tempUser.UpdateIfNecessary(m_strCurrentIdent.c_str(), m_strCurrentHostname.c_str());

		if (strTarget == m_pIrcSocket->GetCurrentNickname())
		{
			m_pParentCore->GetScriptEventManager()->OnUserPrivateMessage(this, &tempUser, strMessage.c_str());
			for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
			{
				(*i)->OnUserPrivateMessage(this, &tempUser, strMessage.c_str());
			}
		}
		else
		{
			CIrcChannel *pChannel = FindChannel(strTarget.c_str());
			if (pChannel != NULL)
			{
				m_pParentCore->GetScriptEventManager()->OnUserChannelMessage(this, &tempUser, pChannel, strMessage.c_str());
				for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
				{
					(*i)->OnUserChannelMessage(this, &tempUser, pChannel, strMessage.c_str());
				}
			}
		}
	}
}

void CBot::HandleJOIN(const std::string &strChannel)
{
	CIrcChannel *pChannel = NULL;
	bool bSelf = false;

	if (m_strCurrentNickname == m_pIrcSocket->GetCurrentNickname())
	{
		pChannel = new CIrcChannel(this, strChannel.c_str());
		m_plIrcChannels.push_back(pChannel);

		SendRawFormat("WHO %s", strChannel.c_str());

		bSelf = true;
	}
	else
	{
		pChannel = FindChannel(strChannel.c_str());
	}

	if (pChannel != NULL)
	{
		if (m_pCurrentUser == NULL)
		{
			m_pCurrentUser = new CIrcUser(this, m_strCurrentNickname.c_str());
			m_plGlobalUsers.push_back(m_pCurrentUser);
		}

		m_pCurrentUser->m_plIrcChannels.push_back(pChannel);
		pChannel->m_plIrcUsers.push_back(m_pCurrentUser);

		if (bSelf)
		{
			// Add this channel to our list of channels with an incomplete list of users
			m_plIncompleteChannels.push_back(pChannel);
		}
		else
		{
			m_pParentCore->GetScriptEventManager()->OnUserJoinedChannel(this, m_pCurrentUser, pChannel);
			for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
			{
				(*i)->OnUserJoinedChannel(this, m_pCurrentUser, pChannel);
			}
		}
	}
}

void CBot::HandleQUIT(const std::string &strReason)
{
	if (m_pCurrentUser != NULL)
	{
		m_pParentCore->GetScriptEventManager()->OnUserQuit(this, m_pCurrentUser, strReason.c_str());
		for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
		{
			(*i)->OnUserQuit(this, m_pCurrentUser, strReason.c_str());
		}

		for (CPool<CIrcChannel *>::iterator i = m_plIrcChannels.begin(); i != m_plIrcChannels.end(); ++i)
		{
			(*i)->m_plIrcUsers.remove(m_pCurrentUser);
		}

		m_plGlobalUsers.remove(m_pCurrentUser);
		delete m_pCurrentUser;
	}
	else
	{
		CIrcUser tempUser(this, m_strCurrentNickname.c_str(), true);
		tempUser.UpdateIfNecessary(m_strCurrentIdent.c_str(), m_strCurrentHostname.c_str());

		m_pParentCore->GetScriptEventManager()->OnUserQuit(this, &tempUser, strReason.c_str());
		for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
		{
			(*i)->OnUserQuit(this, &tempUser, strReason.c_str());
		}
	}
}

void CBot::HandleNICK(const std::string &strNewNickname)
{
	if (m_strCurrentNickname == m_pIrcSocket->GetCurrentNickname())
	{
		m_pIrcSocket->m_strCurrentNickname = strNewNickname;
		return;
	}

	if (m_pCurrentUser != NULL)
	{
		m_pCurrentUser->UpdateNickname(strNewNickname.c_str());

		m_pParentCore->GetScriptEventManager()->OnUserChangedNickname(this, m_pCurrentUser, m_strCurrentNickname.c_str());
		for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
		{
			(*i)->OnUserChangedNickname(this, m_pCurrentUser, m_strCurrentNickname.c_str());
		}
	}
	else
	{
		CIrcUser tempUser(this, strNewNickname.c_str(), true);
		tempUser.UpdateIfNecessary(m_strCurrentIdent.c_str(), m_strCurrentHostname.c_str());

		m_pParentCore->GetScriptEventManager()->OnUserChangedNickname(this, &tempUser, m_strCurrentNickname.c_str());
		for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
		{
			(*i)->OnUserChangedNickname(this, &tempUser, m_strCurrentNickname.c_str());
		}
	}
}

void CBot::HandleMODE(const std::string &strChannel, const std::string &strModes, const std::vector<std::string> &vecParams)
{
	CIrcChannel *pChannel = FindChannel(strChannel.c_str());
	if (pChannel == NULL)
	{
		return;
	}

	int iSet = 0;
	int iParamOffset = 2;
	std::string strParams;
	for (std::string::const_iterator i = strModes.begin(); i != strModes.end(); ++i)
	{
		char cMode = *i;
		switch (cMode)
		{
		case '+':
			iSet = 1;
			break;
		case '-':
			iSet = 2;
			break;
		}

		if (iSet == 0 || cMode == '+' || cMode == '-')
		{
			continue;
		}

		char cGroup = 0;
		bool bPrefixMode = false;

		// Modes in PREFIX are not listed but could be considered type B.
		if (IsPrefixMode(cMode))
		{
			cGroup = 2;
			bPrefixMode = true;
		}
		else
		{
			cGroup = GetModeGroup(cMode);
		}

		switch (cGroup)
		{
			case 1:
			case 2: // Always parameter
			{
				if ((int)vecParams.size() >= iParamOffset + 1)
				{
					std::string strParam = vecParams[iParamOffset++];
					if (strParams.empty())
					{
						strParams = strParam;
					}
					else
					{
						strParams += " " + strParam;
					}

					if (bPrefixMode && pChannel != NULL)
					{
						CIrcUser *pUser = FindUser(strParam.c_str());

						if (pUser != NULL)
						{
							char cNewMode = 0;
							char cPrefix = ModeToPrefix(cMode);
							switch (cPrefix)
							{
							case '~':
								cNewMode = 32;
								break;
							case '&':
								cNewMode = 16;
								break;
							case '@':
								cNewMode = 8;
								break;
							case '%':
								cNewMode = 4;
								break;
							case '+':
								cNewMode = 2;
								break;
							}

							if (iSet == 1)
							{
								pUser->m_mapChannelModes[pChannel] |= cNewMode;
							}
							else
							{
								pUser->m_mapChannelModes[pChannel] &= ~cNewMode;
							}
						}
					}
				}
				break;
			}

			case 3: // Parameter if iSet == 1
			{
				if (iSet == 1 && (int)vecParams.size() >= iParamOffset + 1)
				{
					std::string strParam = vecParams[iParamOffset++];
					if (strParams.empty())
					{
						strParams = strParam;
					}
					else
					{
						strParams += " " + strParam;
					}
				}
				else if (iSet == 2)
				{
				}
				break;
			}

			case 4: // No parameter
			{
				break;
			}
		}
	}
	
	if (m_pCurrentUser != NULL)
	{
		m_pParentCore->GetScriptEventManager()->OnUserSetChannelModes(this, m_pCurrentUser, pChannel, strModes.c_str(), strParams.c_str());
		for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
		{
			(*i)->OnUserSetChannelModes(this, m_pCurrentUser, pChannel, strModes.c_str(), strParams.c_str());
		}
	}
	else
	{
		CIrcUser tempUser(this, m_strCurrentNickname.c_str(), true);
		tempUser.UpdateIfNecessary(m_strCurrentIdent.c_str(), m_strCurrentHostname.c_str());

		m_pParentCore->GetScriptEventManager()->OnUserSetChannelModes(this, &tempUser, pChannel, strModes.c_str(), strParams.c_str());
		for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
		{
			(*i)->OnUserSetChannelModes(this, &tempUser, pChannel, strModes.c_str(), strParams.c_str());
		}
	}
}

bool CBot::TestAccessLevel(CIrcUser *pUser, int iLevel)
{
	if (iLevel < 1 || iLevel > (int)m_vecAccessRules.size())
	{
		return false;
	}

	AccessRules accessRules = m_vecAccessRules[iLevel - 1];

	if (!accessRules.strRequireHost.empty() && !wildcmp(accessRules.strRequireHost.c_str(), pUser->GetHostname()))
	{
		return false;
	}

	if (!accessRules.strRequireIdent.empty() && !wildcmp(accessRules.strRequireIdent.c_str(), pUser->GetIdent()))
	{
		return false;
	}

	if (!accessRules.strRequireNickname.empty() && !wildcmp(accessRules.strRequireNickname.c_str(), pUser->GetNickname()))
	{
		return false;
	}

	return true;
}

int CBot::GetNumAccessLevels()
{
	return m_vecAccessRules.size();
}

CScript *CBot::CreateScript(const char *szFilename)
{
	CScript *pScript = new CScript(this);
	if (!pScript->Load(m_pParentCore, szFilename))
	{
		return NULL;
	}

	m_plScripts.push_back(pScript);
	return pScript;
}

bool CBot::DeleteScript(CScript *pScript)
{
	if (pScript == NULL)
	{
		return false;
	}

	m_plScripts.remove(pScript);
	delete pScript;
	return true;
}

CPool<CScript *> *CBot::GetScripts()
{
	return &m_plScripts;
}

void CBot::JoinChannel(const char *szChannel)
{
	CIrcChannel *pChannel = new CIrcChannel(this, szChannel);
	if (m_bGotMotd)
	{
		SendRawFormat("JOIN %s", szChannel);
	}
	else
	{
		m_pIrcChannelQueue->push_back(pChannel);
	}
}

bool CBot::LeaveChannel(CIrcChannel *pChannel, const char *szReason)
{
	bool bFound = false;
	for (CPool<CIrcChannel *>::iterator i = m_plIrcChannels.begin(); i != m_plIrcChannels.end(); ++i)
	{
		if (*i == pChannel)
		{
			bFound = true;
			break;
		}
	}

	if (!bFound)
	{
		return false;
	}

	if (szReason != NULL)
	{
		SendRawFormat("PART %s :%s", pChannel->GetName(), szReason);
	}
	else
	{
		SendRawFormat("PART %s", pChannel->GetName());
	}

	return true;
}

void CBot::SendMessage(const char *szTarget, const char *szMessage)
{
	SendRawFormat("PRIVMSG %s :%s", szTarget, szMessage);
}

void CBot::SendNotice(const char *szTarget, const char *szMessage)
{
	SendRawFormat("NOTICE %s :%s", szTarget, szMessage);
}

CScriptObject::eScriptType CBot::GetType()
{
	return Bot;
}
