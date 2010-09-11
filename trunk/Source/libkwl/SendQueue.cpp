#include "StdInc.h"
#include "SendQueue.h"

CSendQueue::CSendQueue()
{
}

CSendQueue::~CSendQueue()
{
}

// bRealloc: will copy memory from pPtr to some newly allocated memory
// bNeedFree: free the memory when it's no longer needed (sent)
//            (automatically true when bRealloc)
void CSendQueue::Add(char *pPtr, size_t iSize, bool bRealloc, bool bNeedFree)
{
	SendBuffer buf;

	buf.pFileBuffer = NULL;
	buf.iSize = iSize;

	if (bRealloc)
	{
		buf.ucMode = 1;
		char *pMemory = reinterpret_cast<char *>(malloc(iSize));
		memcpy(pMemory, pPtr, iSize);
		buf.pPtr = pMemory;
	}
	else
	{
		buf.pPtr = pPtr;
		buf.ucMode = bNeedFree ? 1 : 0;
	}

	m_bufferQueue.push(buf);
}

void CSendQueue::AddFile(FILE *pFile, size_t iBufferSize, bool bNeedClose)
{
	SendBuffer buf;

	buf.iLastBufferSize = 0;
	buf.pPtr = reinterpret_cast<char *>(pFile);
	buf.iSize = iBufferSize;
	buf.pFileBuffer = reinterpret_cast<char *>(malloc(iBufferSize));
	buf.ucMode = bNeedClose ? 2 : 3;

	m_bufferQueue.push(buf);
}

bool CSendQueue::IsEmpty() const
{
	return m_bufferQueue.empty();
}

// return true: everything handled well
// return false: something not done yet
bool CSendQueue::Process(socket_t socket)
{
	while (!IsEmpty())
	{
		SendBuffer &buf = m_bufferQueue.front();
		if (buf.pFileBuffer != NULL)
		{
			if (!feof((FILE *)buf.pPtr) || buf.iLastBufferSize != 0)
			{
				if (buf.iLastBufferSize != 0)
				{
					if (send(socket, buf.pFileBuffer, buf.iLastBufferSize, 0) == -1)
					{
						return false;
					}
					else
					{
						buf.iLastBufferSize = 0;
					}
				}
				else
				{
					size_t iRead = fread(buf.pFileBuffer, 1, buf.iSize, (FILE *)buf.pPtr);
					if (iRead != 0)
					{
						if (send(socket, buf.pFileBuffer, iRead, 0) == -1)
						{
							buf.iLastBufferSize = iRead;
							return false;
						}
					}
					else
					{
						if (buf.ucMode == 2)
						{
							fclose((FILE *)buf.pPtr);
						}
						free(buf.pFileBuffer);
						m_bufferQueue.pop();
					}
				}
			}
			else
			{
				if (buf.ucMode == 2)
				{
					fclose((FILE *)buf.pPtr);
				}
				free(buf.pFileBuffer);
				m_bufferQueue.pop();
			}
		}
		else
		{
			if (send(socket, buf.pPtr, buf.iSize, 0) == -1)
			{
				return false;
			}

			if (buf.ucMode == 1)
			{
				free(buf.pPtr);
			}

			m_bufferQueue.pop();
		}
	}
	return true;
}
