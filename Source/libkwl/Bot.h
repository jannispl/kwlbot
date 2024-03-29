/*
kwlbot IRC bot


File:		Bot.h
Purpose:	Class which represents an IRC bot

*/

class CBot;

#ifndef _BOT_H
#define _BOT_H

#include "ScriptObject.h"
#include "Core.h"
#include "IrcSocket.h"
#include "Pool.h"
#include "IrcChannel.h"
#include "IrcUser.h"
#include "Script.h"
#include "EventManager.h"
#include "IrcMessage.h"
#include "v8/v8.h"
#include <vector>
#include <string>

/**
 * @brief Class which represents an IRC bot.
 */
class DLLEXPORT CBot : public CScriptObject
{
public:
	CBot(CCore *pParentCore, CConfig *pConfig);
	~CBot();

	enum eDeathReason
	{
		UserRequest,
		ConnectionError,
		Restart
	};

	/**
	 * Gets the parent core object.
	 * @return A pointer to a CCore.
	 */
	CCore *GetParentCore();
	
	/**
	 * Gets the IRC settings for this bot.
	 * @return A pointer to a CIrcSettings.
	 */
	CIrcSettings *GetSettings();

	/**
	 * Gets the configuration for this bot.
	 * @return A pointer to a CConfig.
	 */
	CConfig *GetConfig();
	
	/**
	 * Gets the IRC socket of this bot.
	 * @return A pointer to a CIrcSocket.
	 */
	CIrcSocket *GetSocket();

	/**
	 * Sends raw data to the server.
	 * @param strData The raw data to send.
	 * @return The amount of bytes transmitted, -1 incase of an error.
	 */
	int SendRaw(const std::string &strData);

	/**
	 * Sends raw data to the server (static).
	 * (Static: the data will be in the program's memory all time)
	 * @param szData The raw data to send.
	 * @see SendRaw()
	 * @return 0 if everything was transmitted, -1 if something had to be queued.
	 */
	int SendRawStatic(const char *szData);

	/**
	 * Sends raw data to the server (formatted).
	 * @param szData The formatted raw data to send.
	 * @param ... Argument list
	 * @see SendRaw()
	 * @return The amount of bytes transmitted, -1 incase of an error.
	 */
	int SendRawFormat(const char *szFormat, ...);

	/**
	 * Receives new data from the server socket, if available.
	 * @param pDest The destination string for the data.
	 * @param iSize The maximum amount of bytes to receive.
	 * @return The amount of bytes received, -1 incase of an error.
	 */
	int ReadRaw(char *pDest, size_t iSize);

	/**
	 * Pulses the bot instance.
	 */
	void Pulse();

	/**
	 * Searches the channel pool for a certain channel.
	 * @param  szName  The channel's name.
	 * @param  bCaseSensitive  Search case-sensitively or not (optional)
	 * @return A pointer to a CIrcChannel, or NULL incase the channel was not found.
	 */
	CIrcChannel *FindChannel(const char *szName, bool bCaseSensitive = false);
	
	/**
	 * Gets a pointer to the channel pool.
	 * @return A pointer to a CPool<CIrcChannel *>.
	 */
	CPool<CIrcChannel *> *GetChannels();

	/**
	 * Searches the user pool for a certain user.
	 * @param  szName          The user's nickname.
	 * @param  bCaseSensitive  Search case-sensitively or not (optional)
	 * @return A pointer to a CIrcUser, or NULL incase the user was not found.
	 */
	CIrcUser *FindUser(const char *szName, bool bCaseSensitive = true);

	/**
	 * Tries to convert a mode character to a prefix character.
	 * @param  cMode  The mode character.
	 * @return The mode character, or 0 if none was found.
	 */
	char ModeToPrefix(char cMode);

	/**
	 * Tries to convert a prefix character to a mode character.
	 * @param  cPrefix  The prefix character.
	 * @return The prefix character, or 0 if none was found.
	 */
	char PrefixToMode(char cPrefix);
	
	/**
	 * Checks if a certain mode character has a prefix character.
	 * @param  cMode  The mode character.
	 * @return True if the mode character has a prefix character, false otherwise.
	 */
	bool IsPrefixMode(char cMode);
	
	/**
	 * Gets the group of a mode character.
	 * @param  cMode  The mode character.
	 * @return A number ranging from 1 to 4 declaring the group, or 0 if none was found.
	 */
	char GetModeGroup(char cMode);
	
	/**
	 * Handles incoming raw IRC messages.
	 * @param  strOrigin   The message's origin.
	 * @param  strCommand  The command
	 * @param  vecParam    The message's parameters.
	 */
	void HandleData(const std::string &strOrigin, const std::string &strCommand, const std::vector<std::string> &vecParams);

	/**
	 * Tests if a certain user matches a certain access level's rules.
	 * @param  pUser   The user.
	 * @param  iLevel  The level.
	 * @return True if the user matches the given access level, false otherwise.
	 */
	bool TestAccessLevel(CIrcUser *pUser, int iLevel);
	
	/**
	 * Gets the number of available access levels.
	 * @return The number of available access levels.
	 */
	int GetNumAccessLevels();

	/**
	 * Creates a new script and adds it to the script pool.
	 * @param  szFilename  The script's filename.
	 * @return A pointer to a CScript.
	 */
	CScript *CreateScript(const char *szFilename);
	
	/**
	 * Deletes an existing script and removes it from the script pool.
	 * @param  pScript  The script to delete.
	 * @return True if the script was deleted, false otherwise.
	 */
	bool DeleteScript(CScript *pScript);
	
	/**
	 * Gets a pointer to the script pool.
	 * @return A pointer to a CPool<CScript *>.
	 */
	CPool<CScript *> *GetScripts();
	
	/**
	 * Makes the bot join an IRC channel.
	 * @param  szChannel  The channel's name.
	 */
	void JoinChannel(const char *szChannel);
	
	/**
	 * Makes the bot leave an IRC channel.
	 * @param  pChannel  The channel to leave.
	 * @param  szReason  The reason to leave for (optional)
	 * @return True if the channel was left, false otherwise.
	 */
	bool LeaveChannel(CIrcChannel *pChannel, const char *szReason = NULL);
	
	/**
	 * Sends an IRC message to the server.
	 * @param  ircMessage  The message.
	 */
	void SendMessage(const CIrcMessage &ircMessage);

	/**
	 * Gets a string of possible mode prefixes.
	 * @return A string of possible mode prefixes.
	 */
	const char *GetModePrefixes();

	/**
	 * Marks the bot as dead.
	 */
	void Die(eDeathReason deathReason = UserRequest);

	/**
	 * Tests whether the bot is dead.
	 * @return True if the bot is dead, false otherwise.
	 */
	bool IsDead();

	/**
	 * Gets the reason for the bot's death.
	 * @return The reason for the bot's death.
	 */
	eDeathReason GetDeathReason();

	/**
	 * Starts the reconnect timer.
	 */
	void StartReconnectTimer(unsigned int uiSeconds);

	/**
	 * Gets the reconnect timer.
	 */
	time_t GetReconnectTimer();

	/**
	 * Blocks/unblocks the send queue.
	 */
	void SetQueueBlocked(bool bBlocked);

	/**
	 * Tests whether the send queue is blocked.
	 * @return True if the send queue is blocked, false otherwise.
	 */
	bool IsQueueBlocked();

	/**
	 * Gets the script type of this class.
	 * @return The script type of this class.
	 */
	CScriptObject::eScriptType GetType();

private:
	void Handle352(const std::string &strNickname, const std::string &strChannel, const std::string &strIdent, const std::string &strHost, const std::string &strRealname);
	void Handle353(const std::string &strChannel, const std::string &strNames);
	void Handle333(const std::string &strChannel, const std::string &strTopicSetBy, time_t ullSetDate);

	void HandleKICK(const std::string &strChannel, const std::string &strVictim, const std::string &strReason = "");
	void Handle332(const std::string &strChannel, const std::string &strTopic);
	void Handle366(const std::string &strChannel);
	void HandlePART(const std::string &strChannel, const std::string &strReason = "");
	void HandlePRIVMSG(const std::string &strTarget, const std::string &strMessage);
	void HandleTOPIC(const std::string &strSetter, time_t ullSetDate, const std::string &strChannel, const std::string &strTopic);
	void HandleTOPIC(const std::string &strChannel, const std::string &strTopic);
	void HandleJOIN(const std::string &strChannel);
	void HandleQUIT(const std::string &strReason = "");
	void HandleNICK(const std::string &strNewNickname);
	void HandleMODE(const std::string &strChannel, const std::string &strModes, const std::vector<std::string> &vecParams);

#ifdef SERVICE
#if IRCD == UNREAL
	void HandleSERVER(const std::string &strHostname, int iHopCount, const std::string &strInformation);
	void HandleNICK(const std::string &strNickname, int iHopCount, time_t ullTimestamp, const std::string &strIdent, const std::string &strHostname, const std::string &strServer, time_t ullServiceStamp, const std::string &strUserModes, const std::string &strVirtualHost, const std::string &strCloakedHost, const std::string &strRealname);

	void HandleSVSMODE(const std::string &strTarget, const std::string &strModes, const std::vector<std::string> &vecParams);

	void HandleSETHOST(const std::string &strNewHost);
	void HandleSETIDENT(const std::string &strNewIdent);
	void HandleSETNAME(const std::string &strNewName);

	void HandleCHGHOST(const std::string &strTarget, const std::string &strNewHost);
	void HandleCHGIDENT(const std::string &strTarget, const std::string &strNewIdent);
	void HandleCHGNAME(const std::string &strTarget, const std::string &strNewName);
#elif IRCD == INSPIRCD
	CIrcUser *FindUserByUid(const std::string &strUid);

	void HandleSERVER(const std::string &strRemoteHost, const std::string &strPassword, int iHopCount, const std::string &strServerId, const std::string &strInformation);
	void HandleUID(const std::string &strUid, time_t ullTimestamp, const std::string &strNickname, const std::string &strHostname1, const std::string &strHostname2, const std::string &strIdent, const std::string &strIp, time_t ullSignonTime, const std::string &strUsermodes, const std::string &strRealname);
	void HandleFJOIN(const std::string &strChannel, time_t ullTimestamp, const std::string &strUsers);
	void HandleFTOPIC(const std::string &strChannel, time_t ullTimestamp, const std::string &strSetter, const std::string &strTopic);
	void HandleFHOST(const std::string &strNewHost);
#endif
#endif

	std::string m_strCurrentOrigin;
	CIrcUser *m_pCurrentUser;
	std::string m_strCurrentNickname;
	std::string m_strCurrentIdent;
	std::string m_strCurrentHostname;

	bool m_bDead;
	time_t m_tReconnectTimer;
	eDeathReason m_deathReason;

#ifdef SERVICE
	std::string m_strServerHost;
#endif

	bool m_bSynced;
	bool m_bGotScripts;

	CCore *m_pParentCore;
	CIrcSocket *m_pIrcSocket;
	CConfig m_botConfig;
	CIrcSettings m_ircSettings;

/*#ifdef WIN32
	template class DLLEXPORT CPool<CIrcChannel *>;
	template class DLLEXPORT CPool<CIrcUser *>;
	template class DLLEXPORT CPool<CScript *>;
#endif*/

	CPool<CIrcChannel *> m_plIrcChannels;
	CPool<CIrcChannel *> *m_pIrcChannelQueue;
	CPool<CIrcChannel *> m_plIncompleteChannels;

	CPool<CIrcUser *> m_plGlobalUsers;
	CPool<CScript *> m_plScripts;

	struct
	{
		std::string strChanmodes[4];
		std::string strPrefixes[2];
#ifndef SERVICE
		bool bNamesX;
#else
		bool bNickV2;
#endif
	} m_supportSettings;

	std::string m_strAutoMode;

	typedef struct
	{
		std::string strRequireHost;
		std::string strRequireNickname;
		std::string strRequireIdent;
		std::string strRequireRealname;
#ifdef SERVICE
		std::string strRequireVhost;
		std::string strRequireUmodes;
#endif
	} AccessRules;
	std::vector<AccessRules> m_vecAccessRules;

#if defined(SERVICE) && IRCD == INSPIRCD
	char m_szServerId[3];
#endif
};

#endif
