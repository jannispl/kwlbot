/*
kwlbot IRC bot


File:		Core.h
Purpose:	Core container which manages all bot instances

*/

class CCore;

#ifndef _CORE_H
#define _CORE_H

#include "Bot.h"
#include "Script.h"
#include "Pool.h"
#include "EventManager.h"
#include "GlobalModule.h"

class DLLEXPORT CCore
{
public:
	CCore();
	~CCore();

	CBot *CreateBot();
	bool DeleteBot(CBot *pBot);

	CScript *CreateScript(const char *szFilename);
	bool DeleteScript(CScript *pScript);

	CGlobalModule *CreateGlobalModule(const char *szPath);
	bool DeleteGlobalModule(CGlobalModule *pGlobalModule);

	CPool<CBot *> *GetBots();
	CPool<CScript *> *GetScripts();
	CPool<CGlobalModule *> *GetGlobalModules();

	void Pulse();
	void ScanDirectoryForBots(const char *szDirectory);

	CEventManager *GetScriptEventManager();
	CPool<CEventManager *> *GetEventManagers();

	void AddEventManager(CEventManager *pEventManager);

private:
	CEventManager *m_pScriptEventManager;
	CPool<CEventManager *> m_plEventManagers;

	CPool<CBot *> m_plBots;
	CPool<CScript *> m_plScripts;
	CPool<CGlobalModule *> m_plGlobalModules;
};

#endif
