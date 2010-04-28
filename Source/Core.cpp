/*
kwlbot IRC bot


File:		Core.cpp
Purpose:	Core container which manages all bot instances

*/

#include "StdInc.h"
#include "Core.h"
#include "Config.h"

CCore::CCore()
{
	m_pEventManager = new CEventManager(this);

	CConfig cfg("core.cfg");
	if (cfg.StartValueList("scripts"))
	{
		std::string strValue;
		while (cfg.GetNextValue(&strValue))
		{
			CScript *pScript = CreateScript(("scripts/" + strValue).c_str());
			if (pScript != NULL)
			{
				printf("loaded %s\n", strValue.c_str());
			}
		}
	}

	ScanDirectoryForBots("bots/");
}

CCore::~CCore()
{
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

	WIN32_FIND_DATAA fd;

	char *szPath = new char[strlen(szDirectory) + 2];
	sprintf(szPath, "%s*", szDirectory);
	HANDLE hDir = FindFirstFileA(szPath, &fd);
	delete[] szPath;
	if (hDir != INVALID_HANDLE_VALUE)
	{
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

					printf("bot '%s'\n", fd.cFileName);

					CBot *pBot = CreateBot();
					pBot->GetSettings()->LoadFromConfig(&Config);

					std::string strTemp;
					if (Config.GetSingleValue("server", &strTemp))
					{
						pBot->GetSocket()->Connect(strTemp.c_str());
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
	}
}

CEventManager *CCore::GetEventManager()
{
	return m_pEventManager;
}