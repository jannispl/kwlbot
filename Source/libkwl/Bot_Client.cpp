/*
kwlbot IRC bot


File:		Bot_Client.cpp
Purpose:	Class which represents an IRC bot
            Client implementation

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

#ifndef SERVICE

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
						
						CScriptFunctions::m_pCallingScript = pScript;
						(*i)->handlerFunction->Call((*i)->handlerFunction, iParamCount + 1, arguments);
						CScriptFunctions::m_pCallingScript = NULL;

						for (int i = 0; i < iParamCount + 1; ++i)
						{
							arguments[i].Dispose();
							arguments[i].Clear();
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
						
						CScriptFunctions::m_pCallingScript = pScript;
						(*i)->handlerFunction->Call((*i)->handlerFunction, iParamCount + 1, arguments);
						CScriptFunctions::m_pCallingScript = NULL;

						for (int j = 0; j < iParamCount + 1; ++j)
						{
							arguments[j].Dispose();
							arguments[j].Clear();
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
		if (iNumeric == 376 || iNumeric == 422)
		{
			m_bSynced = true;

			if (!m_strAutoMode.empty())
			{
				SendRawFormat("MODE %s %s", m_pIrcSocket->GetCurrentNickname(), m_strAutoMode.c_str());
			}

			CALL_EVENT_NOARG(OnBotConnected);

			for (CPool<CIrcChannel *>::iterator i = m_pIrcChannelQueue->begin(); i != m_pIrcChannelQueue->end(); ++i)
			{
				SendMessage(CJoinMessage((*i)->GetName()));

				delete *i;
			}

			delete m_pIrcChannelQueue;
			m_pIrcChannelQueue = NULL;

			printf("[%s] Initial synchronization finished\n", m_pIrcSocket->GetCurrentNickname());

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
			std::string strRealname = vecParams[7];
			std::string::size_type iSeparator = strRealname.find(' ');
			if (iSeparator == std::string::npos)
			{
				return;
			}
			strRealname = strRealname.substr(iSeparator + 1);

			Handle352(vecParams[5], vecParams[1], vecParams[2], vecParams[3], strRealname);
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

		if (strCommand == "TOPIC")
		{
			HandleTOPIC(vecParams[1],
#ifdef WIN32
				_atoi64(vecParams[2].c_str()),
#else
				strtoll(vecParams[2].c_str(), NULL, 10),
#endif
				vecParams[0], vecParams[3]);
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

	if (iNumeric == 5)
	{
		for (int i = 1; i < iParamCount; ++i)
		{
			std::string strSupport = vecParams[i];
			if (strSupport == "NAMESX")
			{
				// Tell the server we support NAMESX
				SendRawStatic("PROTOCTL NAMESX");
				m_supportSettings.bNamesX = true;

				continue;
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

void CBot::Handle352(const std::string &strNickname, const std::string &strChannel, const std::string &strIdent, const std::string &strHost, const std::string &strRealname)
{
	CIrcUser *pUser = FindUser(strNickname.c_str());
	if (pUser != NULL)
	{
		pUser->UpdateIfNecessary(strIdent, strHost);
		pUser->m_strRealname = strRealname;
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
				pUser = new CIrcUser(this, strName);
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
			std::map<CIrcChannel *, char >::iterator i = pVictim->m_mapChannelModes.find(pChannel);
			if (i != pVictim->m_mapChannelModes.end())
			{
				pVictim->m_mapChannelModes.erase(i);
			}

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
			std::map<CIrcChannel *, char >::iterator i = m_pCurrentUser->m_mapChannelModes.find(pChannel);
			if (i != m_pCurrentUser->m_mapChannelModes.end())
			{
				m_pCurrentUser->m_mapChannelModes.erase(i);
			}

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

void CBot::HandleTOPIC(const std::string &strSetter, time_t ullSetDate, const std::string &strChannel, const std::string &strTopic)
{
	CIrcChannel *pChannel = FindChannel(strChannel.c_str());
	if (pChannel == NULL)
	{
		return;
	}

	std::string strOldTopic = pChannel->m_topicInfo.strTopic;

	pChannel->m_topicInfo.ullTopicSetDate = ullSetDate;
	pChannel->m_topicInfo.strTopicSetBy = strSetter;
	pChannel->m_topicInfo.strTopic = strTopic;

	CIrcUser *pUser = pChannel->FindUser(strSetter.c_str());
	if (pUser != NULL)
	{
		CALL_EVENT(OnUserSetChannelTopic, pUser, pChannel, strOldTopic.c_str());
	}
	else
	{
		CIrcUser tempUser(this, strSetter, true);

		CALL_EVENT(OnUserSetChannelTopic, &tempUser, pChannel, strOldTopic.c_str());
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

	CIrcChannel *pChannel = NULL;
	bool bSelf = false;

	if (m_strCurrentNickname == m_pIrcSocket->GetCurrentNickname())
	{
		pChannel = new CIrcChannel(this, strChannel);
		m_plIrcChannels.push_back(pChannel);

		SendMessage(CWhoMessage(strChannel));

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
		m_pCurrentUser = new CIrcUser(this, m_strCurrentNickname);
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
		bool bPrefixMode = IsPrefixMode(cMode);

		// Modes in PREFIX are not listed but could be considered type B.
		cGroup = bPrefixMode ? 2 : GetModeGroup(cMode);

		switch (cGroup)
		{
			case 1:
			case 2: // Always parameter
			{
				if (static_cast<int>(vecParams.size()) >= iParamOffset + 1)
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
				if (iSet == 1 && static_cast<int>(vecParams.size()) >= iParamOffset + 1)
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

#endif
