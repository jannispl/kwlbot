/*
kwlbot IRC bot


File:		Bot_Hybrid.cpp
Purpose:	Class which represents an IRC bot
            IRCD-Hybrid service implementation

*/

#include "StdInc.h"
#include "Bot.h"
#include "WildcardMatch.h"
#include "IrcMessages.h"
#include <algorithm>

// Shortcut to call an event
#define CALL_EVENT_NOARG(what) \
	for (CPool<CEventManager *>::iterator eventManagerIterator = m_pParentCore->GetEventManagers()->begin(); eventManagerIterator != m_pParentCore->GetEventManagers()->end(); ++eventManagerIterator) \
	{ \
		(*eventManagerIterator)->what(this); \
	} \
	m_pParentCore->GetScriptEventManager()->what(this)

#define CALL_EVENT(what, ...) \
	for (CPool<CEventManager *>::iterator eventManagerIterator = m_pParentCore->GetEventManagers()->begin(); eventManagerIterator != m_pParentCore->GetEventManagers()->end(); ++eventManagerIterator) \
	{ \
		(*eventManagerIterator)->what(this, __VA_ARGS__); \
	} \
	m_pParentCore->GetScriptEventManager()->what(this, __VA_ARGS__)

#if defined(SERVICE) && IRCD == HYBRID

// TODO: IRCD-Hybrid implementation

/*
	ircd-hybrid protocol notes

	use SJOIN instead of JOIN: (DONE)
	  send_cmd(NULL, "SJOIN %ld %s + :%s", (long int) chantime, channel, user);
	  SJOIN 1284766570 #home + :LOLBOT

	nick introduction:
	  NICK MaVe 1 1284766547 +i ~MaVe xdsl-89-0-34-142.netcologne.de hades.arpa :...

	channel introduction:
	  :hades.arpa SJOIN 1284766548 #home +nt :@MaVe
*/

#endif
