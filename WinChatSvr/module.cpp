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
		if (uCmd == CMD_DELETE) //소켓을 닫을 필요가 있는 경우
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
		/*AcceptEx호출을 위해 StartThreadpoolIo함수를 호출한다.주의 : 접속 수용을 위한 자식 소켓용 TP_IO가 아닌 리슨 소켓용 TP_IO
		객체에 대해 StartThreadpoolIo를 호출한다는 점*/
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
			//혹시 있을 수 있는 콜백 항목의 완전한 제거를 위해 본 스레드의 연결을 끊는다.
			WaitForThreadpoolIoCallbacks(ptpIo, TRUE);
			//대기 중인 콜백 항목이 있다면 제거
		}
		if (ex == CMD_DELETE)
			SetEventWhenCallbackReturns(pInst, pie->hEvent);
		/*CMD_DELETE는 소켓 리소스 해제를 위해 메인 스레드에서 직접 접속 수용 대기 중인 소켓을 닫은 경우다.따라서 콜백 함수 완료 대기를
		위해 별도의 이벤트를 사용했으며,이를 스레드 풀로 하여금 콜백 함수 실행 완료 후 시그널 상태로 만들 것을 요청한다.*/
		else
			closesocket(psi->_sock);
		delete psi;
		CloseThreadpoolIo(ptpIo);
		//소켓을 닫을 경우 관련 TP_IO객체도 함께 해제

		return bSuccess;
	}
	return bSuccess;
}

VOID CALLBACK MakeSockPool(PTP_CALLBACK_INSTANCE pci, PVOID pCtx, PTP_WAIT ptpWait, TP_WAIT_RESULT tResult)
{
	PIOCP_ENV pie = (PIOCP_ENV)pCtx;

	if (tResult == WAIT_TIMEOUT)	//타임아웃 발생시 소켓풀의 최소를 유지하기위해 잉여 소켓정리
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
	else		//소켓풀에 AcceptEx용 소켓을 추가해야 한다는 것을 의미 즉 소켓풀 증가 요망
	{
		if (tResult != WAIT_FAILED)	//WAIT_FAILED 는 소켓풀 최초 생성을 위해 처음 호출되는 경우
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
	//타임아웃 지정
	LARGE_INTEGER ll; ll.QuadPart = -40000000LL;
	FILETIME ft; ft.dwHighDateTime = ll.HighPart;
	ft.dwLowDateTime = ll.LowPart;
	//WSAEventSelect와 연결된 이벤트가 계속 시그널 통지를 받을 수 있도록 SetThreadpoolWait호출 4초동안 대기하는데 없으면 스레드 반납
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
		//새로 접속된 자식 소켓으로 하여금 최초 WSARecv를 호출하도록 직접 콜백함수 호출

		StartThreadpoolIo(pio);
		//클라이언트로부터 접속이 들어오면 콜백 이 콜백 함수가 호출되도록 리슨 소켓용 TP_IO 객체에 대해 StartThreadpoolIo호출
	}
	catch (ULONG ex)
	{
		if (ex != ERROR_OPERATION_ABORTED)
			cout << "Error Code : " << ex << endl;
		/*MakeSockPool 콜백 함수에서 타임아웃 처리 중에 소켓 풀에 대기 중인 소켓을 닫았을 경우에는 ERROR_OPERATION_ABORTED에러가 발생한다.*/
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
			if (!psi->NickCheck)  // 아직 닉네임이 설정되지 않은 경우 => 초기 수신일 가능성이 있음.
			{
				// 재시도를 위해 WSARecv 재호출
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
				return;  // 아직 데이터가 없으므로 여기서 종료
			}
			else
			{
				// 이미 닉네임이 설정되어 있다면 실제 연결 종료로 판단
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
		/*본 콜백 함수에서 ERROR_OPERATION_ABORTED 에러가 발생하는 경우는 메인 스레드에서 프로그램 종료를 위해서 CancelIoEx를 호출한 경우이므로,
		따라서 바로 리턴한다.*/

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