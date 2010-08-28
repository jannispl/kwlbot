/*
kwlbot IRC bot


File:		TcpSocket.h
Purpose:	Utility class which handles sockets

*/

class CTcpSocket;

#ifndef _TCPSOCKET_H
#define _TCPSOCKET_H

#ifdef WIN32
  #include <winsock2.h>

  typedef SOCKET socket_t;
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <fcntl.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <cstring>
  typedef int socket_t;

  #define closesocket(sock) close(sock)
  #define SOCKET_ERROR (-1)
  #define INVALID_SOCKET (-1)
#endif

/**
 * @brief Utility class which handles sockets.
 */
class DLLEXPORT CTcpSocket
{
public:
	CTcpSocket()
		: m_Socket(INVALID_SOCKET)
	{
#ifdef WIN32
		WSADATA wsa;
		WSAStartup(MAKEWORD(2, 0), &wsa);
#endif

		m_Socket = socket(AF_INET, SOCK_STREAM, 0);
	}

	~CTcpSocket()
	{
		if (m_Socket != INVALID_SOCKET)
		{
			closesocket(m_Socket);
		}
	}

	void SetBlocking(bool bBlocking)
	{
#ifdef WIN32
		u_long ulFlags = bBlocking ? 0 : 1;
		ioctlsocket(m_Socket, FIONBIO, &ulFlags);
#else
		int iFlags;
//#if defined(O_NONBLOCK)
		if ((iFlags = fcntl(m_Socket, F_GETFL, 0)) == -1)
			iFlags = 0;
		fcntl(m_Socket, F_SETFL, iFlags | O_NONBLOCK);
/*#else
		iFlags = 1;
		return ioctl(m_Socket, FIOBIO, &iFlags);
#endif*/

#endif
	}

	bool Connect(const sockaddr *name, int namelen)
	{
		return connect(m_Socket, name, namelen) != SOCKET_ERROR;
	}

	static bool ResolveHostname(const char *szHostname, sockaddr_in *pDest = NULL)
	{
		unsigned int uiIP = inet_addr(szHostname);
		if (uiIP != INADDR_NONE)
		{
			if (pDest != NULL)
			{
				pDest->sin_addr.s_addr = uiIP;
			}
			return true;
		}

		hostent *pHostent = gethostbyname(szHostname);
		if (pHostent == NULL)
		{
			return false;
		}

		if (pDest != NULL)
		{
			memcpy(&(pDest->sin_addr), pHostent->h_addr_list[0], 4);
		}
		return true;
	}

	bool Connect(const char *szHostname, int iPort)
	{
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(iPort);

		if (!ResolveHostname(szHostname, &addr))
		{
			return false;
		}

		return Connect((sockaddr *)&addr, sizeof(sockaddr_in));
	}

	bool Bind(const sockaddr *addr, int namelen)
	{
		return bind(m_Socket, addr, namelen) != SOCKET_ERROR;
	}

	bool Listen(int backlog)
	{
		return listen(m_Socket, backlog) != SOCKET_ERROR;
	}

	size_t Write(const void *buf, size_t len = 0, int flags = 0)
	{
		if (len == 0)
		{
			len = strlen((const char *)buf);
		}

		return send(m_Socket, (const char *)buf, len, flags);
	}

	size_t Read(void *buf, size_t len, int flags = 0)
	{
		return recv(m_Socket, (char *)buf, len, flags);
	}

	bool Close()
	{
		if (closesocket(m_Socket) != SOCKET_ERROR)
		{
			m_Socket = INVALID_SOCKET;
			return true;
		}

		return false;
	}

	socket_t GetSocket()
	{
		return m_Socket;
	}

private:
	socket_t m_Socket;
};

#endif
