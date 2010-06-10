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
	TRACEFUNC("CCore::CCore");
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

	if (cfg.StartValueList("scripts"))
	{
		std::string strValue;
		while (cfg.GetNextValue(&strValue))
		{
			CScript *pScript = CreateScript(("scripts/" + strValue).c_str());
		}
	}

	ScanDirectoryForBots("bots/");

	m_bRunning = true;
}

CCore::~CCore()
{
	TRACEFUNC("CCore::~CCore");

	for (CPool<CScript *>::iterator i = m_plScripts.begin(); i != m_plScripts.end(); ++i)
	{
		delete *i;
		if ((i = m_plScripts.erase(i)) == m_plScripts.end())
		{
			break;
		}
	}

	for (CPool<CBot *>::iterator i = m_plBots.begin(); i != m_plBots.end(); ++i)
	{
		delete *i;
		if ((i = m_plBots.erase(i)) == m_plBots.end())
		{
			break;
		}
	}

	for (CPool<CGlobalModule *>::iterator i = m_plGlobalModules.begin(); i != m_plGlobalModules.end(); ++i)
	{
		delete *i;
		if ((i = m_plGlobalModules.erase(i)) == m_plGlobalModules.end())
		{
			break;
		}
	}

	for (CPool<CEventManager *>::iterator i = m_plEventManagers.begin(); i != m_plEventManagers.end(); ++i)
	{
		//delete *i; /* FIXME */
		if ((i = m_plEventManagers.erase(i)) == m_plEventManagers.end())
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
	TRACEFUNC("CCore::CreateBot");

	CBot *pBot = new CBot(this, pConfig);
	m_plBots.push_back(pBot);
	return pBot;
}

bool CCore::DeleteBot(CBot *pBot)
{
	TRACEFUNC("CCore::DeleteBot");

	if (pBot == NULL)
	{
		return false;
	}

	m_plBots.remove(pBot);
	delete pBot;
	return true;
}

CScript *CCore::CreateScript(const char *szFilename)
{
	TRACEFUNC("CCore::CreateScript");

	CScript *pScript = new CScript(this);
	if (!pScript->Load(szFilename))
	{
		return NULL;
	}

	m_plScripts.push_back(pScript);
	return pScript;
}

bool CCore::DeleteScript(CScript *pScript)
{
	TRACEFUNC("CCore::DeleteScript");

	if (pScript == NULL)
	{
		return false;
	}

	m_plScripts.remove(pScript);
	delete pScript;
	return true;
}

CGlobalModule *CCore::CreateGlobalModule(const char *szPath)
{
	TRACEFUNC("CCore::CreateGlobalModule");

	CGlobalModule *pGlobalModule = new CGlobalModule(this, szPath);

	m_plGlobalModules.push_back(pGlobalModule);
	return pGlobalModule;
}

bool CCore::DeleteGlobalModule(CGlobalModule *pGlobalModule)
{
	TRACEFUNC("CCore::DeleteGlobalModule");

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

CPool<CScript *> *CCore::GetScripts()
{
	return &m_plScripts;
}

CPool<CGlobalModule *> *CCore::GetGlobalModules()
{
	return &m_plGlobalModules;
}

void CCore::Pulse()
{
	TRACEFUNC("CCore::Pulse");

	for (CPool<CBot *>::iterator i = m_plBots.begin(); i != m_plBots.end(); ++i)
	{
		(*i)->Pulse();
	}
	
	for (CPool<CGlobalModule *>::iterator i = m_plGlobalModules.begin(); i != m_plGlobalModules.end(); ++i)
	{
		(*i)->Pulse();
	}
}

void CCore::ScanDirectoryForBots(const char *szDirectory)
{
	TRACEFUNC("CCore::ScanDirectoryForBots");

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

				std::string strTemp;
				if (Config.GetSingleValue("server", &strTemp))
				{
					std::string::size_type iPortSep = strTemp.find(':');
					if (iPortSep == std::string::npos)
					{
						pBot->GetSocket()->Connect(strTemp.c_str());
					}
					else
					{
						pBot->GetSocket()->Connect(strTemp.substr(0, iPortSep).c_str(), atoi(strTemp.substr(iPortSep + 1).c_str()));
					}
				}
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

					std::string strTemp;
					if (Config.GetSingleValue("server", &strTemp))
					{
						std::string::size_type iPortSep = strTemp.find(':');
						if (iPortSep == std::string::npos)
						{
							pBot->GetSocket()->Connect(strTemp.c_str());
						}
						else
						{
							pBot->GetSocket()->Connect(strTemp.substr(0, iPortSep).c_str(), atoi(strTemp.substr(iPortSep + 1).c_str()));
						}
					}
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
