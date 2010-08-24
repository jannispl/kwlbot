/*
kwlbot IRC bot


File:		TimerManager.cpp
Purpose:	Manages script timers

*/

#include "StdInc.h"
#include "TimerManager.h"

#ifndef WIN32
#include <sys/time.h>
unsigned long long GetTickCount64()
{
	struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}
#endif

CTimerManager::CTimerManager(CScript *pParentScript)
	: m_pParentScript(pParentScript)
{
}

CTimerManager::~CTimerManager()
{
}

CTimerManager::Timer *CTimerManager::SetTimer(v8::Local<v8::Function> timerFunction, unsigned long ulInterval, unsigned int uiNumCalls, int iNumArguments, v8::Persistent<v8::Value> *pArgValues)
{
	Timer *pTimer = new Timer;
	pTimer->timerFunction = v8::Persistent<v8::Function>::New(timerFunction);
	pTimer->uiRemainingCalls = uiNumCalls;
	pTimer->ulInterval = ulInterval;
	pTimer->ullNextCall = GetTickCount64() + ulInterval;

	pTimer->iNumArguments = iNumArguments;
	if (iNumArguments > 0)
	{
		pTimer->pArgValues = pArgValues;
	}

	m_plTimers.push_back(pTimer);

	return pTimer;
}

bool CTimerManager::KillTimer(CTimerManager::Timer *pTimer)
{
	for (CPool<Timer *>::iterator i = m_plTimers.begin(); i != m_plTimers.end(); ++i)
	{
		if (*i == pTimer)
		{
			if (pTimer->iNumArguments > 0)
			{
				for (int i = 0; i < pTimer->iNumArguments; ++i)
				{
					pTimer->pArgValues[i].Dispose();
				}
				delete[] pTimer->pArgValues;
			}

			pTimer->timerFunction.Dispose();

			delete pTimer;
			m_plTimers.erase(i);

			return true;
		}
	}

	return false;
}

void CTimerManager::Pulse()
{
	v8::HandleScope handleScope;

	for (CPool<Timer *>::iterator i = m_plTimers.begin(); i != m_plTimers.end(); ++i)
	{
		Timer *pTimer = *i;
		if (GetTickCount64() >= pTimer->ullNextCall)
		{
			pTimer->ullNextCall = GetTickCount64() + pTimer->ulInterval;

			CScriptFunctions::m_pCallingScript = m_pParentScript;
			m_pParentScript->EnterContext();
			pTimer->timerFunction->Call(pTimer->timerFunction, pTimer->iNumArguments, pTimer->pArgValues);
			m_pParentScript->ExitContext();
			CScriptFunctions::m_pCallingScript = NULL;

			if (pTimer->uiRemainingCalls > 0)
			{
				if (--pTimer->uiRemainingCalls == 0)
				{
					if (pTimer->iNumArguments > 0)
					{
						for (int i = 0; i < pTimer->iNumArguments; ++i)
						{
							pTimer->pArgValues[i].Dispose();
						}
						delete[] pTimer->pArgValues;
					}

					pTimer->timerFunction.Dispose();

					delete pTimer;
					i = m_plTimers.erase(i);
					if (i == m_plTimers.end())
					{
						break;
					}
				}
			}
		}
	}
}
