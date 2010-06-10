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
#include "IrcSettings.h"

class DLLEXPORT CCore
{
public:
	CCore();
	~CCore();

	bool IsRunning();
	void Shutdown();

	CBot *CreateBot(CConfig *pConfig);
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
	bool m_bRunning;

	CEventManager *m_pScriptEventManager;

#ifdef WIN32
	template class DLLEXPORT CPool<CEventManager *>;
	template class DLLEXPORT CPool<CBot *>;
	template class DLLEXPORT CPool<CScript *>;
	template class DLLEXPORT CPool<CGlobalModule *>;
#endif

	CPool<CEventManager *> m_plEventManagers;
	CPool<CBot *> m_plBots;
	CPool<CScript *> m_plScripts;
	CPool<CGlobalModule *> m_plGlobalModules;
};

#endif
