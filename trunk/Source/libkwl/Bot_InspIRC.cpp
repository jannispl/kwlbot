/*
kwlbot IRC bot


File:		Bot_InspIRC.cpp
Purpose:	Class which represents an IRC bot
            InspIRCd service implementation

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

#if defined(SERVICE) && IRCD == INSPIRCD

void CBot::HandleData(const std::string &strOrigin, const std::string &strCommand, const std::vector<std::string> &vecParams)
{
	int iParamCount = vecParams.size();
	int iNumeric;

	m_strCurrentOrigin = strOrigin;

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
		m_strCurrentNickname = strOrigin;

		if (iNumeric == 0 && m_strCurrentNickname.length() > 3 && m_szServerId[0] != '\0' && strncmp(m_szServerId, m_strCurrentNickname.c_str(), 3) == 0) // A numeric reply is not allowed to originate from a client (RFC 2812)
		{
			m_pCurrentUser = FindUserByUid(m_strCurrentNickname);
		}
	}

	static v8::Persistent<v8::Value> arguments[16];

	for (CPool<CScript *>::iterator i = m_plScripts.begin(); i != m_plScripts.end(); ++i)
	{
		CScript *pScript = *i;
		bool bEntered = false;

		for (CPool<CScript::CommandSubscription *>::iterator i = pScript->m_plCommandSubscriptions.begin(); i != pScript->m_plCommandSubscriptions.end(); ++i)
		{
			if (stricmp((*i)->strCommand.c_str(), strCommand.c_str()) == 0)
			{
				v8::HandleScope handleScope;
				v8::Local<v8::Function> ctorIrcUser;
				v8::Local<v8::Function> ctorIrcChannel;

				if (!bEntered)
				{
					pScript->EnterContext();

					ctorIrcUser = CScript::m_classTemplates.IrcUser->GetFunction();
					ctorIrcChannel = CScript::m_classTemplates.IrcChannel->GetFunction();

					bEntered = true;
				}

				if ((*i)->iNumArguments == -1 || (*i)->iNumArguments == iParamCount)
				{
					if ((*i)->strFormat.empty())
					{
						arguments[0] = v8::Persistent<v8::Value>::New(v8::String::New(strOrigin.c_str(), strOrigin.length()));

						int iNum = 1;
						for (std::vector<std::string>::const_iterator j = vecParams.begin(); j != vecParams.end(); ++j)
						{
							arguments[iNum++] = v8::Persistent<v8::Value>::New(v8::String::New(j->c_str(), j->length()));
						}

						(*i)->handlerFunction->Call((*i)->handlerFunction, iParamCount + 1, arguments);

						for (int i = 0; i < iParamCount + 1; ++i)
						{
							arguments[i].Dispose();
						}
					}
					else
					{
						const std::string &strFormat = (*i)->strFormat;

						switch (strFormat[0])
						{
						case 's':
							arguments[0] = v8::Persistent<v8::Value>::New(v8::String::New(strOrigin.c_str(), strOrigin.length()));
							break;
						case 'u':
							if (m_pCurrentUser == NULL)
							{
								arguments[0] = v8::Persistent<v8::Value>::New(v8::String::New(m_strCurrentNickname.c_str(), m_strCurrentNickname.length()));
							}
							else
							{
								CScriptFunctions::m_bAllowInternalConstructions = true;

								arguments[0] = v8::Persistent<v8::Value>::New(ctorIrcUser->NewInstance());
								arguments[0]->ToObject()->SetInternalField(0, v8::External::New(m_pCurrentUser));

								CScriptFunctions::m_bAllowInternalConstructions = false;
							}
							break;
						}

						for (std::string::size_type j = 1; j < strFormat.length() && j <= vecParams.size(); ++j)
						{
							const std::string &strParam = vecParams[j - 1];
							v8::HandleScope handleScope;

							switch (strFormat[j])
							{
							case 's':
								arguments[j] = v8::Persistent<v8::Value>::New(v8::String::New(strParam.c_str(), strParam.length()));
								break;
							case 'i':
								arguments[j] = v8::Persistent<v8::Value>::New(v8::Int32::New(atoi(strParam.c_str())));
								break;
							case 'f':
								arguments[j] = v8::Persistent<v8::Value>::New(v8::Number::New(atof(strParam.c_str())));
								break;
							case 'c':
								{
									CIrcChannel *pChannel = NULL;
									if (strParam[0] != '#' || (pChannel = FindChannel(strParam.c_str())) == NULL)
									{
										arguments[j] = v8::Persistent<v8::Value>::New(v8::String::New(strParam.c_str(), strParam.length()));
									}
									else
									{
										CScriptFunctions::m_bAllowInternalConstructions = true;

										arguments[j] = v8::Persistent<v8::Value>::New(ctorIrcChannel->NewInstance());
										arguments[j]->ToObject()->SetInternalField(0, v8::External::New(pChannel));

										CScriptFunctions::m_bAllowInternalConstructions = false;
									}
								}
								break;
							case 'u':
								{
									CIrcUser *pUser = FindUser(strParam.c_str());
									if (pUser == NULL)
									{
										arguments[j] = v8::Persistent<v8::Value>::New(v8::String::New(strParam.c_str(), strParam.length()));
									}
									else
									{
										CScriptFunctions::m_bAllowInternalConstructions = true;

										arguments[j] = v8::Persistent<v8::Value>::New(ctorIrcUser->NewInstance());
										arguments[j]->ToObject()->SetInternalField(0, v8::External::New(pUser));

										CScriptFunctions::m_bAllowInternalConstructions = false;
									}
								}
								break;
							case 'C':
								{
									CIrcChannel *pChannel = NULL;
									if (strParam[0] != '#' || (pChannel = FindChannel(strParam.c_str())) == NULL)
									{
										CIrcUser *pUser = FindUser(strParam.c_str());
										if (pUser == NULL)
										{
											arguments[j] = v8::Persistent<v8::Value>::New(v8::String::New(strParam.c_str(), strParam.length()));
										}
										else
										{
											CScriptFunctions::m_bAllowInternalConstructions = true;

											arguments[j] = v8::Persistent<v8::Value>::New(ctorIrcUser->NewInstance());
											arguments[j]->ToObject()->SetInternalField(0, v8::External::New(pUser));

											CScriptFunctions::m_bAllowInternalConstructions = false;
										}
									}
									else
									{
										CScriptFunctions::m_bAllowInternalConstructions = true;

										arguments[j] = v8::Persistent<v8::Value>::New(ctorIrcChannel->NewInstance());
										arguments[j]->ToObject()->SetInternalField(0, v8::External::New(pChannel));

										CScriptFunctions::m_bAllowInternalConstructions = false;
									}
								}
								break;
							case 'U':
								{
									CIrcUser *pUser = FindUser(strParam.c_str());
									if (pUser == NULL)
									{
										CIrcChannel *pChannel = NULL;
										if (strParam[0] != '#' || (pChannel = FindChannel(strParam.c_str())) == NULL)
										{
											arguments[j] = v8::Persistent<v8::Value>::New(v8::String::New(strParam.c_str(), strParam.length()));
										}
										else
										{
											CScriptFunctions::m_bAllowInternalConstructions = true;

											arguments[j] = v8::Persistent<v8::Value>::New(ctorIrcChannel->NewInstance());
											arguments[j]->ToObject()->SetInternalField(0, v8::External::New(pChannel));

											CScriptFunctions::m_bAllowInternalConstructions = false;
										}
									}
									else
									{
										CScriptFunctions::m_bAllowInternalConstructions = true;

										arguments[j] = v8::Persistent<v8::Value>::New(ctorIrcUser->NewInstance());
										arguments[j]->ToObject()->SetInternalField(0, v8::External::New(pUser));

										CScriptFunctions::m_bAllowInternalConstructions = false;
									}
								}
								break;
							}
						}

						(*i)->handlerFunction->Call((*i)->handlerFunction, iParamCount + 1, arguments);

						for (int j = 0; j < iParamCount + 1; ++j)
						{
							arguments[j].Dispose();
						}
					}
				}
			}
		}

		if (bEntered)
		{
			pScript->ExitContext();
		}
	}

	if (!m_bSynced)
	{
		if (strCommand == "ENDBURST")
		{
			m_bSynced = true;

			SendRawStatic("ENDBURST");

			CALL_EVENT_NOARG(OnBotConnected);

			for (CPool<CIrcChannel *>::iterator i = m_pIrcChannelQueue->begin(); i != m_pIrcChannelQueue->end(); ++i)
			{
				SendMessage(CJoinMessage((*i)->GetName()));

				delete *i;
			}

			delete m_pIrcChannelQueue;
			m_pIrcChannelQueue = NULL;

			return;
		}
	}

	if (iParamCount == 10)
	{
		if (strCommand == "UID")
		{
			HandleUID(vecParams[0],
#ifdef WIN32
				_atoi64(vecParams[1].c_str()),
#else
				strtoll(vecParams[1].c_str(), NULL, 10),
#endif
				vecParams[2], vecParams[3], vecParams[4], vecParams[5], vecParams[6],
#ifdef WIN32
				_atoi64(vecParams[7].c_str()),
#else
				strtoll(vecParams[7].c_str(), NULL, 10),
#endif
				vecParams[8], vecParams[9]);
			return;
		}
	}
	else if (iParamCount == 5)
	{
		if (strCommand == "SERVER")
		{
			HandleSERVER(vecParams[0], vecParams[1], atoi(vecParams[2].c_str()), vecParams[3], vecParams[4]);
			return;
		}
	}
	else if (iParamCount == 4)
	{
		if (strCommand == "FTOPIC")
		{
			HandleFTOPIC(vecParams[0],
#ifdef WIN32
				_atoi64(vecParams[1].c_str()),
#else
				strtoll(vecParams[1].c_str(), NULL, 10),
#endif
				vecParams[2], vecParams[3]);
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
	}
	else if (iParamCount == 2)
	{
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

		if (strCommand == "NICK")
		{
			HandleNICK(vecParams[0]);
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

		if (strCommand == "FHOST")
		{
			HandleFHOST(vecParams[0]);
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

	if (strCommand == "FJOIN")
	{
		HandleFJOIN(vecParams[0],
#ifdef WIN32
				_atoi64(vecParams[1].c_str()),
#else
				strtoll(vecParams[1].c_str(), NULL, 10),
#endif
				vecParams[iParamCount - 1][0] == '+' ? "" : vecParams[iParamCount - 1]);
		return;
	}
}

CIrcUser *CBot::FindUserByUid(const std::string &strUid)
{
	for (CPool<CIrcUser *>::iterator i = m_plGlobalUsers.begin(); i != m_plGlobalUsers.end(); ++i)
	{
		if (strncmp((*i)->m_szUserId, strUid.c_str(), 9) == 0)
		{
			return *i;
		}
	}

	return NULL;
}

void CBot::HandleJOIN(const std::string &strChannel)
{
	if (strChannel.find(',') != std::string::npos)
	{
		std::string::size_type iLastPos = strChannel.find_first_not_of(',', 0);
		std::string::size_type iPos = strChannel.find_first_of(',', iLastPos);
		while (iPos != std::string::npos || iLastPos != std::string::npos)
		{
			std::string strChannel_ = strChannel.substr(iLastPos, iPos - iLastPos);

			HandleJOIN(strChannel_);

			iLastPos = strChannel.find_first_not_of(',', iPos);
			iPos = strChannel.find_first_of(',', iLastPos);
		}

		return;
	}

	CIrcChannel *pChannel = FindChannel(strChannel.c_str());
	bool bSelf = false;

	if (pChannel == NULL)
	{
		pChannel = new CIrcChannel(this, strChannel);
		m_plIrcChannels.push_back(pChannel);
	}

	if (m_pCurrentUser == NULL)
	{
		m_pCurrentUser = new CIrcUser(this, m_strCurrentNickname);
		m_plGlobalUsers.push_back(m_pCurrentUser);
	}

	m_pCurrentUser->m_plIrcChannels.push_back(pChannel);
	pChannel->m_plIrcUsers.push_back(m_pCurrentUser);

	CALL_EVENT(OnUserJoinedChannel, m_pCurrentUser, pChannel);
}

void CBot::HandleQUIT(const std::string &strReason)
{
	if (m_pCurrentUser != NULL)
	{
		CALL_EVENT(OnUserQuit, m_pCurrentUser, strReason.c_str());

		for (CPool<CIrcChannel *>::iterator i = m_plIrcChannels.begin(); i != m_plIrcChannels.end(); ++i)
		{
			if ((*i)->m_plIrcUsers.size() == 1)
			{
				delete *i;
				i = m_plIrcChannels.erase(i);
				if (i == m_plIrcChannels.end())
				{
					break;
				}
			}
			else
			{
				(*i)->m_plIrcUsers.remove(m_pCurrentUser);
			}
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
		m_pCurrentUser->m_strNickname = strNewNickname;

		CALL_EVENT(OnUserChangedNickname, m_pCurrentUser, m_strCurrentNickname.c_str());
	}
	else
	{
		CIrcUser tempUser(this, strNewNickname, true);
		tempUser.UpdateIfNecessary(m_strCurrentIdent, m_strCurrentHostname);

		CALL_EVENT(OnUserChangedNickname, &tempUser, m_strCurrentNickname.c_str());
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
		if (pChannel->m_plIrcUsers.size() == 0)
		{
			m_plIrcChannels.remove(pChannel);
			delete pChannel;
		}
	}
	else
	{
		if (m_pCurrentUser != NULL)
		{
			CALL_EVENT(OnUserLeftChannel, m_pCurrentUser, pChannel, strReason.c_str());

			m_pCurrentUser->m_plIrcChannels.remove(pChannel);
			pChannel->m_plIrcUsers.remove(m_pCurrentUser);
		}
		else
		{
			CIrcUser tempUser(this, m_strCurrentNickname, true);
			tempUser.UpdateIfNecessary(m_strCurrentIdent, m_strCurrentHostname);

			CALL_EVENT(OnUserLeftChannel, &tempUser, pChannel, strReason.c_str());
		}
	}

	if (pChannel->m_plIrcUsers.size() == 0)
	{
		m_plIrcChannels.remove(pChannel);
		delete pChannel;
	}
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
		if (pChannel->m_plIrcUsers.size() == 0)
		{
			m_plIrcChannels.remove(pChannel);
			delete pChannel;
		}
	}
	else
	{
		CIrcUser *pVictim = FindUserByUid(strVictim);

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
		}
		/*
		else
		{
			// The victim *must* be on the channel to be kicked, so we'll not handle this
		}
		*/
	}

	if (pChannel->m_plIrcUsers.size() == 0)
	{
		m_plIrcChannels.remove(pChannel);
		delete pChannel;
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
	if (m_pCurrentUser == NULL)
	{
		return;
	}

	CIrcChannel *pChannel = FindChannel(strChannel.c_str());
	if (pChannel == NULL)
	{
		return;
	}

	pChannel->m_topicInfo.strTopic = strTopic;
	pChannel->m_topicInfo.strTopicSetBy = m_pCurrentUser->GetNickname();
	pChannel->m_topicInfo.ullTopicSetDate = time(NULL);
}

void CBot::HandleSERVER(const std::string &strRemoteHost, const std::string &strPassword, int iHopCount, const std::string &strServerId, const std::string &strInformation)
{
	if (strServerId.length() < 3)
	{
		return;
	}

	memcpy(m_szServerId, strServerId.c_str(), 3);
}

void CBot::HandleUID(const std::string &strUid, time_t ullTimestamp, const std::string &strNickname, const std::string &strHostname1, const std::string &strHostname2, const std::string &strIdent, const std::string &strIp, time_t ullSignonTime, const std::string &strUsermodes, const std::string &strRealname)
{
	if (strUid.length() <= 3)
	{
		return;
	}

	CIrcUser *pUser = new CIrcUser(this, strNickname);
	pUser->UpdateIfNecessary(strIdent, strHostname1);

	memcpy(pUser->m_szUserId, strUid.c_str(), 9);
	pUser->m_strCloakedHost = strHostname2;
	pUser->m_strRealname = strRealname;

	m_plGlobalUsers.push_back(pUser);
}

void CBot::HandleFJOIN(const std::string &strChannel, time_t ullTimestamp, const std::string &strUsers)
{
	CIrcChannel *pChannel = FindChannel(strChannel.c_str());
	if (pChannel == NULL)
	{
		pChannel = new CIrcChannel(this, strChannel);
		m_plIrcChannels.push_back(pChannel);
	}

	std::string::size_type iLastPos = strUsers.find_first_not_of(' ', 0);
	std::string::size_type iPos = strUsers.find_first_of(' ', iLastPos);
	while (iPos != std::string::npos || iLastPos != std::string::npos)
	{
		std::string strUser = strUsers.substr(iLastPos, iPos - iLastPos);

		std::string::size_type iComma = strUser.find(',');
		if (iComma != std::string::npos)
		{
			std::string strModes = strUser.substr(0, iComma);
			std::string strUid = strUser.substr(iComma + 1);

			CIrcUser *pUser = FindUserByUid(strUid);
			if (pUser != NULL)
			{
				for (std::string::iterator i = strModes.begin(); i != strModes.end(); ++i)
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
						pUser->m_mapChannelModes[pChannel] |= cNewMode;
					}
				}

				pUser->m_plIrcChannels.push_back(pChannel);
				pChannel->m_plIrcUsers.push_back(pUser);
			}
		}

		iLastPos = strUsers.find_first_not_of(' ', iPos);
		iPos = strUsers.find_first_of(' ', iLastPos);
	}
}

void CBot::HandleFTOPIC(const std::string &strChannel, time_t ullTimestamp, const std::string &strSetter, const std::string &strTopic)
{
	CIrcChannel *pChannel = FindChannel(strChannel.c_str());
	if (pChannel == NULL)
	{
		return;
	}

	pChannel->m_topicInfo.strTopic = strTopic;
	pChannel->m_topicInfo.strTopicSetBy = strSetter;
	pChannel->m_topicInfo.ullTopicSetDate = ullTimestamp;
}

void CBot::HandleFHOST(const std::string &strNewHost)
{
	if (m_pCurrentUser == NULL)
	{
		return;
	}

	m_pCurrentUser->m_strCloakedHost = strNewHost;
}

#endif
