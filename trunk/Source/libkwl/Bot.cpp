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

void CBot::HandleData(const std::vector<std::string> &vecParts)
{
	bool bPrefix = *(vecParts[0].begin()) == ':';
	
	std::string strPrefix;
	if (bPrefix)
	{
		strPrefix = vecParts[0].substr(1);
	}

	std::string strCommand = vecParts[bPrefix ? 1 : 0];

	if (!m_bGotMotd)
	{
		if (strCommand == "376" || strCommand == "422")
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

	if (strCommand == "353")
	{
		std::string strChannel = vecParts[3];
		std::string::size_type iTempSpace = strChannel.find(' ');
		if (iTempSpace != std::string::npos)
		{
			strChannel = strChannel.substr(iTempSpace + 1);
			iTempSpace = strChannel.find(' ');
			if (iTempSpace != std::string::npos)
			{
				strChannel = strChannel.substr(0, iTempSpace);

				CIrcChannel *pChannel = FindChannel(strChannel.c_str());
				if (pChannel != NULL)
				{
					std::string strNames = vecParts[3].substr(3 + strChannel.length());
					if (*(strNames.begin()) == ':')
					{
						strNames = strNames.substr(1);
					}

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

						if (strName != m_pIrcSocket->GetCurrentNickname())
						{
							CIrcUser *pUser = FindUser(strName.c_str());
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
			}
		}
		return;
	}

	if (strCommand == "366")
	{
		std::string strChannel = vecParts[3];
		std::string::size_type iTempSpace = strChannel.find(' ');
		if (iTempSpace != std::string::npos)
		{
			strChannel = strChannel.substr(iTempSpace + 1);
			iTempSpace = strChannel.find(' ');
			if (iTempSpace != std::string::npos)
			{
				strChannel = strChannel.substr(0, iTempSpace);

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
			}
		}
		return;
	}
	else if (strCommand != "332" /* Topic */ && strCommand != "333" /* Topic Timestamp */ && m_plIncompleteChannels.size() != 0) // If we did not get a NAMES reply right after the JOIN message
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

	if (strCommand == "JOIN")
	{
		std::string strChannel = vecParts[2];
		if (*(strChannel.begin()) == ':')
		{
			strChannel = strChannel.substr(1);
		}

		std::string strNickname = vecParts[0].substr(1);
		std::string::size_type iSeparator = strNickname.find('!');
		if (iSeparator != std::string::npos)
		{
			strNickname = strNickname.substr(0, iSeparator);
			std::string strIdent = vecParts[0].substr(iSeparator + 2);
			if ((iSeparator = strIdent.find('@')) != std::string::npos)
			{
				std::string strHost = strIdent.substr(iSeparator + 1);
				strIdent = strIdent.substr(0, iSeparator);

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
					CIrcUser *pUser = FindUser(strNickname.c_str());
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
					pUser->SetIdent(strIdent.c_str());
					pUser->SetHost(strHost.c_str());
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
			}
		}
		return;
	}

	if (strCommand == "PART")
	{
		std::string strChannel = vecParts[2];
		if (*(strChannel.begin()) == ':')
		{
			strChannel = strChannel.substr(1);
		}

		std::string strNickname = vecParts[0].substr(1);
		std::string::size_type iSeparator = strNickname.find('!');
		if (iSeparator != std::string::npos)
		{
			strNickname = strNickname.substr(0, iSeparator);
			std::string strIdent = vecParts[0].substr(iSeparator + 2);
			if ((iSeparator = strIdent.find('@')) != std::string::npos)
			{
				std::string strHost = strIdent.substr(iSeparator + 1);
				strIdent = strIdent.substr(0, iSeparator);

				std::string strReason = vecParts[3];
				if (!strReason.empty() && *(strReason.begin()) == ':')
				{
					strReason = strReason.substr(1);
				}

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
						CIrcUser *pUser = FindUser(strNickname.c_str());
						if (pUser != NULL)
						{
							pUser->SetIdent(strIdent.c_str());
							pUser->SetHost(strHost.c_str());

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
							tempUser.SetIdent(strIdent.c_str());
							tempUser.SetHost(strHost.c_str());

							m_pParentCore->GetScriptEventManager()->OnUserLeftChannel(this, &tempUser, pChannel, strReason.c_str());
							for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
							{
								(*i)->OnUserLeftChannel(this, &tempUser, pChannel, strReason.c_str());
							}
						}
					}
				}
			}
		}
		return;
	}

	if (strCommand == "KICK")
	{
		std::string strChannel = vecParts[2];
		if (*(strChannel.begin()) == ':')
		{
			strChannel = strChannel.substr(1);
		}

		std::string strNickname = vecParts[0].substr(1);
		std::string::size_type iSeparator = strNickname.find('!');
		if (iSeparator != std::string::npos)
		{
			strNickname = strNickname.substr(0, iSeparator);
			std::string strIdent = vecParts[0].substr(iSeparator + 2);
			if ((iSeparator = strIdent.find('@')) != std::string::npos)
			{
				std::string strHost = strIdent.substr(iSeparator + 1);
				strIdent = strIdent.substr(0, iSeparator);

				std::string::size_type iTempSpace = vecParts[3].find(' ');

				std::string strVictim = vecParts[3].substr(0, iTempSpace);
				std::string strReason = iTempSpace != std::string::npos ? vecParts[3].substr(iTempSpace + 1) : vecParts[3];

				CIrcChannel *pChannel = FindChannel(strChannel.c_str());
				if (pChannel != NULL)
				{
					CIrcUser *pUser = FindUser(strNickname.c_str());
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
								pUser->SetIdent(strIdent.c_str());
								pUser->SetHost(strHost.c_str());

								m_pParentCore->GetScriptEventManager()->OnUserKickedUser(this, pUser, pVictim, pChannel, strReason.c_str());
								for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
								{
									(*i)->OnUserKickedUser(this, pUser, pVictim, pChannel, strReason.c_str());
								}
							}
							else
							{
								CIrcUser tempUser(this, strNickname.c_str(), true);
								tempUser.SetIdent(strIdent.c_str());
								tempUser.SetIdent(strHost.c_str());

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
			}
		}
		return;
	}

	if (strCommand == "QUIT")
	{
		std::string strNickname = vecParts[0].substr(1);
		std::string::size_type iSeparator = strNickname.find('!');
		if (iSeparator != std::string::npos)
		{
			strNickname = strNickname.substr(0, iSeparator);
			std::string strIdent = vecParts[0].substr(iSeparator + 2);
			if ((iSeparator = strIdent.find('@')) != std::string::npos)
			{
				std::string strHost = strIdent.substr(iSeparator + 1);
				strIdent = strIdent.substr(0, iSeparator);

				std::string strReason = vecParts[2].substr(1);
				if (!vecParts[3].empty())
				{
					strReason += " " + vecParts[3];
				}

				CIrcUser *pUser = FindUser(strNickname.c_str());
				if (pUser != NULL)
				{
					pUser->SetIdent(strIdent.c_str());
					pUser->SetHost(strHost.c_str());

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
					tempUser.SetIdent(strIdent.c_str());
					tempUser.SetHost(strHost.c_str());

					m_pParentCore->GetScriptEventManager()->OnUserQuit(this, &tempUser, strReason.c_str());
					for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
					{
						(*i)->OnUserQuit(this, &tempUser, strReason.c_str());
					}
				}
			}
		}
		return;
	}

	if (strCommand == "NICK")
	{
		std::string strNickname = vecParts[0].substr(1);
		std::string::size_type iSeparator = strNickname.find('!');
		if (iSeparator != std::string::npos)
		{
			strNickname = strNickname.substr(0, iSeparator);
			std::string strIdent = vecParts[0].substr(iSeparator + 2);
			if ((iSeparator = strIdent.find('@')) != std::string::npos)
			{
				std::string strHost = strIdent.substr(iSeparator + 1);
				strIdent = strIdent.substr(0, iSeparator);

				std::string strNewNick = vecParts[2];
				if (*(strNewNick.begin()) == ':')
				{
					strNewNick = strNewNick.substr(1);
				}

				if (strNickname == m_pIrcSocket->GetCurrentNickname())
				{
					m_pIrcSocket->m_strCurrentNickname = strNewNick;
				}
				else
				{
					CIrcUser *pUser = FindUser(strNickname.c_str());
					if (pUser != NULL)
					{
						pUser->SetIdent(strIdent.c_str());
						pUser->SetHost(strHost.c_str());

						dbgprintf("Renaming %s to %s.\n", pUser->GetNickname(), strNewNick.c_str());
						pUser->SetNickname(strNewNick.c_str());

						m_pParentCore->GetScriptEventManager()->OnUserChangedNickname(this, pUser, strNickname.c_str());
						for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
						{
							(*i)->OnUserChangedNickname(this, pUser, strNickname.c_str());
						}
					}
					else
					{
						CIrcUser tempUser(this, strNewNick.c_str(), true);
						tempUser.SetIdent(strIdent.c_str());
						tempUser.SetHost(strHost.c_str());

						m_pParentCore->GetScriptEventManager()->OnUserChangedNickname(this, &tempUser, strNickname.c_str());
						for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
						{
							(*i)->OnUserChangedNickname(this, &tempUser, strNickname.c_str());
						}
					}
				}
			}
		}
		return;
	}

	if (strCommand == "PRIVMSG")
	{
		std::string strNickname = vecParts[0].substr(1);
		std::string::size_type iSeparator = strNickname.find('!');
		if (iSeparator != std::string::npos)
		{
			strNickname = strNickname.substr(0, iSeparator);
			std::string strIdent = vecParts[0].substr(iSeparator + 2);
			if ((iSeparator = strIdent.find('@')) != std::string::npos)
			{
				std::string strHost = strIdent.substr(iSeparator + 1);
				strIdent = strIdent.substr(0, iSeparator);

				std::string strTarget = vecParts[2];
				std::string strMessage = vecParts[3];
				if (*(strMessage.begin()) == ':')
				{
					strMessage = strMessage.substr(1);
				}

				CIrcUser *pUser = FindUser(strNickname.c_str());
				if (pUser != NULL)
				{
					pUser->SetIdent(strIdent.c_str());
					pUser->SetHost(strHost.c_str());

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
					tempUser.SetIdent(strIdent.c_str());
					tempUser.SetHost(strHost.c_str());

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
			}
		}
		return;
	}

	if (strCommand == "005")
	{
		std::string strSupports = vecParts[3];
		
		std::string::size_type iLastPos = strSupports.find_first_not_of(' ', 0);
		std::string::size_type iPos = strSupports.find_first_of(' ', iLastPos);
		while (iPos != std::string::npos || iLastPos != std::string::npos)
		{
			std::string strSupport = strSupports.substr(iLastPos, iPos - iLastPos);
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
					std::string::size_type iLastPos_ = strValue.find_first_not_of(',', 0);
					std::string::size_type iPos_ = strValue.find_first_of(',', iLastPos_);
					int i = 0;
					while (i < 4 && (iPos_ != std::string::npos || iLastPos_ != std::string::npos))
					{
						m_SupportSettings.strChanmodes[i] = strValue.substr(iLastPos_, iPos_ - iLastPos_);

						iLastPos_ = strValue.find_first_not_of(',', iPos_);
						iPos_ = strValue.find_first_of(',', iLastPos_);
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

			iLastPos = strSupports.find_first_not_of(' ', iPos);
			iPos = strSupports.find_first_of(' ', iLastPos);
		}
		return;
	}

	if (strCommand == "MODE")
	{
		std::string strNickname = vecParts[0].substr(1);
		std::string::size_type iSeparator = strNickname.find('!');
		std::string strIdent;
		std::string strHost;
		if (iSeparator != std::string::npos)
		{
			strNickname = strNickname.substr(0, iSeparator);
			std::string strIdent = vecParts[0].substr(iSeparator + 2);
			if ((iSeparator = strIdent.find('@')) != std::string::npos)
			{
				strHost = strIdent.substr(iSeparator + 1);
				strIdent = strIdent.substr(0, iSeparator);
			}
		}

		std::string strChannel = vecParts[2];
		std::string strModes = vecParts[3];
		iSeparator = strModes.find(' ');
		std::string strParams;
		if (iSeparator != std::string::npos)
		{
			strModes = strModes.substr(0, iSeparator);
			strParams = vecParts[3].substr(iSeparator + 1);
		}

		CIrcChannel *pChannel = FindChannel(strChannel.c_str());

		if (pChannel != NULL)
		{
			CIrcUser *pUser = FindUser(strNickname.c_str());
			if (pUser != NULL)
			{
				pUser->SetIdent(strIdent.c_str());
				pUser->SetHost(strHost.c_str());

				m_pParentCore->GetScriptEventManager()->OnUserSetChannelModes(this, pUser, pChannel, strModes.c_str(), strParams.c_str());
				for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
				{
					(*i)->OnUserSetChannelModes(this, pUser, pChannel, strModes.c_str(), strParams.c_str());
				}
			}
			else
			{
				CIrcUser tempUser(this, strNickname.c_str(), true);
				tempUser.SetIdent(strIdent.c_str());
				tempUser.SetHost(strHost.c_str());

				m_pParentCore->GetScriptEventManager()->OnUserSetChannelModes(this, &tempUser, pChannel, strModes.c_str(), strParams.c_str());
				for (CPool<CEventManager *>::iterator i = m_pParentCore->GetEventManagers()->begin(); i != m_pParentCore->GetEventManagers()->end(); ++i)
				{
					(*i)->OnUserSetChannelModes(this, &tempUser, pChannel, strModes.c_str(), strParams.c_str());
				}
			}

			std::string::size_type iLastPos = strParams.find_first_not_of(' ', 0);
			std::string::size_type iPos = strParams.find_first_of(' ', iLastPos);
			int iSet = 0;
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
						if (iPos != std::string::npos || iLastPos != std::string::npos)
						{
							std::string strParam = strParams.substr(iLastPos, iPos - iLastPos);
							//dbgprintf("1MODEEEEEEE %c%c %s!\n", iSet == 1 ? '+' : '-', cMode, strParam.c_str());

							if (bPrefixMode)
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
										pUser->m_mapChannelModes[pChannel] |= cNewMode;
									else
										pUser->m_mapChannelModes[pChannel] &= ~cNewMode;
								}
							}

							iLastPos = strParams.find_first_not_of(' ', iPos);
							iPos = strParams.find_first_of(' ', iLastPos);
						}
						break;
					}

					case 3: // Parameter if iSet == 1
					{
						if (iSet == 1 && (iPos != std::string::npos || iLastPos != std::string::npos))
						{
							std::string strParam = strParams.substr(iLastPos, iPos - iLastPos);
							//dbgprintf("2MODEEEEEEE +%c %s!\n", cMode, strParam.c_str());

							iLastPos = strParams.find_first_not_of(' ', iPos);
							iPos = strParams.find_first_of(' ', iLastPos);
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
		}
		return;
	}

	// WHO reply
	//:ircd 352 #channel ident host ircd nickname IDK :hopcount realname
	if (strCommand == "352")
	{
		std::string strChannel = vecParts[2];
		std::string strOther = vecParts[3];
		std::string::size_type iSeparator = strOther.find(' ');
		if (iSeparator != std::string::npos)
		{
			std::string strIdent = strOther.substr(iSeparator + 1);
			if ((iSeparator = strIdent.find(' ')) != std::string::npos)
			{
				std::string strHost = strIdent.substr(iSeparator + 1);
				strIdent = strIdent.substr(0, iSeparator);
				if ((iSeparator = strHost.find(' ')) != std::string::npos)
				{
					std::string strServer = strHost.substr(iSeparator + 1);
					strHost = strHost.substr(0, iSeparator);
					if ((iSeparator = strServer.find(' ')) != std::string::npos)
					{
						std::string strNickname = strServer.substr(iSeparator + 1);
						strServer = strServer.substr(0, iSeparator);
						if ((iSeparator = strNickname.find(' ')) != std::string::npos)
						{
							std::string strUnknown = strNickname.substr(iSeparator + 1);
							strNickname = strNickname.substr(0, iSeparator);
							if ((iSeparator = strUnknown.find(' ')) != std::string::npos)
							{
								std::string strHopCount = strUnknown.substr(iSeparator + 1);
								strUnknown = strUnknown.substr(0, iSeparator);
								if ((iSeparator = strHopCount.find(' ')) != std::string::npos)
								{
									std::string strRealname = strHopCount.substr(iSeparator + 1);
									strHopCount = strHopCount.substr(0, iSeparator);

									CIrcUser *pUser = FindUser(strNickname.c_str());
									if (pUser != NULL)
									{
										pUser->SetIdent(strIdent.c_str());
										pUser->SetHost(strHost.c_str());
									}
								}
							}
						}
					}
				}
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

	if (!accessRules.strRequireHost.empty() && !wildcmp(accessRules.strRequireHost.c_str(), pUser->GetHost()))
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
