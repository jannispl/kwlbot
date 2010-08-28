/*
kwlbot IRC bot


File:		Bot.cpp
Purpose:	Class which represents an IRC bot

*/

#include "StdInc.h"
#include "Bot.h"
#include "WildcardMatch.h"
#include <algorithm>

// Shortcut to call an event
#define CALL_EVENT_NOARG(what) \
	for (CPool<CEventManager *>::iterator eventManagerIterator = m_pParentCore->GetEventManagers()->begin(); eventManagerIterator != m_pParentCore->GetEventManagers()->end(); ++eventManagerIterator) \
	{ \
		(*eventManagerIterator)->what(this); \
	} \
	m_pParentCore->GetScriptEventManager()->what(this)

#define CALL_EVENT(what, ...) \
	for (CPool<CEventManager *>::iterator eventManagerIterator = m_pParentCore->GetEventManagers()->begin(); eventManagerIterator != m_pParentCore->GetEventManagers()->end(); ++eventManagerIterator) \
	{ \
		(*eventManagerIterator)->what(this, __VA_ARGS__); \
	} \
	m_pParentCore->GetScriptEventManager()->what(this, __VA_ARGS__)

CBot::CBot(CCore *pParentCore, CConfig *pConfig)
	: m_bDead(false), m_tReconnectTimer((time_t)-1), m_bGotMotd(false), m_pCurrentUser(NULL)
{
	m_pParentCore = pParentCore;
	m_ircSettings.LoadFromConfig(pConfig);

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
		sprintf(szValue, "access[%d]", i);
		if (!pConfig->StartValueList(szValue))
		{
			break;
		}

		AccessRules rules;

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
	m_supportSettings.bNamesX = false;

	CALL_EVENT_NOARG(OnBotCreated);

	if (pConfig->GetSingleValue("server", &strTemp))
	{
		std::string strPassword;
		pConfig->GetSingleValue("password", &strPassword); // strPassword will remain empty if there is no such config entry

		std::string::size_type iPortSep = strTemp.find(':');
		if (iPortSep == std::string::npos)
		{
			m_pIrcSocket->Connect(strTemp.c_str(), 6667, strPassword.c_str());
		}
		else
		{
			m_pIrcSocket->Connect(strTemp.substr(0, iPortSep).c_str(), atoi(strTemp.substr(iPortSep + 1).c_str()), strPassword.c_str());
		}
	}

	m_botConfig = *pConfig;
}

CBot::~CBot()
{
	CALL_EVENT_NOARG(OnBotDestroyed);

	for (CPool<CScript *>::iterator i = m_plScripts.begin(); i != m_plScripts.end(); ++i)
	{
		delete *i;
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
	return &m_ircSettings;
}

CConfig *CBot::GetConfig()
{
	return &m_botConfig;
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

	for (CPool<CScript *>::iterator i = m_plScripts.begin(); i != m_plScripts.end(); ++i)
	{
		(*i)->Pulse();
	}
}

CIrcChannel *CBot::FindChannel(const char *szName, bool bCaseSensitive)
{
	typedef int (* Compare_t)(const char *, const char *);
	Compare_t pfnCompare = bCaseSensitive ? strcmp : stricmp;

	for (CPool<CIrcChannel *>::iterator i = m_plIrcChannels.begin(); i != m_plIrcChannels.end(); ++i)
	{
		if (pfnCompare(szName, (*i)->GetName()) == 0)
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
		if (pfnCompare(szName, (*i)->GetNickname().c_str()) == 0)
		{
			return *i;
		}
	}
	return NULL;
}

char CBot::ModeToPrefix(char cMode)
{
	std::string::size_type iIndex = m_supportSettings.strPrefixes[0].find(cMode);
	if (iIndex == std::string::npos)
	{
		return 0;
	}
	return m_supportSettings.strPrefixes[1][iIndex];
}

char CBot::PrefixToMode(char cPrefix)
{
	std::string::size_type iIndex = m_supportSettings.strPrefixes[1].find(cPrefix);
	if (iIndex == std::string::npos)
	{
		return 0;
	}
	return m_supportSettings.strPrefixes[0][iIndex];
}

bool CBot::IsPrefixMode(char cMode)
{
	return ModeToPrefix(cMode) != 0;
}

char CBot::GetModeGroup(char cMode)
{
	for (int i = 0; i < 4; ++i)
	{
		std::string::size_type iPos = m_supportSettings.strChanmodes[i].find(cMode);
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
				m_pCurrentUser->UpdateIfNecessary(m_strCurrentIdent, m_strCurrentHostname.c_str());
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

			CALL_EVENT_NOARG(OnBotConnected);

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
			CALL_EVENT(OnBotJoinedChannel, *i);

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
			Handle332(vecParams[1], vecParams[2]);
			return;
		}
	}
	else if (iParamCount == 2)
	{
		if (iNumeric == 366)
		{
			Handle366(vecParams[0]);
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
			HandleTOPIC(vecParams[0], vecParams[1]);
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

	if (iNumeric == 005)
	{
		for (int i = 1; i < iParamCount; ++i)
		{
			std::string strSupport = vecParams[i];
			if (strSupport == "NAMESX")
			{
				// Tell the server we support NAMESX
				SendRawStatic("PROTOCTL NAMESX");
				m_supportSettings.bNamesX = true;
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
						m_supportSettings.strChanmodes[i] = strValue.substr(iLastPos, iPos - iLastPos);

						iLastPos = strValue.find_first_not_of(',', iPos);
						iPos = strValue.find_first_of(',', iLastPos);
						++i;
					}
				}
				else if (strSupport.length() > 7 && strSupport.substr(0, 7) == "PREFIX=")
				{
					strValue = strValue.substr(1);
					std::string::size_type iEnd = strValue.find(')');
					m_supportSettings.strPrefixes[0] = strValue.substr(0, iEnd);
					m_supportSettings.strPrefixes[1] = strValue.substr(iEnd + 1);

					std::reverse(m_supportSettings.strPrefixes[0].begin(), m_supportSettings.strPrefixes[0].end());
					std::reverse(m_supportSettings.strPrefixes[1].begin(), m_supportSettings.strPrefixes[1].end());
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
		pUser->UpdateIfNecessary(m_strCurrentIdent, m_strCurrentHostname);
	}

	CIrcChannel *pChannel = FindChannel(strChannel.c_str());
	if (pChannel != NULL && !pChannel->m_bHasDetailedUsers)
	{
		pChannel->m_bHasDetailedUsers = true;

		CALL_EVENT(OnBotSyncedChannel, pChannel);
	}
}

void CBot::Handle353(const std::string &strChannel, const std::string &strNames)
{
	CIrcChannel *pChannel = FindChannel(strChannel.c_str());
	if (pChannel == NULL)
	{
		return;
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
			char cModeFlag = 1;
			for (size_t j = 0; j < m_supportSettings.strPrefixes[1].length(); ++j)
			{
				cModeFlag *= 2;
				if (*i == m_supportSettings.strPrefixes[1][j])
				{
					++iOffset;
					cMode |= cModeFlag;
					break;
				}
				else if (j == m_supportSettings.strPrefixes[1].length() - 1)
				{
					bBreak = true;
				}
			}

			// If the server does not support NAMESX, consider any other mode chars part of the nickname.
			if (!m_supportSettings.bNamesX || bBreak)
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
	if (pChannel == NULL)
	{
		return;
	}

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
				CALL_EVENT(OnUserKickedUser, m_pCurrentUser, pVictim, pChannel, strReason.c_str());
			}
			else
			{
				CIrcUser tempUser(this, m_strCurrentNickname, true);
				tempUser.UpdateIfNecessary(m_strCurrentIdent, m_strCurrentHostname);

				CALL_EVENT(OnUserKickedUser, &tempUser, pVictim, pChannel, strReason.c_str());
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

void CBot::Handle332(const std::string &strChannel, const std::string &strTopic)
{
	CIrcChannel *pChannel = FindChannel(strChannel.c_str());
	if (pChannel == NULL)
	{
		return;
	}

	pChannel->m_topicInfo.strTopicSetBy = m_strCurrentNickname;
	pChannel->m_topicInfo.strTopic = strTopic;
}

void CBot::Handle366(const std::string &strChannel)
{
	CIrcChannel *pChannel = FindChannel(strChannel.c_str());
	if (pChannel == NULL)
	{
		return;
	}

	for (CPool<CIrcChannel *>::iterator i = m_plIncompleteChannels.begin(); i != m_plIncompleteChannels.end(); ++i)
	{
		if (*i == pChannel)
		{
			CALL_EVENT(OnBotJoinedChannel, pChannel);

			i = m_plIncompleteChannels.erase(i);
			break;
		}
	}
}

void CBot::HandlePART(const std::string &strChannel, const std::string &strReason)
{
	CIrcChannel *pChannel = FindChannel(strChannel.c_str());
	if (pChannel == NULL)
	{
		return;
	}

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
			CALL_EVENT(OnUserLeftChannel, m_pCurrentUser, pChannel, strReason.c_str());

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
			CIrcUser tempUser(this, m_strCurrentNickname, true);
			tempUser.UpdateIfNecessary(m_strCurrentIdent, m_strCurrentHostname);

			CALL_EVENT(OnUserLeftChannel, &tempUser, pChannel, strReason.c_str());
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
			CALL_EVENT(OnUserPrivateMessage, m_pCurrentUser, strMessage.c_str());
		}
		else
		{
			CIrcChannel *pChannel = FindChannel(strTarget.c_str());
			if (pChannel != NULL)
			{
				CALL_EVENT(OnUserChannelMessage, m_pCurrentUser, pChannel, strMessage.c_str());
			}
		}
	}
	else
	{
		CIrcUser tempUser(this, m_strCurrentNickname, true);
		tempUser.UpdateIfNecessary(m_strCurrentIdent, m_strCurrentHostname);

		if (strTarget == m_pIrcSocket->GetCurrentNickname())
		{
			CALL_EVENT(OnUserPrivateMessage, &tempUser, strMessage.c_str());
		}
		else
		{
			CIrcChannel *pChannel = FindChannel(strTarget.c_str());
			if (pChannel != NULL)
			{
				CALL_EVENT(OnUserChannelMessage, &tempUser, pChannel, strMessage.c_str());
			}
		}
	}
}

void CBot::HandleTOPIC(const std::string &strChannel, const std::string &strTopic)
{
	CIrcChannel *pChannel = FindChannel(strChannel.c_str());
	if (pChannel == NULL)
	{
		return;
	}

	std::string strOldTopic = pChannel->m_topicInfo.strTopic;

	pChannel->m_topicInfo.ullTopicSetDate = time(NULL);
	pChannel->m_topicInfo.strTopicSetBy = m_strCurrentNickname;
	pChannel->m_topicInfo.strTopic = strTopic;

	CIrcUser *pUser = pChannel->FindUser(m_strCurrentNickname.c_str());
	if (pUser != NULL)
	{
		CALL_EVENT(OnUserSetChannelTopic, pUser, pChannel, strOldTopic.c_str());
	}
	else
	{
		CIrcUser tempUser(this, m_strCurrentNickname, true);
		tempUser.UpdateIfNecessary(m_strCurrentIdent, m_strCurrentHostname);

		CALL_EVENT(OnUserSetChannelTopic, &tempUser, pChannel, strOldTopic.c_str());
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

	if (pChannel == NULL)
	{
		return;
	}

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
		CALL_EVENT(OnUserJoinedChannel, m_pCurrentUser, pChannel);
	}
}

void CBot::HandleQUIT(const std::string &strReason)
{
	if (m_pCurrentUser != NULL)
	{
		CALL_EVENT(OnUserQuit, m_pCurrentUser, strReason.c_str());

		for (CPool<CIrcChannel *>::iterator i = m_plIrcChannels.begin(); i != m_plIrcChannels.end(); ++i)
		{
			(*i)->m_plIrcUsers.remove(m_pCurrentUser);
		}

		m_plGlobalUsers.remove(m_pCurrentUser);
		delete m_pCurrentUser;
	}
	else
	{
		CIrcUser tempUser(this, m_strCurrentNickname, true);
		tempUser.UpdateIfNecessary(m_strCurrentIdent, m_strCurrentHostname);

		CALL_EVENT(OnUserQuit, &tempUser, strReason.c_str());
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
		m_pCurrentUser->UpdateNickname(strNewNickname);

		CALL_EVENT(OnUserChangedNickname, m_pCurrentUser, m_strCurrentNickname.c_str());
	}
	else
	{
		CIrcUser tempUser(this, strNewNickname, true);
		tempUser.UpdateIfNecessary(m_strCurrentIdent, m_strCurrentHostname);

		CALL_EVENT(OnUserChangedNickname, &tempUser, m_strCurrentNickname.c_str());
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
							char cNewMode = 1;
							for (size_t j = 0; j < m_supportSettings.strPrefixes[0].length(); ++j)
							{
								cNewMode *= 2;
								if (*i == m_supportSettings.strPrefixes[0][j])
								{
									break;
								}
							}

							if (cNewMode != 1)
							{
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
		CALL_EVENT(OnUserSetChannelModes, m_pCurrentUser, pChannel, strModes.c_str(), strParams.c_str());
	}
	else
	{
		CIrcUser tempUser(this, m_strCurrentNickname, true);
		tempUser.UpdateIfNecessary(m_strCurrentIdent, m_strCurrentHostname);

		CALL_EVENT(OnUserSetChannelModes, &tempUser, pChannel, strModes.c_str(), strParams.c_str());
	}
}

bool CBot::TestAccessLevel(CIrcUser *pUser, int iLevel)
{
	if (iLevel < 1 || iLevel > GetNumAccessLevels())
	{
		return false;
	}

	AccessRules accessRules = m_vecAccessRules[iLevel - 1];

	if (!accessRules.strRequireHost.empty() && !wildcmp(accessRules.strRequireHost.c_str(), pUser->GetHostname().c_str()))
	{
		return false;
	}

	if (!accessRules.strRequireIdent.empty() && !wildcmp(accessRules.strRequireIdent.c_str(), pUser->GetIdent().c_str()))
	{
		return false;
	}

	if (!accessRules.strRequireNickname.empty() && !wildcmp(accessRules.strRequireNickname.c_str(), pUser->GetNickname().c_str()))
	{
		return false;
	}

	return true;
}

int CBot::GetNumAccessLevels()
{
	return (int)m_vecAccessRules.size();
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

void CBot::SendMessage(const CIrcMessage &ircMessage)
{
	ircMessage.Send(this);
}

const char *CBot::GetModePrefixes()
{
	return m_supportSettings.strPrefixes[1].c_str();
}

void CBot::Die(CBot::eDeathReason deathReason)
{
	m_bDead = true;
	m_deathReason = deathReason;
}

bool CBot::IsDead()
{
	return m_bDead;
}

CBot::eDeathReason CBot::GetDeathReason()
{
	return m_deathReason;
}

void CBot::StartReconnectTimer(unsigned int uiSeconds)
{
	m_tReconnectTimer = time(NULL) + uiSeconds;
}

time_t CBot::GetReconnectTimer()
{
	return m_tReconnectTimer;
}

CScriptObject::eScriptType CBot::GetType()
{
	return Bot;
}
