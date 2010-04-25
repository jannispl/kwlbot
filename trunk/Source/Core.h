/*
kwlbot IRC bot


File:		Core.h
Purpose:	Core container which manages all bot instances

*/

class CCore;

#ifndef _CORE_H
#define _CORE_H

#include "Bot.h"
#include "Pool.h"

class CCore
{
public:
	CCore();
	~CCore();

	CBot *NewBot();
	bool DeleteBot(CBot *pBot);
	void Pulse();
	void ScanDirectoryForBots(const char *szDirectory);

private:
	CPool<CBot *> m_plBots;
};

#endif
