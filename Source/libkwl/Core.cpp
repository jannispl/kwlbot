/*
kwlbot IRC bot


File:		Core.cpp
Purpose:	Core container which manages all bot instances

*/

#include "StdInc.h"
#include <iostream>
#include <stdlib.h>
#include "Core.h"
#include "ScriptEventManager.h"

#ifndef WIN32
#include <dirent.h>
#endif

CCore::CCore()
{
	printf("Initializing kwlbot\n");
	printf("Version %s\n", VERSION_STRING);

	printf("\n\n");

	m_pScriptEventManager = new CScriptEventManager(this);

	CConfig cfg("kwlbot.conf");
	if (cfg.StartValueList("modules"))
	{
		std::string strValue;
		while (cfg.GetNextValue(&strValue))
		{
			CGlobalModule *pGlobalModule = CreateGlobalModule(("modules/" + strValue).c_str());
		}
	}

	printf("\n");

	ScanDirectoryForBots("bots/");

	m_bRunning = true;
}

CCore::~CCore()
{
	for (CPool<CBot *>::iterator i = m_plBots.begin(); i != m_plBots.end(); ++i)
	{
		delete *i;
		i = m_plBots.erase(i);
		if (i == m_plBots.end())
		{
			break;
		}
	}

	for (CPool<CGlobalModule *>::iterator i = m_plGlobalModules.begin(); i != m_plGlobalModules.end(); ++i)
	{
		delete *i;
		i = m_plGlobalModules.erase(i);
		if (i == m_plGlobalModules.end())
		{
			break;
		}
	}

	for (CPool<CEventManager *>::iterator i = m_plEventManagers.begin(); i != m_plEventManagers.end(); ++i)
	{
		// We don't need to delete the instance here. The modules are responsible for doing it.
		i = m_plEventManagers.erase(i);
		if (i == m_plEventManagers.end())
		{
			break;
		}
	}

	delete m_pScriptEventManager;
}

bool CCore::IsRunning()
{
	return m_bRunning;
}

void CCore::Shutdown()
{
	m_bRunning = false;
}

CBot *CCore::CreateBot(CConfig *pConfig)
{
	CBot *pBot = new CBot(this, pConfig);
	m_plBots.push_back(pBot);
	return pBot;
}

bool CCore::DeleteBot(CBot *pBot)
{
	if (pBot == NULL)
	{
		return false;
	}

	m_plBots.remove(pBot);
	delete pBot;
	return true;
}

CGlobalModule *CCore::CreateGlobalModule(const char *szPath)
{
	CGlobalModule *pGlobalModule = new CGlobalModule(this, szPath);

	m_plGlobalModules.push_back(pGlobalModule);
	return pGlobalModule;
}

bool CCore::DeleteGlobalModule(CGlobalModule *pGlobalModule)
{
	if (pGlobalModule == NULL)
	{
		return false;
	}

	m_plGlobalModules.remove(pGlobalModule);
	delete pGlobalModule;
	return true;
}

CPool<CBot *> *CCore::GetBots()
{
	return &m_plBots;
}

CPool<CGlobalModule *> *CCore::GetGlobalModules()
{
	return &m_plGlobalModules;
}

void CCore::Pulse()
{
	for (CPool<CBot *>::iterator i = m_plBots.begin(); i != m_plBots.end(); ++i)
	{
		CBot *pBot = *i;
		
		if (!pBot->IsDead())
		{
			pBot->Pulse();
		}

		if (pBot->IsDead())
		{
			CBot::eDeathReason deathReason = pBot->GetDeathReason();

			if (deathReason == CBot::ConnectionError || deathReason == CBot::Restart)
			{
#ifdef ENABLE_AUTO_RECONNECT
				if (pBot->GetReconnectTimer() == (time_t)-1)
				{
					pBot->StartReconnectTimer(deathReason == CBot::ConnectionError ? AUTO_RECONNECT_TIMEOUT : BOT_RESTART_TIMEOUT);
				}
				else if (time(NULL) >= pBot->GetReconnectTimer())
				{
					CConfig botConfig = *(pBot->GetConfig());

					delete pBot;
					i.assign(CreateBot(&botConfig));
				}
#else
				delete pBot;
				i = m_plBots.erase(i);
				if (i == m_plBots.end())
				{
					break;
				}
#endif
			}
			else if (deathReason == CBot::UserRequest)
			{				
				delete pBot;
				i = m_plBots.erase(i);
				if (i == m_plBots.end())
				{
					break;
				}
			}
		}
	}
	
	for (CPool<CGlobalModule *>::iterator i = m_plGlobalModules.begin(); i != m_plGlobalModules.end(); ++i)
	{
		(*i)->Pulse();
	}
}

void CCore::ScanDirectoryForBots(const char *szDirectory)
{
#ifdef WIN32
	WIN32_FIND_DATAA fd;

	char *szPath = new char[strlen(szDirectory) + 2];
	sprintf(szPath, "%s*", szDirectory);
	HANDLE hDir = FindFirstFileA(szPath, &fd);
	delete[] szPath;
	if (hDir == INVALID_HANDLE_VALUE)
	{
		printf("Warning: No /bots/ directory, I will stay idle.\n");
		return;
	}
	do
	{
		if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			size_t len = strlen(fd.cFileName);
			if ((fd.cFileName[len - 3] == 'i' || fd.cFileName[len - 3] == 'I') &&
			(fd.cFileName[len - 2] == 'n' || fd.cFileName[len - 2] == 'N') &&
			(fd.cFileName[len - 1] == 'i' || fd.cFileName[len - 1] == 'I'))
			{
				CConfig Config((std::string(szDirectory) + fd.cFileName));

				CBot *pBot = CreateBot(&Config);
			}
		}
	}
	while (FindNextFileA(hDir, &fd));
	FindClose(hDir);
#else
	dirent *pEntry;
	DIR *pDir = opendir("./bots/");
	if (pDir == NULL)
	{
		printf("Warning: No /bots/ directory, I will stay idle\n");
		return;
	}
	do
	{
		pEntry = readdir(pDir);
		if (pEntry != NULL)
		{
			if (pEntry->d_type == DT_REG)
			{
				unsigned char len = strlen(pEntry->d_name);
				if ((pEntry->d_name[len - 3] == 'i' || pEntry->d_name[len - 3] == 'I') &&
				    (pEntry->d_name[len - 2] == 'n' || pEntry->d_name[len - 2] == 'N') &&
				    (pEntry->d_name[len - 1] == 'i' || pEntry->d_name[len - 1] == 'I'))
				{
					CConfig Config((std::string(szDirectory) + pEntry->d_name));

					CBot *pBot = CreateBot(&Config);
				}
			}
		}
	}
	while (pEntry != NULL);
#endif
}

CEventManager *CCore::GetScriptEventManager()
{
	return m_pScriptEventManager;
}

CPool<CEventManager *> *CCore::GetEventManagers()
{
	return &m_plEventManagers;
}

void CCore::AddEventManager(CEventManager *pEventManager)
{
	if (pEventManager == NULL)
	{
		return;
	}

	m_plEventManagers.push_back(pEventManager);
}
