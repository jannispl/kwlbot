/*
kwlbot IRC bot


File:		Core.cpp
Purpose:	Core container which manages all bot instances

*/

#include "StdInc.h"
#include <iostream>
#include <stdlib.h>
#include "Core.h"
#include "Config.h"

#ifndef WIN32
#include <dirent.h>
#endif

CCore::CCore()
{
	TRACEFUNC("CCore::CCore");

	printf("Initializing kwlbot\n");
	printf("Version %s\n", VERSION_STRING);

	printf("\n\n");

	m_pEventManager = new CEventManager(this);

	CConfig cfg("core.cfg");
	if (cfg.StartValueList("scripts"))
	{
		std::string strValue;
		while (cfg.GetNextValue(&strValue))
		{
			CScript *pScript = CreateScript(("scripts/" + strValue).c_str());
		}
	}

	ScanDirectoryForBots("bots/");
}

CCore::~CCore()
{
	TRACEFUNC("CCore::~CCore");

	delete m_pEventManager;
}

CBot *CCore::CreateBot()
{
	TRACEFUNC("CCore::NewBot");

	CBot *pBot = new CBot(this);
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
	TRACEFUNC("CCore::NewScript");

	CScript *pScript = new CScript();
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

CPool<CScript *> *CCore::GetScripts()
{
	return &m_plScripts;
}

void CCore::Pulse()
{
	TRACEFUNC("CCore::Pulse");

	for (CPool<CBot *>::iterator i = m_plBots.begin(); i != m_plBots.end(); ++i)
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
		printf("Warning: No /bots/ directory\n");
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

				CBot *pBot = CreateBot();
				pBot->GetSettings()->LoadFromConfig(&Config);

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

				if (Config.StartValueList("channels"))
				{
					while (Config.GetNextValue(&strTemp))
					{
						pBot->JoinChannel(strTemp.c_str());
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
		printf("Warning: No /bots/ directory\n");
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

					CBot *pBot = CreateBot();
					pBot->GetSettings()->LoadFromConfig(&Config);

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
					
					if (Config.StartValueList("channels"))
					{
						while (Config.GetNextValue(&strTemp))
						{
							pBot->JoinChannel(strTemp.c_str());
						}
					}

				}
			}
		}
	}
	while (pEntry != NULL);
#endif
}

CEventManager *CCore::GetEventManager()
{
	return m_pEventManager;
}
