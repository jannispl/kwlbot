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
#include "v8/v8.h"
#include <map>

/**
 * @brief Class which represents a remote IRC user.
 */
class DLLEXPORT CIrcUser : public CScriptObject
{
	friend class CBot;

public:
	CIrcUser(CBot *pParentBot, const std::string &strNickname, bool bTemporary = false);
	~CIrcUser();
	
	/**
	 * Gets the user's nickname.
	 * @return The user's nickname.
	 */
	const std::string &GetNickname();
	
	/**
	 * Gets the user's hostname.
	 * @return The user's hostname.
	 */
	const std::string &GetHostname();
	
	/**
	 * Gets the user's ident.
	 * @return The user's ident.
	 */
	const std::string &GetIdent();

	/**
	 * Gets the user's realname.
	 * @return The user's realname.
	 */
	const std::string &GetRealname();

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

#ifdef SERVICE
	/**
	 * Gets the user's modes.
	 * @return The user's modes.
	 */
	const std::string &GetUserModes();

	/**
	 * Gets the user's cloaked hostname.
	 * @return The user's cloaked hostname.
	 */
	const std::string &GetCloakedHost();

	/**
	 * Gets the user's virtual hostname (V-Host).
	 * @return The user's virtual hostname.
	 */
	const std::string &GetVirtualHost();
#endif

	/**
	 * Gets the parent bot.
	 * @return The parent bot.
	 */
	CBot *GetParentBot();

	void NewScript(CScript *pScript);
	void DeleteScript(CScript *pScript);
	v8::Object *GetScriptObject(CScript *pScript);

	/**
	 * Gets the script type of this class.
	 * @return The script type of this class.
	 */
	CScriptObject::eScriptType GetType();

private:
	void UpdateIfNecessary(const std::string &strIdent, const std::string &strHostname);

/*#ifdef WIN32
	template class DLLEXPORT CPool<CIrcChannel *>;
#endif*/

	CPool<CIrcChannel *> m_plIrcChannels;

	CBot *m_pParentBot;
	std::string m_strNickname;
	std::string m_strHostname;
	std::string m_strIdent;
	std::string m_strRealname;
	std::map<CIrcChannel *, char> m_mapChannelModes;
	bool m_bTemporary;

	std::map<CScript *, v8::Persistent<v8::Object> > m_mapScriptObjects;

#ifdef SERVICE
	std::string m_strUserModes;

	std::string m_strCloakedHost;
	std::string m_strVirtualHost;

#if IRCD == INSPIRCD
	char m_szUserId[9];
#endif

#endif
};

#endif
