/*
kwlbot IRC bot


File:		File.h
Purpose:	Utility class which handles files

*/

class CFile;

#ifndef _FILE_H
#define _FILE_H

#include "ScriptObject.h"
#include <stdio.h>

class CFile : public CScriptObject
{
public:
	CFile(const char *szFilename, const char *szMode)
	{
		m_pFile = fopen(szFilename, szMode);
	}

	~CFile()
	{
		if (IsValid())
		{
			fclose(m_pFile);
		}
	}

	bool IsValid()
	{
		return m_pFile != NULL;
	}

	size_t Read(void *pDest, size_t iSize, size_t iCount)
	{
		return fread(pDest, iSize, iCount, m_pFile);
	}

	size_t Write(const void *pData, size_t iSize, size_t iCount)
	{
		size_t iRet = fwrite(pData, iSize, iCount, m_pFile);
		fflush(m_pFile);
		return iRet;
	}

	void Rewind()
	{
		rewind(m_pFile);
	}

	int Seek(long int iOffset, int iOrigin)
	{
		return fseek(m_pFile, iOffset, iOrigin);
	}

	long Tell()
	{
		return ftell(m_pFile);
	}

	/*int SetPos(const fpos_t *iPos)
	{
		return fsetpos(m_pFile, iPos);
	}

	int GetPos(fpos_t *iPos)
	{
		return fgetpos(m_pFile, iPos);
	}*/

	int Flush()
	{
		return fflush(m_pFile);
	}

	int Eof()
	{
		return feof(m_pFile);
	}

	CScriptObject::eScriptType GetType()
	{
		return File;
	}

private:
	FILE *m_pFile;
};

#endif
