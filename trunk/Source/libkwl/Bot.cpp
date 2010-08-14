/*
kwlbot IRC bot


File:		Bot.cpp
Purpose:	Class which represents an IRC bot

*/

#include "StdInc.h"
#include "Bot.h"
#include "WildcardMatch.h"

CBot::CBot(CCore *pParentCore, CConfig *pConfig)
	: m_bGotMotd(false)
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

int CBot::SendRawFormat(const char *szFormat, ...)
{
	char szBuffer[IRC_MAX_LEN + 1];
	va_list vaArgs;
	va_start(vaArgs, szFormat);
	vsprintf(szBuffer, szFormat, vaArgs);
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

	CIrcUser *pUser = NULL;
	std::string strNickname, strIdent, strHostname;
	if (!strOrigin.empty())
	{
		std::string::size_type iSeparator = strOrigin.find('!');
		if (iSeparator == std::string::npos)
		{
			strNickname = strOrigin;
		}
		else
		{
			strNickname = strOrigin.substr(0, iSeparator);
			std::string::size_type iSeparator2 = strOrigin.find('@', iSeparator);
			if (iSeparator2 == std::string::npos)
			{
				strIdent = strOrigin.substr(iSeparator + 1);
			}
			else
			{
				strIdent = strOrigin.substr(iSeparator + 1, iSeparator2 - iSeparator - 1);
				strHostname = strOrigin.substr(iSeparator2 + 1);
			}
		}

		if (iNumeric == 0) // A numeric reply is not allowed to originate from a client (RFC 2812)
		{
			pUser = FindUser(strNickname.c_str());
			if (pUser != NULL)
			{
				pUser->UpdateIfNecessary(strIdent.c_str(), strHostname.c_str());
			}
		}
	}

	if (!m_bGotMotd)
	{
		if (iNumeric != 0 && (iNumeric == 376 || iNumeric == 422))
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
			std::string strChannel = vecParams[1];
			std::string strIdent = vecParams[2];
			std::string strHost = vecParams[3];

			pUser = FindUser(vecParams[5].c_str());
			if (pUser != NULL)
			{
				pUser->UpdateIfNecessary(strIdent.c_str(), strHost.c_str());
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

			return;
		}
	}
	else if (iParamCount == 4)
	{
		if (iNumeric == 353)
		{
			std::string strChannel = vecParams[2];

			CIrcChannel *pChannel = FindChannel(strChannel.c_str());
			if (pChannel != NULL)
			{
				std::string strNames = vecParams[3];

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
						if (bBreak)
						{
							break;
						}
					}
					
					if (iOffset != 0)
					{
						strName = strName.substr(iOffset);
					}

					pUser = FindUser(strName.c_str());
					if (strName != m_pIrcSocket->GetCurrentNickname())
					{
						if (pUser == NULL)
						{
							dbgprintf("We don't know %s yet, adding him (1).\n", strName.c_str());
							pUser = new CIrcUser(this, strName.c_str());
							m_plGlobalUsers.push_back(pUser);
						}
						else
						{
							dbgprintf("We already know %s.\n", pUser->GetNickname());
						}
						pUser->m_plIrcChannels.push_back(pChannel);
						pUser->m_mapChannelModes[pChannel] = cMode;

						pChannel->m_plIrcUsers.push_back(pUser);
					}

					iLastPos = strNames.find_first_not_of(' ', iPos);
					iPos = strNames.find_first_of(' ', iLastPos);
				}
			}
			return;
		}

		if (iNumeric == 333)
		{
			std::string strChannel = vecParams[1];

			CIrcChannel *pChannel = FindChannel(strChannel.c_str());
			if (pChannel == NULL)
			{
				return;
			}

			pChannel->m_topicInfo.strTopicSetBy = vecParams[2];
#ifdef WIN32
			pChannel->m_topicInfo.ullTopicSetDate = _atoi64(vecParams[3].c_str());
#else
			pChannel->m_topicInfo.ullTopicSetDate = strtoll(vecParams[3].c_str(), NULL, 10);
#endif
			return;
		}
	}
	else if (iParamCount == 3)
	{
		if (strCommand == "KICK")
		{
			std::string strChannel = vecParams[0];
			std::string strVictim = vecParams[1];
			std::string strReason = vecParams[2];

			CIrcChannel *pChannel = FindChannel(strChannel.c_str());
			if (pChannel != NULL)
			{
				if (strVictim == m_pIrcSocket->GetCurrentNickname())
				{
					dbgprintf("WE WERE KICKED FROM %s.\n", pChannel->GetName());

					for (CPool<CIrcUser *>::iterator i = pChannel->m_plIrcUsers.begin(); i != pChannel->m_plIrcUsers.end(); ++i)
					{
						if ((*i)->m_plIrcChannels.size() == 1)
						{
							dbgprintf("We lost %s.\n", (*i)->GetNickname());
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
						if (pUser != NULL)
						{
							pUser->UpdateIfNecessary(strIdent.c_str(), strHostname.c_str());

							m_pParentCore->GetScriptEventManager()->OnUserKickedUser(this, pUser, pVictim, pChannel, strReason.c_str());
							for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
							{
								(*i)->OnUserKickedUser(this, pUser, pVictim, pChannel, strReason.c_str());
							}
						}
						else
						{
							CIrcUser tempUser(this, strNickname.c_str(), true);
							tempUser.UpdateIfNecessary(strIdent.c_str(), strHostname.c_str());

							m_pParentCore->GetScriptEventManager()->OnUserKickedUser(this, &tempUser, pVictim, pChannel, strReason.c_str());
							for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
							{
								(*i)->OnUserKickedUser(this, &tempUser, pVictim, pChannel, strReason.c_str());
							}
						}

						dbgprintf("Removing %s from %s.\n", pVictim->GetNickname(), pChannel->GetName());
						pVictim->m_plIrcChannels.remove(pChannel);
						pChannel->m_plIrcUsers.remove(pVictim);
						if (pVictim->m_plIrcChannels.size() == 0)
						{
							dbgprintf("We don't know %s anymore.\n", pVictim->GetNickname());
							m_plGlobalUsers.remove(pVictim);
							delete pVictim;
						}
					}
					/* (The victim *must* be on the channel to be kicked, so we'll not handle this)
					else
					{
					}
					*/
				}
			}
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

			pChannel->m_topicInfo.strTopicSetBy = strNickname;
			pChannel->m_topicInfo.strTopic = vecParams[2];
			return;
		}
	}
	else if (iParamCount == 2)
	{
		if (strCommand == "366")
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
			std::string strChannel = vecParams[0];
			std::string strReason = vecParams[1];

			CIrcChannel *pChannel = FindChannel(strChannel.c_str());
			if (pChannel != NULL)
			{
				if (strNickname == m_pIrcSocket->GetCurrentNickname())
				{
					dbgprintf("WE LEFT %s.\n", pChannel->GetName());

					for (CPool<CIrcUser *>::iterator i = pChannel->m_plIrcUsers.begin(); i != pChannel->m_plIrcUsers.end(); ++i)
					{
						if ((*i)->m_plIrcChannels.size() == 1)
						{
							dbgprintf("We lost %s.\n", (*i)->GetNickname());
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
					if (pUser != NULL)
					{
						pUser->UpdateIfNecessary(strIdent.c_str(), strHostname.c_str());

						m_pParentCore->GetScriptEventManager()->OnUserLeftChannel(this, pUser, pChannel, strReason.c_str());
						for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
						{
							(*i)->OnUserLeftChannel(this, pUser, pChannel, strReason.c_str());
						}

						dbgprintf("Removing %s from %s.\n", pUser->GetNickname(), pChannel->GetName());
						pUser->m_plIrcChannels.remove(pChannel);
						pChannel->m_plIrcUsers.remove(pUser);
						if (pUser->m_plIrcChannels.size() == 0)
						{
							dbgprintf("We don't know %s anymore.\n", pUser->GetNickname());
							m_plGlobalUsers.remove(pUser);
							delete pUser;
						}
					}
					else
					{
						CIrcUser tempUser(this, strNickname.c_str(), true);
						tempUser.UpdateIfNecessary(strIdent.c_str(), strHostname.c_str());

						m_pParentCore->GetScriptEventManager()->OnUserLeftChannel(this, &tempUser, pChannel, strReason.c_str());
						for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
						{
							(*i)->OnUserLeftChannel(this, &tempUser, pChannel, strReason.c_str());
						}
					}
				}
			}
			return;
		}

		if (strCommand == "KICK")
		{
			std::string strChannel = vecParams[0];
			std::string strVictim = vecParams[1];

			CIrcChannel *pChannel = FindChannel(strChannel.c_str());
			if (pChannel != NULL)
			{
				if (strVictim == m_pIrcSocket->GetCurrentNickname())
				{
					dbgprintf("WE WERE KICKED FROM %s.\n", pChannel->GetName());

					for (CPool<CIrcUser *>::iterator i = pChannel->m_plIrcUsers.begin(); i != pChannel->m_plIrcUsers.end(); ++i)
					{
						if ((*i)->m_plIrcChannels.size() == 1)
						{
							dbgprintf("We lost %s.\n", (*i)->GetNickname());
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
						if (pUser != NULL)
						{
							pUser->UpdateIfNecessary(strIdent.c_str(), strHostname.c_str());

							m_pParentCore->GetScriptEventManager()->OnUserKickedUser(this, pUser, pVictim, pChannel, "");
							for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
							{
								(*i)->OnUserKickedUser(this, pUser, pVictim, pChannel, "");
							}
						}
						else
						{
							CIrcUser tempUser(this, strNickname.c_str(), true);
							tempUser.UpdateIfNecessary(strIdent.c_str(), strHostname.c_str());

							m_pParentCore->GetScriptEventManager()->OnUserKickedUser(this, &tempUser, pVictim, pChannel, "");
							for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
							{
								(*i)->OnUserKickedUser(this, &tempUser, pVictim, pChannel, "");
							}
						}

						dbgprintf("Removing %s from %s.\n", pVictim->GetNickname(), pChannel->GetName());
						pVictim->m_plIrcChannels.remove(pChannel);
						pChannel->m_plIrcUsers.remove(pVictim);
						if (pVictim->m_plIrcChannels.size() == 0)
						{
							dbgprintf("We don't know %s anymore.\n", pVictim->GetNickname());
							m_plGlobalUsers.remove(pVictim);
							delete pVictim;
						}
					}
					/* (The victim *must* be on the channel to be kicked, so we'll not handle this)
					else
					{
					}
					*/
				}
			}
			return;
		}
		if (strCommand == "PRIVMSG")
		{
			std::string strTarget = vecParams[0];
			std::string strMessage = vecParams[1];

			if (pUser != NULL)
			{
				pUser->UpdateIfNecessary(strIdent.c_str(), strHostname.c_str());

				if (strTarget == m_pIrcSocket->GetCurrentNickname())
				{
					// privmsg to bot
					dbgprintf("[priv] <%s> %s\n", strNickname.c_str(), strMessage.c_str());

					m_pParentCore->GetScriptEventManager()->OnUserPrivateMessage(this, pUser, strMessage.c_str());
					for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
					{
						(*i)->OnUserPrivateMessage(this, pUser, strMessage.c_str());
					}
				}
				else
				{
					CIrcChannel *pChannel = FindChannel(strTarget.c_str());
					if (pChannel != NULL)
					{
						dbgprintf("[%s] <%s> %s\n", strTarget.c_str(), strNickname.c_str(), strMessage.c_str());

						m_pParentCore->GetScriptEventManager()->OnUserChannelMessage(this, pUser, pChannel, strMessage.c_str());
						for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
						{
							(*i)->OnUserChannelMessage(this, pUser, pChannel, strMessage.c_str());
						}
					}
				}
			}
			else
			{
				CIrcUser tempUser(this, strNickname.c_str(), true);
				tempUser.UpdateIfNecessary(strIdent.c_str(), strHostname.c_str());

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
						dbgprintf("[%s] <%s> %s\n", strTarget.c_str(), strNickname.c_str(), strMessage.c_str());

						m_pParentCore->GetScriptEventManager()->OnUserChannelMessage(this, &tempUser, pChannel, strMessage.c_str());
						for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
						{
							(*i)->OnUserChannelMessage(this, &tempUser, pChannel, strMessage.c_str());
						}
					}
				}
			}
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
			pChannel->m_topicInfo.strTopicSetBy = strNickname;
			pChannel->m_topicInfo.strTopic = vecParams[1];
			return;
		}
	}
	else if (iParamCount == 1)
	{
		if (strCommand == "JOIN")
		{
			std::string strChannel = vecParams[0];

			CIrcChannel *pChannel = NULL;
			bool bSelf = false;
			if (strNickname == m_pIrcSocket->GetCurrentNickname())
			{
				pChannel = new CIrcChannel(this, strChannel.c_str());
				m_plIrcChannels.push_back(pChannel);
				dbgprintf("WE JOINED '%s'\n", pChannel->GetName());

				SendRawFormat("WHO %s", pChannel->GetName());

				bSelf = true;
			}
			else
			{
				pChannel = FindChannel(strChannel.c_str());
				if (pChannel != NULL)
				{
					dbgprintf("'%s' JOINED '%s'\n", strNickname.c_str(), pChannel->GetName());
				}
			}
			if (pChannel != NULL)
			{
				if (pUser == NULL)
				{
					dbgprintf("We don't know %s yet, adding him (2).\n", strNickname.c_str());
					pUser = new CIrcUser(this, strNickname.c_str());
					m_plGlobalUsers.push_back(pUser);
				}
				else
				{
					dbgprintf("We already know %s.\n", strNickname.c_str());
				}

				pUser->UpdateIfNecessary(strIdent.c_str(), strHostname.c_str());

				pUser->m_plIrcChannels.push_back(pChannel);
				pChannel->m_plIrcUsers.push_back(pUser);

				if (bSelf)
				{
					// Add this channel to our list of channels with an incomplete list of users
					m_plIncompleteChannels.push_back(pChannel);
				}
				else
				{
					m_pParentCore->GetScriptEventManager()->OnUserJoinedChannel(this, pUser, pChannel);
					for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
					{
						(*i)->OnUserJoinedChannel(this, pUser, pChannel);
					}
				}
			}
			return;
		}

		if (strCommand == "PART")
		{
			std::string strChannel = vecParams[0];

			CIrcChannel *pChannel = FindChannel(strChannel.c_str());
			if (pChannel != NULL)
			{
				if (strNickname == m_pIrcSocket->GetCurrentNickname())
				{
					dbgprintf("WE LEFT %s.\n", pChannel->GetName());

					for (CPool<CIrcUser *>::iterator i = pChannel->m_plIrcUsers.begin(); i != pChannel->m_plIrcUsers.end(); ++i)
					{
						if ((*i)->m_plIrcChannels.size() == 1)
						{
							dbgprintf("We lost %s.\n", (*i)->GetNickname());
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
					if (pUser != NULL)
					{
						pUser->UpdateIfNecessary(strIdent.c_str(), strHostname.c_str());

						m_pParentCore->GetScriptEventManager()->OnUserLeftChannel(this, pUser, pChannel, "");
						for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
						{
							(*i)->OnUserLeftChannel(this, pUser, pChannel, "");
						}

						dbgprintf("Removing %s from %s.\n", pUser->GetNickname(), pChannel->GetName());
						pUser->m_plIrcChannels.remove(pChannel);
						pChannel->m_plIrcUsers.remove(pUser);
						if (pUser->m_plIrcChannels.size() == 0)
						{
							dbgprintf("We don't know %s anymore.\n", pUser->GetNickname());
							m_plGlobalUsers.remove(pUser);
							delete pUser;
						}
					}
					else
					{
						CIrcUser tempUser(this, strNickname.c_str(), true);
						tempUser.UpdateIfNecessary(strIdent.c_str(), strHostname.c_str());

						m_pParentCore->GetScriptEventManager()->OnUserLeftChannel(this, &tempUser, pChannel, "");
						for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
						{
							(*i)->OnUserLeftChannel(this, &tempUser, pChannel, "");
						}
					}
				}
			}
			return;
		}
		if (strCommand == "QUIT")
		{
			std::string strReason = vecParams[0];

			if (pUser != NULL)
			{
				pUser->UpdateIfNecessary(strIdent.c_str(), strHostname.c_str());

				m_pParentCore->GetScriptEventManager()->OnUserQuit(this, pUser, strReason.c_str());
				for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
				{
					(*i)->OnUserQuit(this, pUser, strReason.c_str());
				}

				for (CPool<CIrcChannel *>::iterator i = m_plIrcChannels.begin(); i != m_plIrcChannels.end(); ++i)
				{
					(*i)->m_plIrcUsers.remove(pUser);
				}

				dbgprintf("We don't know %s anymore.\n", pUser->GetNickname());
				m_plGlobalUsers.remove(pUser);
				delete pUser;
			}
			else
			{
				dbgprintf("Error: we don't know %s yet, for some reason.\n", strNickname.c_str());

				CIrcUser tempUser(this, strNickname.c_str(), true);
				tempUser.UpdateIfNecessary(strIdent.c_str(), strHostname.c_str());

				m_pParentCore->GetScriptEventManager()->OnUserQuit(this, &tempUser, strReason.c_str());
				for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
				{
					(*i)->OnUserQuit(this, &tempUser, strReason.c_str());
				}
			}
			return;
		}
		if (strCommand == "NICK")
		{
			std::string strNewNick = vecParams[0];

			if (strNickname == m_pIrcSocket->GetCurrentNickname())
			{
				m_pIrcSocket->m_strCurrentNickname = strNewNick;
			}
			else
			{
				if (pUser != NULL)
				{
					pUser->UpdateIfNecessary(strIdent.c_str(), strHostname.c_str());

					dbgprintf("Renaming %s to %s.\n", pUser->GetNickname(), strNewNick.c_str());
					pUser->UpdateNickname(strNewNick.c_str());

					m_pParentCore->GetScriptEventManager()->OnUserChangedNickname(this, pUser, strNickname.c_str());
					for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
					{
						(*i)->OnUserChangedNickname(this, pUser, strNickname.c_str());
					}
				}
				else
				{
					CIrcUser tempUser(this, strNewNick.c_str(), true);
					tempUser.UpdateIfNecessary(strIdent.c_str(), strHostname.c_str());

					m_pParentCore->GetScriptEventManager()->OnUserChangedNickname(this, &tempUser, strNickname.c_str());
					for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
					{
						(*i)->OnUserChangedNickname(this, &tempUser, strNickname.c_str());
					}
				}
			}
			return;
		}
	}
	else if (iParamCount == 0)
	{
		if (strCommand == "QUIT")
		{
			if (pUser != NULL)
			{
				pUser->UpdateIfNecessary(strIdent.c_str(), strHostname.c_str());

				m_pParentCore->GetScriptEventManager()->OnUserQuit(this, pUser, "");
				for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
				{
					(*i)->OnUserQuit(this, pUser, "");
				}

				for (CPool<CIrcChannel *>::iterator i = m_plIrcChannels.begin(); i != m_plIrcChannels.end(); ++i)
				{
					(*i)->m_plIrcUsers.remove(pUser);
				}

				dbgprintf("We don't know %s anymore.\n", pUser->GetNickname());
				m_plGlobalUsers.remove(pUser);
				delete pUser;
			}
			else
			{
				dbgprintf("Error: we don't know %s yet, for some reason.\n", strNickname.c_str());

				CIrcUser tempUser(this, strNickname.c_str(), true);
				tempUser.UpdateIfNecessary(strIdent.c_str(), strHostname.c_str());

				m_pParentCore->GetScriptEventManager()->OnUserQuit(this, &tempUser, "");
				for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
				{
					(*i)->OnUserQuit(this, &tempUser, "");
				}
			}
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
				SendRaw("PROTOCTL NAMESX");
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
		std::string strChannel = vecParams[0];
		std::string strModes = vecParams[1];

		CIrcChannel *pChannel = FindChannel(strChannel.c_str());

		if (pChannel != NULL)
		{
			int iSet = 0;
			int iParamOffset = 2;
			std::string strParams;
			for (std::string::iterator i = strModes.begin(); i != strModes.end(); ++i)
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

							//dbgprintf("1MODEEEEEEE %c%c %s!\n", iSet == 1 ? '+' : '-', cMode, strParam.c_str());

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

							//dbgprintf("2MODEEEEEEE +%c %s!\n", cMode, strParam.c_str());
						}
						else if (iSet == 2)
						{
							//dbgprintf("2MODEEEEEEE -%c!\n", cMode);
						}
						break;
					}

					case 4: // No parameter
					{
						//dbgprintf("3MODEEEEEEE %c%c!\n", iSet == 1 ? '+' : '-', cMode);
						break;
					}
				}
			}
			
			if (pUser != NULL)
			{
				pUser->UpdateIfNecessary(strIdent.c_str(), strHostname.c_str());

				m_pParentCore->GetScriptEventManager()->OnUserSetChannelModes(this, pUser, pChannel, strModes.c_str(), strParams.c_str());
				for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
				{
					(*i)->OnUserSetChannelModes(this, pUser, pChannel, strModes.c_str(), strParams.c_str());
				}
			}
			else
			{
				CIrcUser tempUser(this, strNickname.c_str(), true);
				tempUser.UpdateIfNecessary(strIdent.c_str(), strHostname.c_str());

				m_pParentCore->GetScriptEventManager()->OnUserSetChannelModes(this, &tempUser, pChannel, strModes.c_str(), strParams.c_str());
				for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
				{
					(*i)->OnUserSetChannelModes(this, &tempUser, pChannel, strModes.c_str(), strParams.c_str());
				}
			}
		}
		return;
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
