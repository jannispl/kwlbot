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

	CBot *CreateBot(const CIrcSettings &ircSettings);
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
#ifdef WIN32
	template class DLLEXPORT CPool<CEventManager *>;
#endif
	CPool<CEventManager *> m_plEventManagers;

#ifdef WIN32
	template class DLLEXPORT CPool<CBot *>;
#endif
	CPool<CBot *> m_plBots;
#ifdef WIN32
	template class DLLEXPORT CPool<CScript *>;
#endif
	CPool<CScript *> m_plScripts;
#ifdef WIN32
	template class DLLEXPORT CPool<CGlobalModule *>;
#endif
	CPool<CGlobalModule *> m_plGlobalModules;
};

#endif
