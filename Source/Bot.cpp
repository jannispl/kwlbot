/*
kwlbot IRC bot


File:		Bot.cpp
Purpose:	Class which represents an IRC bot

*/

#include "StdInc.h"
#include "Bot.h"

CBot::CBot(CCore *pParentCore)
{
	m_pParentCore = pParentCore;

	m_pIrcSocket = new CIrcSocket(this);
	m_pIrcChannelQueue = new CPool<CIrcChannel *>();

	m_bGotMotd = false;
}

CBot::~CBot()
{
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
	TRACEFUNC("CBot::SendRaw");

	return m_pIrcSocket->SendRaw(szData);
}

int CBot::SendRawFormat(const char *szFormat, ...)
{
	TRACEFUNC("CBot::SendRawFormat");

	char szBuffer[IRC_MAX_LEN + 1];
	va_list vaArgs;
	va_start(vaArgs, szFormat);
	vsprintf(szBuffer, szFormat, vaArgs);
	va_end(vaArgs);

	return SendRaw(szBuffer);
}

void CBot::Pulse()
{
	TRACEFUNC("CBot::Pulse");

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

CIrcUser *CBot::FindUser(const char *szName)
{
	for (CPool<CIrcUser *>::iterator i = m_plGlobalUsers.begin(); i != m_plGlobalUsers.end(); ++i)
	{
		if (strcmp(szName, (*i)->GetName()) == 0)
		{
			return *i;
		}
	}
	return NULL;
}

char CBot::ModeToPrefix(char cMode)
{
	TRACEFUNC("CBot::ModeToPrefix");

	std::string::size_type iIndex = m_SupportSettings.strPrefixes[0].find(cMode);
	if (iIndex == std::string::npos)
	{
		return 0;
	}
	return m_SupportSettings.strPrefixes[1][iIndex];
}

char CBot::PrefixToMode(char cPrefix)
{
	TRACEFUNC("CBot::PrefixToMode");

	std::string::size_type iIndex = m_SupportSettings.strPrefixes[1].find(cPrefix);
	if (iIndex == std::string::npos)
	{
		return 0;
	}
	return m_SupportSettings.strPrefixes[0][iIndex];
}

bool CBot::IsPrefixMode(char cMode)
{
	TRACEFUNC("CBot::IsPrefixMode");

	return ModeToPrefix(cMode) != 0;
}

char CBot::GetModeGroup(char cMode)
{
	TRACEFUNC("CBot::GetModeGroup");

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
	TRACEFUNC("CBot::HandleData");

	DWORD dwTick = GetTickCount();

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
			for (CPool<CIrcChannel *>::iterator i = m_pIrcChannelQueue->begin(); i != m_pIrcChannelQueue->end(); ++i)
			{
				SendRawFormat("JOIN %s", (*i)->GetName());
				delete *i;
			}
			delete m_pIrcChannelQueue;
			m_pIrcChannelQueue = NULL;

			m_bGotMotd = true;

			v8::HandleScope handleScope;
			for (CPool<CScript *>::iterator i = m_pParentCore->GetScripts()->begin(); i != m_pParentCore->GetScripts()->end(); ++i)
			{
				(*i)->EnterContext();

				v8::Handle<v8::Value> argValues[1] = { GetScriptThis() };
				(*i)->CallEvent("onBotConnected", 1, argValues);

				(*i)->ExitContext();
			}
			return;
		}
	}

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

						if (strName != GetSettings()->GetNickname())
						{
							CIrcUser *pUser = FindUser(strName.c_str());
							if (pUser == NULL)
							{
								printf("We don't know %s yet, adding him (1).\n", strName.c_str());
								pUser = new CIrcUser(strName.c_str());
								m_plGlobalUsers.push_back(pUser);
							}
							else
							{
								printf("We already know %s.\n", pUser->GetName());
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

	if (strCommand == "JOIN")
	{
		std::string strChannel = vecParts[2];
		if (*(strChannel.begin()) == ':')
			strChannel = strChannel.substr(1);

		std::string strNickname = vecParts[0].substr(1);
		std::string::size_type iSeparator = strNickname.find('!');
		if (iSeparator != std::string::npos)
		{
			strNickname = strNickname.substr(0, iSeparator);
			CIrcChannel *pChannel = NULL;
			bool bSelf = false;
			if (strNickname == GetSettings()->GetNickname())
			{
				pChannel = new CIrcChannel(strChannel.c_str());
				m_plIrcChannels.push_back(pChannel);
				printf("WE JOINED '%s'\n", pChannel->GetName());

				bSelf = true;
			}
			else
			{
				pChannel = FindChannel(strChannel.c_str());
				if (pChannel != NULL)
				{
					printf("'%s' JOINED '%s'\n", strNickname.c_str(), pChannel->GetName());
				}
			}
			if (pChannel != NULL)
			{
				CIrcUser *pUser = FindUser(strNickname.c_str());
				if (pUser == NULL)
				{
					printf("We don't know %s yet, adding him (2).\n", strNickname.c_str());
					pUser = new CIrcUser(strNickname.c_str());
					m_plGlobalUsers.push_back(pUser);
				}
				else
				{
					printf("We already know %s.\n", strNickname.c_str());
				}
				pUser->m_plIrcChannels.push_back(pChannel);
				pChannel->m_plIrcUsers.push_back(pUser);

				if (bSelf)
				{
					v8::HandleScope handleScope;
					for (CPool<CScript *>::iterator i = m_pParentCore->GetScripts()->begin(); i != m_pParentCore->GetScripts()->end(); ++i)
					{
						(*i)->EnterContext();

						v8::Handle<v8::Value> argValues[2] = { GetScriptThis(), pChannel->GetScriptThis() };
						(*i)->CallEvent("onBotJoinedChannel", 2, argValues);

						(*i)->ExitContext();
					}
				}
				else
				{
					v8::HandleScope handleScope;
					for (CPool<CScript *>::iterator i = m_pParentCore->GetScripts()->begin(); i != m_pParentCore->GetScripts()->end(); ++i)
					{
						(*i)->EnterContext();

						v8::Handle<v8::Value> argValues[3] = { GetScriptThis(), pUser->GetScriptThis(), pChannel->GetScriptThis() };
						(*i)->CallEvent("onUserJoinedChannel", 3, argValues);

						(*i)->ExitContext();
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

			CIrcChannel *pChannel = FindChannel(strChannel.c_str());
			if (pChannel != NULL)
			{
				CIrcUser *pUser = FindUser(strNickname.c_str());
				if (pUser != NULL)
				{
					v8::HandleScope handleScope;
					for (CPool<CScript *>::iterator i = m_pParentCore->GetScripts()->begin(); i != m_pParentCore->GetScripts()->end(); ++i)
					{
						(*i)->EnterContext();

						v8::Handle<v8::Value> argValues[3] = { GetScriptThis(), pUser->GetScriptThis(), pChannel->GetScriptThis() };
						(*i)->CallEvent("onUserLeftChannel", 3, argValues);

						(*i)->ExitContext();
					}

					printf("Removing %s from %s.\n", pUser->GetName(), pChannel->GetName());
					pUser->m_plIrcChannels.remove(pChannel);
					pChannel->m_plIrcUsers.remove(pUser);
					if (pUser->m_plIrcChannels.size() == 0)
					{
						printf("We don't know %s anymore.\n", pUser->GetName());
						m_plGlobalUsers.remove(pUser);
						delete pUser;
					}
				}
				else
				{
					printf("Error: we don't know %s yet, for some reason.\n", strNickname.c_str());
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
			std::string::size_type iTempSpace = vecParts[3].find(' ');

			std::string strVictim = vecParts[3].substr(0, iTempSpace);
			std::string strReason = iTempSpace != std::string::npos ? vecParts[3].substr(iTempSpace + 1) : vecParts[3];

			CIrcChannel *pChannel = FindChannel(strChannel.c_str());
			if (pChannel != NULL)
			{
				CIrcUser *pVictim = FindUser(strVictim.c_str());
				if (pVictim != NULL)
				{
					CIrcUser *pUser = FindUser(strNickname.c_str());
					if (pUser != NULL)
					{
						v8::HandleScope handleScope;
						for (CPool<CScript *>::iterator i = m_pParentCore->GetScripts()->begin(); i != m_pParentCore->GetScripts()->end(); ++i)
						{
							(*i)->EnterContext();

							v8::Handle<v8::Value> argValues[4] = { GetScriptThis(), pUser->GetScriptThis(), pVictim->GetScriptThis(), pChannel->GetScriptThis() };
							(*i)->CallEvent("onUserKickedUser", 4, argValues);

							(*i)->ExitContext();
						}
					}

					printf("Removing %s from %s.\n", pVictim->GetName(), pChannel->GetName());
					pUser->m_plIrcChannels.remove(pChannel);
					pChannel->m_plIrcUsers.remove(pVictim);
					if (pVictim->m_plIrcChannels.size() == 0)
					{
						printf("We don't know %s anymore.\n", pVictim->GetName());
						m_plGlobalUsers.remove(pVictim);
						delete pVictim;
					}
				}
				else
				{
					printf("Error: we don't know %s yet, for some reason.\n", strVictim.c_str());
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
			std::string strReason = vecParts[2].substr(1) + vecParts[3];

			CIrcUser *pUser = FindUser(strNickname.c_str());
			if (pUser != NULL)
			{
				v8::HandleScope handleScope;
				for (CPool<CScript *>::iterator i = m_pParentCore->GetScripts()->begin(); i != m_pParentCore->GetScripts()->end(); ++i)
				{
					(*i)->EnterContext();

					v8::Handle<v8::Value> argValues[2] = { GetScriptThis(), pUser->GetScriptThis() };
					(*i)->CallEvent("onUserQuit", 2, argValues);

					(*i)->ExitContext();
				}

				for (CPool<CIrcChannel *>::iterator i = m_plIrcChannels.begin(); i != m_plIrcChannels.end(); ++i)
				{
					(*i)->m_plIrcUsers.remove(pUser);
				}

				printf("We don't know %s anymore.\n", pUser->GetName());
				m_plGlobalUsers.remove(pUser);
				delete pUser;
			}
			else
			{
				printf("Error: we don't know %s yet, for some reason.\n", strNickname.c_str());
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

			std::string strNewNick = vecParts[2];
			if (*(strNewNick.begin()) == '!')
			{
				strNewNick = strNewNick.substr(1);
			}

			CIrcUser *pUser = FindUser(strNickname.c_str());
			if (pUser != NULL)
			{
				printf("Renaming %s to %s.\n", pUser->GetName(), strNewNick.c_str());
				pUser->SetName(strNewNick.c_str());

				v8::HandleScope handleScope;
				for (CPool<CScript *>::iterator i = m_pParentCore->GetScripts()->begin(); i != m_pParentCore->GetScripts()->end(); ++i)
				{
					(*i)->EnterContext();

					v8::Handle<v8::Value> argValues[3] = { GetScriptThis(), pUser->GetScriptThis(), v8::String::New(strNickname.c_str()) };
					(*i)->CallEvent("onUserQuit", 3, argValues);

					(*i)->ExitContext();
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

			std::string strTarget = vecParts[2];
			std::string strMessage = vecParts[3];
			if (*(strMessage.begin()) == ':')
			{
				strMessage = strMessage.substr(1);
			}

			if (strTarget == GetSettings()->GetNickname())
			{
				// privmsg to bot
				printf("[priv] <%s> %s\n", strNickname.c_str(), strMessage.c_str());
			}
			else
			{
				CIrcChannel *pChannel = FindChannel(strTarget.c_str());
				if (pChannel != NULL)
				{
					printf("[%s] <%s> %s\n", strTarget.c_str(), strNickname.c_str(), strMessage.c_str());

					CIrcUser *pUser = FindUser(strNickname.c_str());
					if (pUser != NULL)
					{
						v8::HandleScope handleScope;
						for (CPool<CScript *>::iterator i = m_pParentCore->GetScripts()->begin(); i != m_pParentCore->GetScripts()->end(); ++i)
						{
							(*i)->EnterContext();

							v8::Handle<v8::Object> This/* = GetScriptThis()*/;
							v8::Handle<v8::Object> UserThis/* = pUser->GetScriptThis()*/;
							v8::Handle<v8::Object> ChannelThis/* = pChannel->GetScriptThis()*/;
							v8::Handle<v8::Value> Message = v8::String::New(strMessage.c_str());

							v8::Local<v8::Function> ctor1 = CScript::m_ClassTemplates.Bot->GetFunction();
							v8::Local<v8::Function> ctor2 = CScript::m_ClassTemplates.IrcUser->GetFunction();
							v8::Local<v8::Function> ctor3 = CScript::m_ClassTemplates.IrcChannel->GetFunction();
							
							This = ctor1->NewInstance();
							This->SetInternalField(0, v8::External::New(this));

							UserThis = ctor2->NewInstance();
							UserThis->SetInternalField(0, v8::External::New(pUser));

							ChannelThis = ctor3->NewInstance();
							ChannelThis->SetInternalField(0, v8::External::New(pChannel));

							v8::Handle<v8::Value> argValues[4] = { This, UserThis, ChannelThis, Message };
							(*i)->CallEvent("onUserChannelMessage", 4, argValues);

							(*i)->ExitContext();
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
		if (iSeparator != std::string::npos)
		{
			strNickname = strNickname.substr(0, iSeparator);
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
				std::string strNickname = vecParts[0].substr(1);
				std::string::size_type iSeparator = strNickname.find('!');
				if (iSeparator != std::string::npos)
				{
					strNickname = strNickname.substr(0, iSeparator);

					v8::HandleScope handleScope;
					for (CPool<CScript *>::iterator i = m_pParentCore->GetScripts()->begin(); i != m_pParentCore->GetScripts()->end(); ++i)
					{
						(*i)->EnterContext();

						v8::Handle<v8::Value> argValues[5] = { GetScriptThis(), pUser->GetScriptThis(), pChannel->GetScriptThis(), v8::String::New(strModes.c_str()), v8::String::New(strParams.c_str()) };
						(*i)->CallEvent("onUserSetChannelModes", 5, argValues);

						(*i)->ExitContext();
					}
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
							printf("1MODEEEEEEE %c%c %s!\n", iSet == 1 ? '+' : '-', cMode, strParam.c_str());

							if (bPrefixMode)
							{
								CIrcUser *pUser = FindUser(strParam.c_str());

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
							printf("2MODEEEEEEE +%c %s!\n", cMode, strParam.c_str());

							iLastPos = strParams.find_first_not_of(' ', iPos);
							iPos = strParams.find_first_of(' ', iLastPos);
						}
						else if (iSet == 2)
						{
							printf("2MODEEEEEEE -%c!\n", cMode);
						}
						break;
					}

					case 4: // No parameter
					{
						printf("3MODEEEEEEE %c%c!\n", iSet == 1 ? '+' : '-', cMode);
						break;
					}
				}
			}
		}
		return;
	}
}

void CBot::JoinChannel(const char *szChannel)
{
	CIrcChannel *pChannel = new CIrcChannel(szChannel);
	if (m_bGotMotd)
	{
		SendRawFormat("JOIN %s", szChannel);
	}
	else
	{
		m_pIrcChannelQueue->push_back(pChannel);
	}
}

v8::Handle<v8::Value> CBot::GetScriptThis()
{
	v8::HandleScope handleScope;
	v8::Local<v8::Object> obj = CScript::m_ClassTemplates.Bot->GetFunction()->NewInstance();
	obj->SetInternalField(0, v8::External::New(this));
	return obj;
}
