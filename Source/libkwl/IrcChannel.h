/*
kwlbot IRC bot


File:		IrcChannel.h
Purpose:	Class which represents a remote IRC channel

*/

class CIrcChannel;

#ifndef _IRCCHANNEL_H
#define _IRCCHANNEL_H

#include "ScriptObject.h"
#include "Bot.h"
#include "IrcUser.h"
#include "Pool.h"
#include "Script.h"
#include <time.h>
#include <map>

/**
 * @brief Class which represents a remote IRC channel.
 */
class DLLEXPORT CIrcChannel : public CScriptObject
{
	friend class CBot;

public:
	CIrcChannel(CBot *pParentBot, const std::string &strName);
	~CIrcChannel();

	/**
	 * Gets the channel's name.
	 * @return The channel's name.
	 */
	const std::string &GetName();

	/**
	 * Searches the user pool for a certain user.
	 * @param  szName          The user's nickname.
	 * @param  bCaseSensitive  Search case-sensitively or not (optional)
	 * @return A pointer to a CIrcUser, or NULL incase the user was not found.
	 */
	CIrcUser *FindUser(const char *szNickname, bool bCaseSensitive = true);
	
	/**
	 * Checks if a certain user exists in the user pool.
	 * @param  pUser  The user.
	 * @return True if the given user exists in the user pool, false otherwise.
	 */
	bool HasUser(CIrcUser *pUser);

	/**
	 * Gets a pointer to the user pool.
	 * @return A pointer to a CPool<CIrcUser *>.
	 */
	CPool<CIrcUser *> *GetUsers();

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

	typedef struct
	{
		time_t ullTopicSetDate;
		std::string strTopicSetBy;
		std::string strTopic;
	} TopicInfo;
	TopicInfo m_topicInfo;
	
private:
	CBot *m_pParentBot;
	std::string m_strName;

/*#ifdef WIN32
	template class DLLEXPORT CPool<CIrcUser *>;
#endif*/

	CPool<CIrcUser *> m_plIrcUsers;
	bool m_bHasDetailedUsers;

	std::map<CScript *, v8::Persistent<v8::Object> > m_mapScriptObjects;

#if defined(SERVICE) && IRCD == HYBRID
	time_t m_ullChannelStamp;
#endif
};

#endif
