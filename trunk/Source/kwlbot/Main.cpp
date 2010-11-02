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

	while (true)
	{
		Sleep(SLEEP_MS);
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

	puts("\n");
	while (true)
	{
		Sleep(SLEEP_MS);
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
	signal(SIGKILL, CtrlHandler);

	switch (fork())
	{
	case -1:
		printf("Failed to fork process into background\n");
		return 1;
	case 0:
		break;
	default:
		printf("Forked into background.\n");
		return 0;
	}
#endif

	while (g_pCore->IsRunning())
	{
		g_pCore->Pulse();

		Sleep(SLEEP_MS);
	}

	delete g_pCore;

	return 0;
}
