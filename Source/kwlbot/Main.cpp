/*
kwlbot IRC bot


File:		Main.cpp
Purpose:	Contains the entry point of the application

*/

#include "StdInc.h"
#include "../libkwl/Core.h"

CCore *g_pCore = NULL;
#ifdef _DEBUG
CDebug *g_pDebug = NULL;
#endif

#ifndef WIN32
#define Sleep(ms) usleep(ms * 1000)
#endif

int main(int iArgCount, char *szArgs[])
{
#ifdef _DEBUG
	g_pDebug = new CDebug();
#endif
	TRACEFUNC("main");

	g_pCore = new CCore();

	while (true)
	{
		g_pCore->Pulse();

		Sleep(SLEEP_MS);
	}

	delete g_pCore;
#ifdef _DEBUG
	delete g_pDebug;
#endif

	return 0;
}
