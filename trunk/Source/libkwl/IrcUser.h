/*
kwlbot IRC bot


File:		IrcUser.h
Purpose:	Class which represents a remote IRC user

*/

class CIrcUser;

#ifndef _IRCUSER_H
#define _IRCUSER_H

#include "ScriptObject.h"
#include "Bot.h"
#include "IrcChannel.h"
#include "Script.h"
#include <map>

/**
 * @brief Class which represents a remote IRC user.
 */
class DLLEXPORT CIrcUser : public CScriptObject
{
	friend class CBot;

public:
	CIrcUser(CBot *pParentBot, const char *szNickname, bool bTemporary = false);
	~CIrcUser();

	/**
	 * Updates the user's nickname.
	 * @internal
	 * @param  szNickname  The new nickname.
	 */
	void UpdateNickname(const char *szNickname);
	
	/**
	 * Gets the user's nickname.
	 * @return The user's nickname.
	 */
	const char *GetNickname();
	
	/**
	 * Gets the user's hostname.
	 * @return The user's hostname.
	 */
	const char *GetHostname();
	
	/**
	 * Gets the user's ident.
	 * @return The user's ident.
	 */
	const char *GetIdent();

	/**
	 * Checks if this is a temporary instance.
	 * @return True if this is a temporary instance, false otherwise.
	 */
	bool IsTemporary();

	/**
	 * Checks if a certain channel exists in the channel pool.
	 * @param  pChannel  The channel.
	 * @return True if the given channel exists in the user pool, false otherwise.
	 */
	bool HasChannel(CIrcChannel *pChannel);
	
	/**
	 * Gets the user's mode on a certain channel.
	 * @param  pChannel  The channel.
	 * @return The mode consisting of certain bits.
	 */
	char GetModeOnChannel(CIrcChannel *pChannel);

	/**
	 * Gets the parent bot.
	 * @return The parent bot.
	 */
	CBot *GetParentBot();

	/**
	 * Gets the script type of this class.
	 * @return The script type of this class.
	 */
	CScriptObject::eScriptType GetType();

private:
	/**
	 * Updates user information if necessary.
	 * @internal
	 * @param  szIdent     The new ident.
	 * @param  szHostname  The new hostname.
	 */
	void UpdateIfNecessary(const char *szIdent, const char *szHostname);

#ifdef WIN32
	template class DLLEXPORT CPool<CIrcChannel *>;
#endif
	CPool<CIrcChannel *> m_plIrcChannels;

	CBot *m_pParentBot;
	std::string m_strNickname;
	std::string m_strHostname;
	std::string m_strIdent;
	std::map<CIrcChannel *, char> m_mapChannelModes;
	bool m_bTemporary;
};

#endif