/*
kwlbot IRC bot


File:		TimerManager.h
Purpose:	Manages script timers

*/

class CTimerManager;

#ifndef _TIMERMANAGER_H
#define _TIMERMANAGER_H

#include "Pool.h"
#include "Script.h"
#include "v8/v8.h"

/**
 * @brief Manages script timers.
 */
class DLLEXPORT CTimerManager
{
public:
	CTimerManager(CScript *pParentScript);
	~CTimerManager();

	typedef struct
	{
		v8::Persistent<v8::Function> timerFunction;
		unsigned long long ullNextCall;
		unsigned int uiRemainingCalls;
		unsigned long ulInterval;

		int iNumArguments;
		v8::Persistent<v8::Value> *pArgValues;
	} Timer;

	Timer *SetTimer(v8::Local<v8::Function> timerFunction, unsigned long ulInterval, unsigned int uiNumCalls, int iNumArguments = 0, v8::Persistent<v8::Value> *pArgValues = NULL);
	bool KillTimer(Timer *pTimer);

	void Pulse();

private:
/*#ifdef WIN32
	template class DLLEXPORT CPool<Timer *>;
#endif*/

	CScript *m_pParentScript;
	CPool<Timer *> m_plTimers;
};

#endif
