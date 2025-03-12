#include <WinSock2.h>
#include <windows.h>
#include <iostream>
#include <tchar.h>
#include <set>
#include <Mswsock.h>
#include "module.h"
#pragma comment(lib,"Ws2_32.lib")

PVOID GetSockExtAPI(SOCKET sock, GUID guidFn)
{
	PVOID pfnEx = NULL;
	GUID guid = guidFn;
	DWORD dwBytes = 0;
	LONG lRet = WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &pfnEx, sizeof(pfnEx), &dwBytes, NULL, NULL);
	if (lRet == SOCKET_ERROR)
	{
		cout << "WSAIoctl failed, code : " << WSAGetLastError() << endl;
		return NULL;
	}
	return pfnEx;
}

SOCKET GetListen(short Port, int nBacklog = SOMAXCONN)
{
	SOCKET hsoListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (hsoListen == INVALID_SOCKET)
	{
		cout << "sock failed : " << WSAGetLastError();
		return INVALID_SOCKET;
	}

	SOCKADDR_IN sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_port = htons(Port);
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(hsoListen, (PSOCKADDR)&sa, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
	{
		cout << "bind failed : " << WSAGetLastError();
		closesocket(hsoListen);
		return INVALID_SOCKET;
	}

	if (listen(hsoListen, nBacklog) == SOCKET_ERROR)
	{
		cout << "listen failed : " << WSAGetLastError();
		closesocket(hsoListen);
		return INVALID_SOCKET;
	}
	return hsoListen;
}

PIOCP_ENV MakeIocp(int Th = 3, HANDLE hDevice)
{
	PIOCP_ENV pi;
	memset(&pi, 0, sizeof(pi));
	DWORD dwThrId = 0;
	pi->_iocp = CreateIoCompletionPort(hDevice, NULL, IOKEY_LISTEN, 2);

	for (int i = 0; i < Th; i++)
	{
		pi->hThread[i] = CreateThread(NULL, 0, IocpCallBack, &pi, 0, &dwThrId);
		pi->ThreadCnt++;
	}

	return pi;
}

DWORD MakeSockPool(SOCKET hsoListen, int Sc, SOCK_SET ss)
{
	LPFN_ACCEPTEX pfnAcceptEx = (LPFN_ACCEPTEX)GetSockExtAPI(hsoListen, WSAID_ACCEPTEX);
	DWORD SockCnt = 0;
	for (; SockCnt < SOCKPOOL_MAX; SockCnt++)
	{
		SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sock == INVALID_SOCKET)
			break;
		PSOCK_ITEM pi = new SOCK_ITEM(sock);
		if (pfnAcceptEx(hsoListen, sock, pi->_buff, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL, (LPOVERLAPPED)pi) == FALSE)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				cout << "AcceptEx failed : " << WSAGetLastError() << endl;
				closesocket(pi->_sock);
				delete pi;
				break;
			}
		}
		ss.insert(pi);
	}
	return SockCnt;
}

DWORD IocpCallBack(PVOID pParam)
{
	PIOCP_ENV pi = (PIOCP_ENV)pParam;
	PSOCK_ITEM psi = NULL;
	DWORD dwBytes = 0;
	ULONG_PTR Status = 0;
	while (true)
	{
		try
		{
			if (GetQueuedCompletionStatus(pi->_iocp, &dwBytes, &Status, (LPOVERLAPPED*)&psi, INFINITE) == FALSE)
			{
				if (psi != NULL)
					throw (int)psi->Internal;

				int nErrCode = WSAGetLastError();
				if (nErrCode == ERROR_ABANDONED_WAIT_0)
					cout << "GQCS failed : " << nErrCode << endl;
				break;
			}

			if (Status == IOKEY_LISTEN)
			{
				CreateIoCompletionPort((HANDLE)psi->_sock, pi->_iocp, IOKEY_CHILD, 0);
				cout << "New Client In : " << psi->_sock << "connected..." << endl;
				pi->pool.erase(psi);
				pi->conn.insert(psi);
			}
			else
			{
				if (dwBytes == 0)
					throw (INT)ERROR_SUCCESS;
				for (SOCK_SET::iterator it = pi->conn.begin(); it != pi->conn.end(); it++)
				{
					if (send((*it)->_sock, psi->_buff, dwBytes, 0) == SOCKET_ERROR)
						throw WSAGetLastError();
				}
			}

			DWORD dwFlags = 0;
			WSABUF wsb;
			wsb.buf = psi->_buff, wsb.len = sizeof(psi->_buff);
			if (WSARecv(psi->_sock, &wsb, 1, NULL, &dwFlags, psi, NULL) == SOCKET_ERROR)
			{
				int nErrCode = WSAGetLastError();
				if (nErrCode == WSA_IO_PENDING)
					throw nErrCode;
			}
		}
		catch (DWORD ex)
		{
			if (ex == STATUS_LOCAL_DISCONNECT || ex == STATUS_CANCELLED)
			{
				cout << " Child socket closed." << endl;
				continue;
			}
			if (ex == ERROR_SUCCESS)
				cout << "Client " << psi->_sock << " disconnected..." << endl;
			else if (ex == STATUS_CONNECTION_RESET)
				cout << " Pending Client " << psi->_sock << " disconnected..." << endl;
			else
				cout << " Client " << psi->_sock << " has error" << ex << endl;

			SetEvent(pi->ExtEvent);
		}
	}
	return 0;
}