/*
kwlbot IRC bot


File:		Bot.cpp
Purpose:	Class which represents an IRC bot

*/

#include "StdInc.h"
#include "Bot.h"
#include "WildcardMatch.h"
#include "IrcMessages.h"
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
	: m_bDead(false), m_tReconnectTimer((time_t)-1), m_bSynced(false), m_pCurrentUser(NULL), m_bGotScripts(false)
{
	m_pParentCore = pParentCore;
	m_ircSettings.LoadFromConfig(pConfig);

	std::string strServer;
	std::string strPassword;
	int iPort = 6667;
	if (pConfig->GetSingleValue("server", &strServer))
	{
		pConfig->GetSingleValue("password", &strPassword); // strPassword will remain empty if there is no such config entry

		std::string::size_type iPortSep = strServer.find(':');
		if (iPortSep != std::string::npos)
		{
			iPort = atoi(strServer.substr(iPortSep + 1).c_str());
			strServer = strServer.substr(0, iPortSep);
		}
	}

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
	m_bGotScripts = true;
	
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
				else if (strKey == "require-realname")
				{
					rules.strRequireRealname = strValue;
				}
#ifndef SERVICE
				else if (strKey == "require-vhost")
				{
					printf("[%s] Warning: Not compiled as service bot - using 'require-host' instead of 'require-vhost'.\n", m_pIrcSocket->GetCurrentNickname());
					rules.strRequireHost = strValue;
				}
				else if (strKey == "require-umodes")
				{
					printf("[%s] Warning: Not compiled as service bot - ignoring 'require-umodes'.\n", m_pIrcSocket->GetCurrentNickname());
				}
#else
				else if (strKey == "require-vhost")
				{
					rules.strRequireVhost = strValue;
				}
				else if (strKey == "require-umodes")
				{
					rules.strRequireUmodes = strValue;
				}
#endif
			}
		}

		m_vecAccessRules.push_back(rules);

		++i;
	}

	m_bSynced = false;
#ifndef SERVICE
	m_supportSettings.bNamesX = false;
#else
	m_supportSettings.bNickV2 = false;

#if IRCD == UNREAL || IRCD == INSPIRCD
	m_supportSettings.strPrefixes[0] = "vhoaq";
	m_supportSettings.strPrefixes[1] = "+%@&~";
#elif IRCD == HYBRID
	m_supportSettings.strPrefixes[0] = "vho";
	m_supportSettings.strPrefixes[1] = "+%@";
#endif

#endif

	CALL_EVENT_NOARG(OnBotCreated);

	if (!strServer.empty())
	{
		m_pIrcSocket->Connect(strServer.c_str(), iPort, strPassword.c_str());
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

int CBot::SendRaw(const std::string &strData)
{
	return m_pIrcSocket->SendRaw(strData.c_str());
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
		if (pfnCompare(szName, (*i)->GetName().c_str()) == 0)
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

bool CBot::TestAccessLevel(CIrcUser *pUser, int iLevel)
{
	if (iLevel < 1 || iLevel > GetNumAccessLevels())
	{
		return false;
	}

	AccessRules accessRules = m_vecAccessRules[iLevel - 1];

#ifdef SERVICE
	if (!accessRules.strRequireUmodes.empty())
	{
		std::string strMustModes, strMustNotModes;
		int iMode = 1;

		for (std::string::iterator i = accessRules.strRequireUmodes.begin(); i != accessRules.strRequireUmodes.end(); ++i)
		{
			if (*i == '+')
			{
				iMode = 1;
			}
			else if (*i == '-')
			{
				iMode = 2;
			}
			else
			{
				if (iMode == 1)
				{
					if (pUser->GetUserModes().find(*i) == std::string::npos)
					{
						return false;
					}
				}
				else
				{
					if (pUser->GetUserModes().find(*i) != std::string::npos)
					{
						return false;
					}
				}
			}
		}
	}

	if (!accessRules.strRequireVhost.empty() && !std::wildcmp(accessRules.strRequireVhost, pUser->GetVirtualHost()))
	{
		return false;
	}
#endif

	if (!accessRules.strRequireRealname.empty() && !std::wildcmp(accessRules.strRequireRealname, pUser->GetRealname()))
	{
		return false;
	}

	if (!accessRules.strRequireHost.empty() && !std::wildcmp(accessRules.strRequireHost, pUser->GetHostname()))
	{
		return false;
	}

	if (!accessRules.strRequireIdent.empty() && !std::wildcmp(accessRules.strRequireIdent, pUser->GetIdent()))
	{
		return false;
	}

	if (!accessRules.strRequireNickname.empty() && !std::wildcmp(accessRules.strRequireNickname, pUser->GetNickname()))
	{
		return false;
	}

	return true;
}

int CBot::GetNumAccessLevels()
{
	return static_cast<int>(m_vecAccessRules.size());
}

CScript *CBot::CreateScript(const char *szFilename)
{
	CScript *pScript = new CScript(this);
	if (!pScript->Load(m_pParentCore, szFilename))
	{
		return NULL;
	}

	if (m_bGotScripts)
	{
		for (CPool<CIrcUser *>::iterator i = m_plGlobalUsers.begin(); i != m_plGlobalUsers.end(); ++i)
		{
			(*i)->NewScript(pScript);
		}
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
	if (m_bSynced)
	{
#if !defined(SERVICE) || IRCD != HYBRID
		SendMessage(CJoinMessage(szChannel));
#endif

#ifdef SERVICE
		CIrcChannel *pChannel = FindChannel(szChannel);
		if (pChannel == NULL)
		{
			pChannel = new CIrcChannel(this, szChannel);
#if IRCD == HYBRID
			pChannel->m_ullChannelStamp = time(NULL);
#endif
			m_plIrcChannels.push_back(pChannel);
		}

#if IRCD == HYBRID
		SendMessage(CJoinMessage(pChannel->m_ullChannelStamp, m_pIrcSocket->GetCurrentNickname(), szChannel));
#endif

		CALL_EVENT(OnBotJoinedChannel, pChannel);
#endif
	}
	else
	{
		CIrcChannel *pChannel = new CIrcChannel(this, szChannel);
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
		SendMessage(CPartMessage(pChannel->GetName(), szReason));
	}
	else
	{
		SendMessage(CPartMessage(pChannel->GetName()));
	}

#ifdef SERVICE
	if (pChannel->m_plIrcUsers.size() == 0)
	{
		delete pChannel;
		m_plIrcChannels.remove(pChannel);
	}
#endif

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

void CBot::SetQueueBlocked(bool bBlocked)
{
	m_pIrcSocket->m_bBlockQueue = bBlocked;
}

bool CBot::IsQueueBlocked()
{
	return m_pIrcSocket->m_bBlockQueue;
}

CScriptObject::eScriptType CBot::GetType()
{
	return Bot;
}
