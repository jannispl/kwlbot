class CSendQueue;

#ifndef _SENDQUEUE_H
#define _SENDQUEUE_H

#include <queue>

#ifdef WIN32
  #include <winsock2.h>

  typedef SOCKET socket_t;
#else
  #include <sys/types.h>
  #include <sys/socket.h>

  typedef int socket_t;
#endif

class CSendQueue
{
public:
	CSendQueue();
	~CSendQueue();

	void Add(char *pPtr, size_t iSize, bool bRealloc = true, bool bNeedFree = false);
	void AddFile(FILE *pFile, size_t iBufferSize, bool bNeedClose = true);
	bool IsEmpty() const;
	bool Process(SOCKET socket);

private:
	typedef struct
	{
		char *pPtr;
		size_t iSize;
		unsigned char ucMode;

		char *pFileBuffer;
		size_t iLastBufferSize;
	} SendBuffer;

	std::queue<SendBuffer> m_bufferQueue;
};

#endif
