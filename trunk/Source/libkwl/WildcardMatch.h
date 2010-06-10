/*
kwlbot IRC bot


File:		WildcardMatch.h
Purpose:	Contains utility function for matching strings against a pattern

*/

#ifndef _WILDCARDMATCH_H
#define _WILDCARDMATCH_H

static bool wildcmp(const char *wild, const char *string)
{
	char *cp = NULL, *mp = NULL;
	while (*string && *wild != '*')
	{
		if (*wild != *string && *wild != '?')
		{
			return false;
		}

		wild++;
		string++;
	}

	while (*string)
	{
		if (*wild == '*')
		{
			if (!*++wild)
			{
				return true;
			}

			mp = (char *)wild;
			cp = (char *)(string + 1);
		}
		else if (*wild == *string || *wild == '?')
		{
			wild++;
			string++;
		}
		else
		{
			wild = mp;
			string = cp++;
		}
	}

	while (*wild == '*')
	{
		wild++;
	}

	return !*wild;
}

#endif
