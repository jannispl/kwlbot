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

/**
 * @brief Core container which manages all bot instances.
 */
class DLLEXPORT CCore
{
public:
	CCore();
	~CCore();

	/**
	 * Checks if the core is still running.
	 * @return True if the core is still running, false otherwise.
	 */
	bool IsRunning();
	
	/**
	 * Makes the core shutdown.
	 */
	void Shutdown();

	/**
	 * Creates a new bot and adds it to the bot pool.
	 * @param  pConfig  A pointer to a bot configuration.
	 * @return A pointer to a CBot.
	 */
	CBot *CreateBot(CConfig *pConfig);

	/**
	 * Deletes an existing bot and removes it from the bot pool.
	 * @param  pBot  The bot to delete.
	 * @return True if the bot was deleted, false otherwise.
	 */
	bool DeleteBot(CBot *pBot);

	/**
	 * Creates a new global module and adds it to the global module pool.
	 * @param  szPath  The path to the global module.
	 * @return A pointer to a CGlobalModule.
	 */
	CGlobalModule *CreateGlobalModule(const char *szPath);

	/**
	 * Deletes an existing bot and removes it from the bot pool.
	 * @param  pBot  The bot to delete.
	 * @return True if the bot was deleted, false otherwise.
	 */
	bool DeleteGlobalModule(CGlobalModule *pGlobalModule);

	/**
	 * Gets a pointer to the bot pool.
	 * @return A pointer to a CPool<CBot *>.
	 */
	CPool<CBot *> *GetBots();
	
	/**
	 * Gets a pointer to the global module pool.
	 * @return A pointer to a CPool<CGlobalModule *>.
	 */
	CPool<CGlobalModule *> *GetGlobalModules();

	/**
	 * Pulses the core instance.
	 */
	void Pulse();
	
	/**
	 * Scans a directory for bots and adds them.
	 * @param  szDirectory  The directory's path to scan for bots.
	 */
	void ScanDirectoryForBots(const char *szDirectory);

	/**
	 * Gets a pointer to the script event manager.
	 * @return A pointer to a CEventManager
	 */
	CEventManager *GetScriptEventManager();
	
	/**
	 * Gets a pointer to the event manager pool.
	 * @return A pointer to a CPool<CEventManager *>.
	 */
	CPool<CEventManager *> *GetEventManagers();

	/**
	 * Adds an event manager to the event manager pool.
	 * @param  pEventManager  The event manager to add.
	 */
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
