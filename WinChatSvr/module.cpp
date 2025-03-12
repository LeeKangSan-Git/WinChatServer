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

SOCKET GetListen(short Port, int nBacklog)
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

DWORD MakeIocp(SOCKET hDevice, PIOCP_ENV pi, int Th)
{
	DWORD dwThrId = 0;
	pi->_iocp = CreateIoCompletionPort((HANDLE)hDevice, NULL, IOKEY_LISTEN, MAX_TH);

	for (int i = 0; i < Th; i++)
	{
		pi->hThread[i] = CreateThread(NULL, 0, IocpCallBack, pi, 0, &dwThrId);
		pi->ThreadCnt++;
	}

	return 0;
}

DWORD MakeSockPool(SOCKET hsoListen, DWORD Sc, SOCK_SET* ss)
{
	LPFN_ACCEPTEX pfnAcceptEx = (LPFN_ACCEPTEX)GetSockExtAPI(hsoListen, WSAID_ACCEPTEX);
	DWORD SockCnt = 0;
	for (; SockCnt < Sc; SockCnt++)
	{
		SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (sock == INVALID_SOCKET)
			break;
		PSOCK_ITEM pi = new SOCK_ITEM(sock);
		if (pfnAcceptEx(hsoListen, pi->_sock, pi->_buff, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL, (LPOVERLAPPED)pi) == FALSE)
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				cout << "AcceptEx failed : " << WSAGetLastError() << endl;
				closesocket(pi->_sock);
				delete pi;
				break;
			}
		}
		ss->insert(pi);
	}
	return SockCnt;
}

DWORD WINAPI IocpCallBack(PVOID pParam)
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
					throw (DWORD)psi->Internal;

				int nErrCode = WSAGetLastError();
				if (nErrCode != ERROR_ABANDONED_WAIT_0)
					cout << "GQCS failed : " << nErrCode << endl;
				break;
			}

			if (Status == IOKEY_LISTEN)
			{
				CreateIoCompletionPort((HANDLE)psi->_sock, pi->_iocp, IOKEY_CHILD, 0);
				cout << "New Client In : " << psi->_sock << " ,connected..." << endl;
				PostThreadMessage(pi->MainThrId, TM_SOCK_CONNECTED, 0, (LPARAM)psi);
			}
			else
			{
				if (dwBytes == 0)
					throw (INT)ERROR_SUCCESS;
				for (SOCK_SET::iterator it = pi->conn->begin(); it != pi->conn->end(); it++)
				{
					PSOCK_ITEM target = *it;
					if (target != psi)
					{
						if (send(target->_sock, psi->_buff, dwBytes, 0) == SOCKET_ERROR)
							throw WSAGetLastError();
					}
				}
			}

			DWORD dwFlags = 0;
			WSABUF wsb;
			wsb.buf = psi->_buff, wsb.len = sizeof(psi->_buff);
			if (WSARecv(psi->_sock, &wsb, 1, NULL, &dwFlags, psi, NULL) == SOCKET_ERROR)
			{
				int nErrCode = WSAGetLastError();
				if (nErrCode != WSA_IO_PENDING)
					throw nErrCode;
			}
		}
		catch (DWORD ex)
		{
			if (ex == STATUS_LOCAL_DISCONNECT || ex == STATUS_CANCELLED)
			{
				cout << "Child socket closed." << endl;
				continue;
			}
			if (ex == ERROR_SUCCESS)
				cout << "Client " << psi->_sock << " disconnected..." << endl;
			else if (ex == STATUS_CONNECTION_RESET)
				cout << "Pending Client " << psi->_sock << " disconnected..." << endl;
			else
				cout << "Client " << psi->_sock << " has error" << ex << endl;

			PostThreadMessage(pi->MainThrId, TM_SOCK_DISCONNECT, ex, (LPARAM)psi);
		}
	}
	return 0;
}