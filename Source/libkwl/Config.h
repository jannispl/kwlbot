/*
kwlbot IRC bot


File:		Config.h
Purpose:	Config parser

*/

class CConfig;

#ifndef _CONFIG_H
#define _CONFIG_H

#include <fstream>
#include <string>
#include <map>

class CConfig
{
public:
	CConfig(std::string strFilename, std::string strSeparator = "=")
		: m_iValueIndex(0)
	{
		m_bLoaded = false;

		std::ifstream ConfigFile;
		ConfigFile.open(strFilename.c_str(), std::ios::in);
		if (ConfigFile.fail())
			return;

		char buf[321];
		std::string strLine;
		while (!ConfigFile.eof())
		{
			ConfigFile.getline(buf, 320);
			if (buf[0] != '#')
			{
				strLine = buf;
				if (strLine.length() > 0)
				{
					std::string::size_type eqpos = strLine.find(strSeparator);
					if (eqpos != std::string::npos)
					{
						std::string before = strLine.substr(0, eqpos);
						std::string::size_type beforelen = before.length();
						if (beforelen != 0)
						{
							std::string after = strLine.substr(eqpos + strSeparator.length());
							std::string::size_type afterlen = after.length();
							if (afterlen != 0)
							{
								std::string::size_type off_begin = 0, off_end = 0;
								while (after[off_begin] == ' ')
									++off_begin;
								after = after.substr(off_begin);

								while (before[beforelen - off_end - 1] == ' ')
									++off_end;
								before[beforelen - off_end] = '\0';

								if (after[afterlen - 1] == '\n')
									if (after[afterlen - 2] == '\r')
										after[afterlen - 2] = '\0';
									else
										after[afterlen - 1] = '\0';
								else if (after[afterlen - 1] == '\r')
									after[afterlen - 1] = '\0';

								m_mapSettings[before.c_str()] = after;
							}
						}
					}
				}
			}
		}

		ConfigFile.close();

		m_bLoaded = true;
	}

	~CConfig()
	{
	}

	bool StartValueList(std::string strKey)
	{
		if (!m_bLoaded)
			return false;

		m_strCurrentValue = m_mapSettings[strKey];
		if (m_strCurrentValue.empty())
			return false;

		m_iValueIndex = 0;

		return true;
	}

	bool GetNextValue(std::string *pDest)
	{
		if (!m_bLoaded || m_iValueIndex == -1)
			return false;

		std::string strValue = m_strCurrentValue.substr(m_iValueIndex);
		std::string::size_type separator = strValue.find(',');

		std::string::size_type off = 0;
		while (*(strValue.begin() + off) == ' ')
			++off;

		if (separator == std::string::npos)
		{
			m_iValueIndex = -1;
			strValue = strValue.substr(off);
		}
		else
		{		
			strValue = strValue.substr(off, separator - off);
			m_iValueIndex += strValue.length() + 1 + off;
		}

		if (pDest != NULL)
			*pDest = strValue;

		return true;
	}

	bool GetSingleValue(std::string strKey, std::string *pDest)
	{
		if (!m_bLoaded)
			return false;

		std::string strValue = m_mapSettings[strKey];
		if (strValue.empty())
			return false;

		if (pDest != NULL)
			*pDest = strValue;

		return true;
	}

private:
	bool m_bLoaded;

	std::map<std::string, std::string> m_mapSettings;
	std::string m_strCurrentValue;
	std::string::size_type m_iValueIndex;
};

#endif
