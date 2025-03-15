#include <WinSock2.h>
#include <windows.h>
#include <iostream>
#include <tchar.h>
#include <MSWSock.h>
#include "module.h"
#include "main.h"
using namespace std;

//BOOL CtrlHandler(DWORD fdwCtrlType)
//{
//	PostThreadMessage(mainThrId, TM_PROG_EXIT, 0, 0);
//	return TRUE;
//}

//int _tmain()
//{
//	WSADATA wsd;
//	if (WSAStartup(MAKEWORD(2, 2), &wsd))
//	{
//		cout << "WSAStartup failed" << endl;
//		return 1;
//	}
//
//	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE))
//	{
//		cout << "CtrlHandler failed : " << GetLastError() << endl;
//		return 1;
//	}
//
//	SOCKET hsoListen = GetListen(9001);
//	if (hsoListen == INVALID_SOCKET)
//	{
//		cout << "Make ListenSock failed : " << WSAGetLastError() << endl;
//		WSACleanup();
//		return 1;
//	}
//
//	IOCP_ENV ie;
//	DWORD SockCnt = 0;
//	SOCK_SET pool, conn;
//	WSAEVENT hEvent = WSACreateEvent();		//소켓풀이 부족하면 발생하는 이벤트
//	WSAEventSelect(hsoListen, hEvent, FD_ACCEPT);
//	MakeIocp(hsoListen, &ie);
//	ie.conn = &conn;
//	ie.MainThrId = mainThrId = GetCurrentThreadId();
//	SockCnt = MakeSockPool(hsoListen, SOCK_INC_CNT, &pool);
//	cout << "Server Start" << endl;
//	while (true)
//	{
//		DWORD dwWaitRet = MsgWaitForMultipleObjectsEx(1, &hEvent, 2000, QS_POSTMESSAGE, MWMO_INPUTAVAILABLE);	//스레드 메시지큐와 지정한 이벤트를 같이 감시한다.
//
//		if (dwWaitRet == WAIT_FAILED)		//메시지 대기 실패
//			break;
//
//		if (dwWaitRet == WAIT_TIMEOUT)		//지정한 타임시간동안 메시지가 온게 없다
//		{
//			//타임아웃이 발생했으면,이는 소켓 풀의 크기를 감소시킬 수 있는 기회(소켓풀은 기본적으로 최소 4개까지만 유지 최대 10개 까지 늘어날수 있음)
//			if (pool.size() > SOCKPOOL_MIN)
//			{
//				SOCK_SET::iterator it = pool.begin();
//				PSOCK_ITEM psi = *it;
//				pool.erase(it);
//				closesocket(psi->_sock);
//				delete psi;
//
//				cout << "Timeout pool = " << pool.size() << " ,conn = " << conn.size() << endl;
//			}
//			continue;
//		}
//
//		if (dwWaitRet == 1)					//스레드 메시지 큐에 있는 메시지를 꺼내 확인(즉 메시지가 도착했음을 이벤트가 알림) 지정한 이벤트 신호가 아닌 메시지큐 신호가 왔음 = 1
//		{
//			MSG msg;
//			if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
//				continue;
//
//			PSOCK_ITEM psi = (PSOCK_ITEM)msg.lParam;
//			if (msg.message == TM_PROG_EXIT)
//				break;
//
//			if (msg.message == TM_SOCK_CONNECTED)
//			{
//				pool.erase(psi);
//				conn.insert(psi);
//				cout << "Client Connect pool = " << pool.size() << "conn = " << conn.size() << endl;
//			}
//			else if (msg.message == TM_SOCK_DISCONNECT)
//			{
//				conn.erase(psi);
//				if (pool.size() > SOCKPOOL_MIN)
//				{
//					closesocket(psi->_sock);
//					delete psi;
//				}
//				else
//				{
//					LPFN_DISCONNECTEX pfnDisconnectEx = (LPFN_DISCONNECTEX)GetSockExtAPI(psi->_sock, WSAID_DISCONNECTEX);
//					pfnDisconnectEx(psi->_sock, NULL, TF_REUSE_SOCKET, 0);	//소켓의 재사용
//
//					LPFN_ACCEPTEX pfnAcceptEx = (LPFN_ACCEPTEX)GetSockExtAPI(psi->_sock, WSAID_ACCEPTEX);
//					if (pfnAcceptEx(hsoListen, psi->_sock, psi->_buff, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL, (LPOVERLAPPED)psi) == FALSE)
//					{
//						DWORD nErrCode = WSAGetLastError();
//						if (nErrCode != WSA_IO_PENDING)
//						{
//							cout << "AcceptEx failed : " << WSAGetLastError() << endl;
//							closesocket(psi->_sock);
//							delete psi;
//							continue;
//						}
//					}
//					pool.insert(psi);
//				}
//				cout << "Sock Connection released, pool = " << pool.size() << " ,conn = " << conn.size() << endl;
//			}
//		}
//		else			//지정한 이벤트가 신호 상태가 되었다 반환값 = 0 즉 소켓 풀의 크기를 늘려야 함을 의미
//		{
//			WSANETWORKEVENTS ne;
//			WSAEnumNetworkEvents(hsoListen, hEvent, &ne);	//발생한 이벤트메시지를 받아오고 이벤트를 다시 비신호 상태로 전환
//			if (ne.lNetworkEvents & FD_ACCEPT)	//접속 이벤트가 발생함 근데 소켓풀의 크기가 부족함(즉 소켓풀이 비어있다.)
//			{
//				if (pool.size() < SOCKPOOL_MAX)
//					MakeSockPool(hsoListen, SOCK_INC_CNT, &pool);
//				cout << "Listen Event Singnaled, pool = " << pool.size() << ", conn = " << conn.size() << endl;
//			}
//		}
//	}
//
//	CloseHandle(ie._iocp);
//	WaitForMultipleObjects(ie.ThreadCnt, ie.hThread, TRUE, INFINITE);
//	for (DWORD i = 0; i < ie.ThreadCnt; i++)
//		CloseHandle(ie.hThread[i]);
//
//	closesocket(hsoListen);
//	CloseHandle(hEvent);
//	
//	for (SOCK_SET::iterator it = conn.begin(); it != conn.end(); it++)
//	{
//		PSOCK_ITEM psi = *it;
//		closesocket(psi->_sock);
//		delete psi;
//	}
//	for (SOCK_SET::iterator it = pool.begin(); it != pool.end(); it++)
//	{
//		PSOCK_ITEM psi = *it;
//		closesocket(psi->_sock);
//		delete psi;
//	}
//
//	cout << "Sever End" << endl;
//	WSACleanup();
//	return 0;
//}

int _tmain()
{
	WSADATA wsd;
	if (WSAStartup(MAKEWORD(2, 2), &wsd) < 0)
	{
		cout << "WSAStartup failed" << endl;
		return 1;
	}
	try
	{
		char exit[10]{ 0, };
		SOCKET hsoListen = GetListen(9001);
		if (hsoListen == INVALID_SOCKET)
			throw WSAGetLastError();
		cout << "Server Start" << endl;
		cout << "종료 희망시 exit입력" << endl;
		IOCP_ENV ie(hsoListen);
		ie.pwi = CreateThreadpoolWait(MakeSockPool, &ie, NULL);
		if (ie.pwi == NULL)
			throw (int)GetLastError();

		WSAEventSelect(ie.hsoListen, ie.wse, FD_ACCEPT);
		LARGE_INTEGER ll; ll.QuadPart = -40000000LL;
		FILETIME ft; ft.dwHighDateTime = ll.HighPart;
		ft.dwLowDateTime = ll.LowPart;
		SetThreadpoolWait(ie.pwi, ie.wse, &ft); //잉여 소켓 제거를 위해 타임아웃 4초로 지정

		ie.pio = CreateThreadpoolIo((HANDLE)hsoListen, ListenSockCallback, &ie, NULL);
		if (ie.pio == NULL)
			throw (int)GetLastError();
		MakeSockPool(NULL, &ie, ie.pwi, -1);

		while (true)
		{
			cin >> exit;
			if (strcmp(exit, "exit") == 0)
				break;
			else
				cout << "알수없는 명령어 입니다." << endl;
		}
		if (ie.pio != NULL)
		{
			WaitForThreadpoolIoCallbacks(ie.pio, TRUE);
			CloseThreadpoolIo(ie.pio);
		}
		if (hsoListen != INVALID_SOCKET)
			closesocket(hsoListen);
		if (ie.pwi != NULL)
		{
			WaitForThreadpoolWaitCallbacks(ie.pwi, TRUE);
			CloseThreadpoolWait(ie.pwi);
		}
		if (ie.wse != NULL)
			CloseHandle(ie.wse);
		for (SOCK_SET::iterator it = ie.conn.begin(); it != ie.conn.end(); it++)
		{
			PSOCK_ITEM psi = *it;
			CancelIoEx((HANDLE)psi->_sock, psi);
			WaitForThreadpoolIoCallbacks(psi->pio, TRUE);
			closesocket(psi->_sock);
			CloseThreadpoolIo(psi->pio);
			delete psi;
		}
		for (SOCK_SET::iterator it = ie.pool.begin(); it != ie.pool.end(); it++)
		{
			PSOCK_ITEM psi = *it;
			closesocket(psi->_sock);
			CloseThreadpoolIo(psi->pio);
			delete psi;
		}
		if (ie.hEvent != NULL)
			CloseHandle(ie.hEvent);
		DeleteCriticalSection(&ie.cs);
	}
	catch (int ex)
	{
		cout << "Error occurred in main, " << ex << endl;
	}
	cout << "Server End" << endl;
	WSACleanup();

	return 0;
}