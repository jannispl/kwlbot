/*
kwlbot IRC bot


File:		CTcpSocket.h
Purpose:	Utility class which handles sockets

*/

class CTcpSocket;

#ifndef _CTCPSOCKET_H
#define _CTCPSOCKET_H

#ifdef WIN32
  #include <winsock2.h>

  typedef SOCKET socket_t;
#else
  typedef int socket_t;

  #define closesocket(sock) close(sock)
#endif

class CTcpSocket
{
public:
	CTcpSocket()
	{
		WSADATA wsa;
		WSAStartup(MAKEWORD(2, 0), &wsa);

		m_Socket = socket(AF_INET, SOCK_STREAM, 0);
	}

	~CTcpSocket()
	{
		if (m_Socket)
			closesocket(m_Socket);
	}

	void SetBlocking(bool bBlocking)
	{
#ifdef WIN32
		u_long ulFlags = bBlocking ? 0 : 1;
		ioctlsocket(m_Socket, FIONBIO, &ulFlags);
#else

#endif
	}

	int Connect(const sockaddr *name, int namelen)
	{
		return connect(m_Socket, name, namelen);
	}

	static bool ResolveHostname(const char *szHostname, sockaddr_in *dest)
	{
		if (dest == NULL)
		{
			return false;
		}

		unsigned int uiIP = inet_addr(szHostname);
		if (uiIP != INADDR_NONE)
		{
			dest->sin_addr.S_un.S_addr = uiIP;
			return true;
		}

		hostent *pHostent = gethostbyname(szHostname);
		if (pHostent == NULL)
		{
			return false;
		}
		memcpy(&(dest->sin_addr), pHostent->h_addr_list[0], 4);
		return true;
	}

	int Connect(const char *szHostname, int iPort)
	{
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(iPort);
		ResolveHostname(szHostname, &addr);

		return Connect((sockaddr *)&addr, sizeof(sockaddr_in));
	}

	int Bind(const sockaddr *addr, int namelen)
	{
		return bind(m_Socket, addr, namelen);
	}

	int Listen(int backlog)
	{
		return listen(m_Socket, backlog);
	}

	int Write(const void *buf, size_t len = 0, int flags = 0)
	{
		if (len == 0)
			len = strlen((const char *)buf);

		return send(m_Socket, (const char *)buf, len, flags);
	}

	int Read(void *buf, size_t len, int flags = 0)
	{
		return recv(m_Socket, (char *)buf, len, flags);
	}

	int Close()
	{
		int ret = closesocket(m_Socket);
		m_Socket = 0;
		return ret;
	}

private:
	socket_t m_Socket;
};

#endif

