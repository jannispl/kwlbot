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

class CCore
{
public:
	CCore();
	~CCore();

	CBot *CreateBot();
	bool DeleteBot(CBot *pBot);

	CScript *CreateScript(const char *szFilename);
	bool DeleteScript(CScript *pScript);

	CPool<CScript *> *GetScripts();

	void Pulse();
	void ScanDirectoryForBots(const char *szDirectory);

private:
	CPool<CBot *> m_plBots;
	CPool<CScript *> m_plScripts;
};

#endif
