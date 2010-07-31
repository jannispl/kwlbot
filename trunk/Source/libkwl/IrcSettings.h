/*
kwlbot IRC bot


File:		IrcSettings.h
Purpose:	Class which manages IRC-related settings

*/

class CIrcSettings;

#ifndef _CIRCSETTINGS_H
#define _CIRCSETTINGS_H

#include <string>
#include "Config.h"

/**
 * Class which manages IRC-related settings.
 */
class DLLEXPORT CIrcSettings
{
public:
	/**
	 * Loads settings from a configuration.
	 * @param  pConfig  The configuration.
	 * @return True if the configuration was loaded successfully, false otherwise.
	 */
	bool LoadFromConfig(CConfig *pConfig);

	/**
	 * Changes the nickname.
	 * @param  szNickname  The new nickname.
	 * @return True if the nickname was changed, false otherwise.
	 */
	bool SetNickname(const char *szNickname);
	
	/**
	 * Changes the ident.
	 * @param  szIdent  The new ident.
	 * @return True if the ident was changed, false otherwise.
	 */
	bool SetIdent(const char *szIdent);
	
	/**
	 * Changes the realname.
	 * @param  szRealname  The new realname.
	 * @return True if the realname was changed, false otherwise.
	 */
	bool SetRealname(const char *szRealname);
	
	/**
	 * Changes the alternative nickname.
	 * @param  szNickname  The new alternative nickname.
	 * @return True if the alternative nickname was changed, false otherwise.
	 */
	bool SetAlternativeNickname(const char *szNickname);
	
	/**
	 * Changes the quit message.
	 * @param  szMessage  The new quit message.
	 * @return True if the quit message was changed, false otherwise.
	 */
	bool SetQuitMessage(const char *szMessage);

	/**
	 * Gets the nickname.
	 * @return The current nickname.
	 */
	const char *GetNickname();
	
	/**
	 * Gets the ident.
	 * @return The current ident.
	 */
	const char *GetIdent();
	
	/**
	 * Gets the realname.
	 * @return The current realname.
	 */
	const char *GetRealname();

	/**
	 * Gets the alternative nickname.
	 * @return The current alternative nickname.
	 */
	const char *GetAlternativeNickname();
	
	/**
	 * Gets the quit message.
	 * @return The current quit message.
	 */
	const char *GetQuitMessage();

private:
	std::string m_strNickname;
	std::string m_strIdent;
	std::string m_strRealname;
	std::string m_strAlternativeNickname;
	std::string m_strQuitMessage;
};

#endif
