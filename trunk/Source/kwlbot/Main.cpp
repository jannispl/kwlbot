/*
kwlbot IRC bot


File:		Main.cpp
Purpose:	Contains the entry point of the application

*/

#include "StdInc.h"
#include "../libkwl/Core.h"

CCore *g_pCore = NULL;

#ifndef WIN32
#define Sleep(ms) usleep(ms * 1000)
#include <signal.h>
#endif

#ifdef WIN32
BOOL WINAPI CtrlHandler(DWORD dwCtrlType)
{
	if (g_pCore == NULL)
	{
		return FALSE;
	}

	g_pCore->Shutdown();
	delete g_pCore;

	ExitProcess(0);
	while (true)
	{
		Sleep(20);
	}

	return TRUE;
}
#else
void CtrlHandler(int)
{
	if (g_pCore == NULL)
	{
		return;
	}

	g_pCore->Shutdown();
	delete g_pCore;

	puts("\n");
	_exit(0);
	while (true)
	{
		Sleep(20);
	}
}
#endif

int main(int iArgCount, char *szArgs[])
{
	g_pCore = new CCore();

#ifdef WIN32
	SetConsoleCtrlHandler(CtrlHandler, TRUE);
#else
	signal(SIGINT, CtrlHandler);
#endif

	while (true)
	{
		g_pCore->Pulse();

		Sleep(SLEEP_MS);
	}

	delete g_pCore;

	return 0;
}
