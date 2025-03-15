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

//DWORD MakeIocp(SOCKET hDevice, PIOCP_ENV pi, int Th)
//{
//	DWORD dwThrId = 0;
//	pi->_iocp = CreateIoCompletionPort((HANDLE)hDevice, NULL, IOKEY_LISTEN, MAX_TH);
//
//	for (int i = 0; i < Th; i++)
//	{
//		pi->hThread[i] = CreateThread(NULL, 0, IocpCallBack, pi, 0, &dwThrId);
//		pi->ThreadCnt++;
//	}
//
//	return 0;
//}

//DWORD MakeSockPool(SOCKET hsoListen, DWORD Sc, SOCK_SET* ss)
//{
//	LPFN_ACCEPTEX pfnAcceptEx = (LPFN_ACCEPTEX)GetSockExtAPI(hsoListen, WSAID_ACCEPTEX);
//	DWORD SockCnt = 0;
//	for (; SockCnt < Sc; SockCnt++)
//	{
//		SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//		if (sock == INVALID_SOCKET)
//			break;
//		PSOCK_ITEM pi = new SOCK_ITEM(sock);
//		if (pfnAcceptEx(hsoListen, pi->_sock, pi->_buff, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL, (LPOVERLAPPED)pi) == FALSE)
//		{
//			if (WSAGetLastError() != WSA_IO_PENDING)
//			{
//				cout << "AcceptEx failed : " << WSAGetLastError() << endl;
//				closesocket(pi->_sock);
//				delete pi;
//				break;
//			}
//		}
//		ss->insert(pi);
//	}
//	return SockCnt;
//}

//DWORD WINAPI IocpCallBack(PVOID pParam)
//{
//	PIOCP_ENV pi = (PIOCP_ENV)pParam;
//	PSOCK_ITEM psi = NULL;
//	DWORD dwBytes = 0;
//	ULONG_PTR Status = 0;
//	while (true)
//	{
//		try
//		{
//			if (GetQueuedCompletionStatus(pi->_iocp, &dwBytes, &Status, (LPOVERLAPPED*)&psi, INFINITE) == FALSE)
//			{
//				if (psi != NULL)
//					throw (DWORD)psi->Internal;
//
//				int nErrCode = WSAGetLastError();
//				if (nErrCode != ERROR_ABANDONED_WAIT_0)
//					cout << "GQCS failed : " << nErrCode << endl;
//				break;
//			}
//
//			if (Status == IOKEY_LISTEN)
//			{
//				CreateIoCompletionPort((HANDLE)psi->_sock, pi->_iocp, IOKEY_CHILD, 0);
//				cout << "New Client In : " << psi->_sock << " ,connected..." << endl;
//				PostThreadMessage(pi->MainThrId, TM_SOCK_CONNECTED, 0, (LPARAM)psi);
//			}
//			else
//			{
//				if (!psi->NickCheck)
//				{
//					strncpy_s(psi->NickName, sizeof(psi->NickName), psi->_buff, sizeof(psi->NickName) - 1);
//					psi->NickCheck = TRUE;
//					cout << "Client " << psi->_sock << " NickName Set : " << psi->NickName << endl;
//
//					DWORD dwFlags = 0;
//					WSABUF wsb;
//					wsb.buf = psi->_buff, wsb.len = sizeof(psi->_buff);
//					if (WSARecv(psi->_sock, &wsb, 1, NULL, &dwFlags, psi, NULL) == SOCKET_ERROR)
//					{
//						int nErrCode = WSAGetLastError();
//						if (nErrCode != WSA_IO_PENDING)
//							throw nErrCode;
//					}
//					continue;
//				}
//				if (dwBytes == 0)
//					throw (INT)ERROR_SUCCESS;
//				psi->_buff[dwBytes < sizeof(psi->_buff) ? dwBytes : sizeof(psi->_sock) - 1] = 0;
//				char sendbuff[200];
//				sprintf_s(sendbuff, sizeof(sendbuff), "[%s] %s", psi->NickName, psi->_buff);
//				int len = (int)strlen(sendbuff);
//				for (SOCK_SET::iterator it = pi->conn->begin(); it != pi->conn->end(); it++)
//				{
//					PSOCK_ITEM target = *it;
//					if (target != psi)
//					{
//						if (send(target->_sock, sendbuff, len, 0) == SOCKET_ERROR)
//							throw WSAGetLastError();
//					}
//				}
//			}
//
//			DWORD dwFlags = 0;
//			WSABUF wsb;
//			wsb.buf = psi->_buff, wsb.len = sizeof(psi->_buff);
//			if (WSARecv(psi->_sock, &wsb, 1, NULL, &dwFlags, psi, NULL) == SOCKET_ERROR)
//			{
//				int nErrCode = WSAGetLastError();
//				if (nErrCode != WSA_IO_PENDING)
//					throw nErrCode;
//			}
//		}
//		catch (DWORD ex)
//		{
//			if (ex == STATUS_LOCAL_DISCONNECT || ex == STATUS_CANCELLED)
//			{
//				cout << "Child socket closed." << endl;
//				continue;
//			}
//			if (ex == ERROR_SUCCESS)
//				cout << "Client " << psi->_sock << " disconnected..." << endl;
//			else if (ex == STATUS_CONNECTION_RESET)
//				cout << "Pending Client " << psi->_sock << " disconnected..." << endl;
//			else
//				cout << "Client " << psi->_sock << " has error" << ex << endl;
//
//			PostThreadMessage(pi->MainThrId, TM_SOCK_DISCONNECT, ex, (LPARAM)psi);
//		}
//	}
//	return 0;
//}

bool SocketManger(UINT uCmd, PIOCP_ENV pie, PSOCK_ITEM psi, PTP_CALLBACK_INSTANCE pInst)
{
	bool bSuccess = true;
	LPFN_ACCEPTEX pfnAcceptEx = NULL;
	BOOL bIsOK = FALSE;
	try
	{
		if (uCmd == CMD_DELETE) //������ ���� �ʿ䰡 �ִ� ���
			throw (int)uCmd;

		if (uCmd == CMD_REUSE)
		{
			LPFN_DISCONNECTEX pfnDisconnectEx = (LPFN_DISCONNECTEX)GetSockExtAPI(psi->_sock, WSAID_DISCONNECTEX);
			if (!pfnDisconnectEx(psi->_sock, NULL, TF_REUSE_SOCKET, 0))
			{
				cout << "DisconnectEx failed : " << WSAGetLastError() << endl;
				bSuccess = false;
				throw (int)uCmd;
			}
		}

		if (uCmd == CMD_NEW)
		{
			psi->pio = CreateThreadpoolIo((HANDLE)psi->_sock, ChiledSockCallBack, pie, NULL);
			if (psi->pio == NULL)
				throw (int)uCmd;
		}

		StartThreadpoolIo(pie->pio);
		/*AcceptExȣ���� ���� StartThreadpoolIo�Լ��� ȣ���Ѵ�.���� : ���� ������ ���� �ڽ� ���Ͽ� TP_IO�� �ƴ� ���� ���Ͽ� TP_IO
		��ü�� ���� StartThreadpoolIo�� ȣ���Ѵٴ� ��*/
		pfnAcceptEx = (LPFN_ACCEPTEX)GetSockExtAPI(pie->hsoListen, WSAID_ACCEPTEX);
		bIsOK = pfnAcceptEx(pie->hsoListen, psi->_sock, psi->_buff, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL, (LPOVERLAPPED)psi);
		if (bIsOK == FALSE && WSAGetLastError() != WSA_IO_PENDING)
		{
			cout << "AcceptEx failed : " << WSAGetLastError() << endl;
			bSuccess = false;
			CancelThreadpoolIo(pie->pio);
			throw (int)uCmd;
		}
	}
	catch (int ex)
	{
		PTP_IO ptpIo = psi->pio;
		if (ex == CMD_REUSE)
		{
			DisassociateCurrentThreadFromCallback(pInst);
			//Ȥ�� ���� �� �ִ� �ݹ� �׸��� ������ ���Ÿ� ���� �� �������� ������ ���´�.
			WaitForThreadpoolIoCallbacks(ptpIo, TRUE);
			//��� ���� �ݹ� �׸��� �ִٸ� ����
		}
		if (ex == CMD_DELETE)
			SetEventWhenCallbackReturns(pInst, pie->hEvent);
		/*CMD_DELETE�� ���� ���ҽ� ������ ���� ���� �����忡�� ���� ���� ���� ��� ���� ������ ���� ����.���� �ݹ� �Լ� �Ϸ� ��⸦
		���� ������ �̺�Ʈ�� ���������,�̸� ������ Ǯ�� �Ͽ��� �ݹ� �Լ� ���� �Ϸ� �� �ñ׳� ���·� ���� ���� ��û�Ѵ�.*/
		else
			closesocket(psi->_sock);
		delete psi;
		CloseThreadpoolIo(ptpIo);
		//������ ���� ��� ���� TP_IO��ü�� �Բ� ����

		return bSuccess;
	}
	return bSuccess;
}

VOID CALLBACK MakeSockPool(PTP_CALLBACK_INSTANCE pci, PVOID pCtx, PTP_WAIT ptpWait, TP_WAIT_RESULT tResult)
{
	PIOCP_ENV pie = (PIOCP_ENV)pCtx;

	if (tResult == WAIT_TIMEOUT)	//Ÿ�Ӿƿ� �߻��� ����Ǯ�� �ּҸ� �����ϱ����� �׿� ��������
	{
		if (pie->pool.size() > SOCKPOOL_MIN)
		{
			EnterCriticalSection(&pie->cs);
			SOCK_SET::iterator it = pie->pool.begin();
			PSOCK_ITEM psi = *it;
			pie->pool.erase(it);
			LeaveCriticalSection(&pie->cs);

			closesocket(psi->_sock);
			WaitForSingleObject(pie->hEvent, INFINITE);
			cout << "TimeOut expired, pool = " << pie->pool.size() << " ,conn = " << pie->conn.size() << endl;
		}
	}
	else		//����Ǯ�� AcceptEx�� ������ �߰��ؾ� �Ѵٴ� ���� �ǹ� �� ����Ǯ ���� ���
	{
		if (tResult != WAIT_FAILED)	//WAIT_FAILED �� ����Ǯ ���� ������ ���� ó�� ȣ��Ǵ� ���
		{
			WSANETWORKEVENTS wse;
			WSAEnumNetworkEvents(pie->hsoListen, pie->wse, &wse);
			if (!(wse.lNetworkEvents & FD_ACCEPT))
				return;

			int nErrCode = wse.iErrorCode[FD_ACCEPT_BIT];
			if (nErrCode != 0)
			{
				cout << "Listen failed = " << nErrCode << endl;
				return;
			}
		}

		if (pie->pool.size() < SOCKPOOL_MAX)
		{
			for (int i = 0; i < SOCK_INC_CNT; i++)
			{
				SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				if (sock == INVALID_SOCKET)
					break;
				PSOCK_ITEM psi = new SOCK_ITEM(sock);
				if (!SocketManger(CMD_NEW, pie, psi))
					break;
				EnterCriticalSection(&pie->cs);
				pie->pool.insert(psi);
				LeaveCriticalSection(&pie->cs);
			}
		}
		cout << "Listen event signaled, pool = " << pie->pool.size() << " ,conn = " << pie->conn.size() << endl;
	}
	//Ÿ�Ӿƿ� ����
	LARGE_INTEGER ll; ll.QuadPart = -40000000LL;
	FILETIME ft; ft.dwHighDateTime = ll.HighPart;
	ft.dwLowDateTime = ll.LowPart;
	//WSAEventSelect�� ����� �̺�Ʈ�� ��� �ñ׳� ������ ���� �� �ֵ��� SetThreadpoolWaitȣ�� 4�ʵ��� ����ϴµ� ������ ������ �ݳ�
	SetThreadpoolWait(pie->pwi, pie->wse, &ft);
}

VOID CALLBACK ListenSockCallback(PTP_CALLBACK_INSTANCE pci, PVOID pCtx, PVOID pol, ULONG IoResult, ULONG_PTR dwTrBytes, PTP_IO pio)
{
	PIOCP_ENV pie = (PIOCP_ENV)pCtx;
	PSOCK_ITEM psi = (PSOCK_ITEM)pol;

	try
	{
		if (IoResult != NO_ERROR)
			throw IoResult;

		cout << "New Client " << psi->_sock << " connected..." << endl;
		EnterCriticalSection(&pie->cs);
		pie->pool.erase(psi);
		pie->conn.insert(psi);
		LeaveCriticalSection(&pie->cs);
		cout << "Client Connected, pool = " << pie->pool.size() << " ,conn = " << pie->conn.size() << endl;

		ChiledSockCallBack(pci, pie, psi, NO_ERROR, 0, psi->pio);
		//���� ���ӵ� �ڽ� �������� �Ͽ��� ���� WSARecv�� ȣ���ϵ��� ���� �ݹ��Լ� ȣ��

		StartThreadpoolIo(pio);
		//Ŭ���̾�Ʈ�κ��� ������ ������ �ݹ� �� �ݹ� �Լ��� ȣ��ǵ��� ���� ���Ͽ� TP_IO ��ü�� ���� StartThreadpoolIoȣ��
	}
	catch (ULONG ex)
	{
		if (ex != ERROR_OPERATION_ABORTED)
			cout << "Error Code : " << ex << endl;
		/*MakeSockPool �ݹ� �Լ����� Ÿ�Ӿƿ� ó�� �߿� ���� Ǯ�� ��� ���� ������ �ݾ��� ��쿡�� ERROR_OPERATION_ABORTED������ �߻��Ѵ�.*/
		EnterCriticalSection(&pie->cs);
		pie->conn.erase(psi);
		LeaveCriticalSection(&pie->cs);
		SocketManger(CMD_DELETE, pie, psi, pci);
	}
}

VOID CALLBACK ChiledSockCallBack(PTP_CALLBACK_INSTANCE pci, PVOID pCtx, PVOID pol, ULONG IoResult, ULONG_PTR dwTrBytes, PTP_IO pio)
{
	PIOCP_ENV pie = (PIOCP_ENV)pCtx;
	PSOCK_ITEM psi = (PSOCK_ITEM)pol;

	WSABUF wb; DWORD dwFlags = 0;
	try
	{
		if (IoResult != NO_ERROR)
			throw IoResult;

		if (dwTrBytes == 0)
		{
			if (!psi->NickCheck)  // ���� �г����� �������� ���� ��� => �ʱ� ������ ���ɼ��� ����.
			{
				// ��õ��� ���� WSARecv ��ȣ��
				StartThreadpoolIo(pio);
				wb.buf = psi->_buff;
				wb.len = sizeof(psi->_buff);
				if (WSARecv(psi->_sock, &wb, 1, NULL, &dwFlags, psi, NULL) == SOCKET_ERROR)
				{
					IoResult = WSAGetLastError();
					if (IoResult != WSA_IO_PENDING)
					{
						CancelThreadpoolIo(pio);
						throw IoResult;
					}
				}
				return;  // ���� �����Ͱ� �����Ƿ� ���⼭ ����
			}
			else
			{
				// �̹� �г����� �����Ǿ� �ִٸ� ���� ���� ����� �Ǵ�
				throw IoResult;
			}
		}
		else if (dwTrBytes > 0)
		{
			if (!psi->NickCheck)
			{
				psi->NickCheck = TRUE;
				strncpy_s(psi->NickName, sizeof(psi->NickName), psi->_buff, sizeof(psi->NickName) - 1);
				cout << "Client " << psi->_sock << " NickName : " << psi->NickName << endl;
			}
			else
			{
				psi->_buff[dwTrBytes < sizeof(psi->_buff) ? dwTrBytes : sizeof(psi->_sock) - 1] = 0;
				char SendBuff[600]{ 0, };
				sprintf_s(SendBuff, sizeof(SendBuff), "[%s] : %s", psi->NickName, psi->_buff);
				int len = strlen(SendBuff);
				SendBuff[len] = 0;
				for (SOCK_SET::iterator it = pie->conn.begin(); it != pie->conn.end(); it++)
				{
					PSOCK_ITEM target = *it;
					if (target != psi)
					{
						if (send(target->_sock, SendBuff, len, 0) == SOCKET_ERROR)
							throw WSAGetLastError();
					}
				}
			}
		}

		StartThreadpoolIo(pio);
		wb.buf = psi->_buff, wb.len = sizeof(psi->_buff);
		if (WSARecv(psi->_sock, &wb, 1, NULL, &dwFlags, psi, NULL) == SOCKET_ERROR)
		{
			IoResult = WSAGetLastError();
			if (IoResult != WSA_IO_PENDING)
			{
				CancelThreadpoolIo(pio);
				throw IoResult;
			}
		}
	}
	catch (ULONG ex)
	{
		if (ex == ERROR_OPERATION_ABORTED)
			return;
		/*�� �ݹ� �Լ����� ERROR_OPERATION_ABORTED ������ �߻��ϴ� ���� ���� �����忡�� ���α׷� ���Ḧ ���ؼ� CancelIoEx�� ȣ���� ����̹Ƿ�,
		���� �ٷ� �����Ѵ�.*/

		if (ex == ERROR_SUCCESS || ex == ERROR_NETNAME_DELETED)
			cout << "Client " << psi->_sock << " disconnected..." << endl;
		else
			cout << "Error occurred : " << ex << endl;

		EnterCriticalSection(&pie->cs);
		pie->conn.erase(psi);
		LeaveCriticalSection(&pie->cs);
		if (SocketManger(CMD_REUSE, pie, psi, pci))
		{
			EnterCriticalSection(&pie->cs);
			pie->pool.insert(psi);
			LeaveCriticalSection(&pie->cs);
		}
		cout << "Client Reused, pool = " << pie->pool.size() << " ,conn = " << pie->conn.size() << endl;
	}
}