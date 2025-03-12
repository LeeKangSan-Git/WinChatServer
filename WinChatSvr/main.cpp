#include <WinSock2.h>
#include <windows.h>
#include <iostream>
#include <tchar.h>
#include <MSWSock.h>
#include "module.h"
#include "main.h"
using namespace std;

BOOL CtrlHandler(DWORD fdwCtrlType)
{
	PostThreadMessage(mainThrId, TM_PROG_EXIT, 0, 0);
	return TRUE;
}

int _tmain()
{
	WSADATA wsd;
	if (WSAStartup(MAKEWORD(2, 2), &wsd))
	{
		cout << "WSAStartup failed" << endl;
		return 1;
	}

	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE))
	{
		cout << "CtrlHandler failed : " << GetLastError() << endl;
		return 1;
	}

	SOCKET hsoListen = GetListen(9001);
	if (hsoListen == INVALID_SOCKET)
	{
		cout << "Make ListenSock failed : " << WSAGetLastError() << endl;
		WSACleanup();
		return 1;
	}

	IOCP_ENV ie;
	DWORD SockCnt = 0;
	SOCK_SET pool, conn;
	WSAEVENT hEvent = WSACreateEvent();		//����Ǯ�� �����ϸ� �߻��ϴ� �̺�Ʈ
	WSAEventSelect(hsoListen, hEvent, FD_ACCEPT);
	MakeIocp(hsoListen, &ie);
	ie.conn = &conn;
	ie.MainThrId = mainThrId = GetCurrentThreadId();
	SockCnt = MakeSockPool(hsoListen, SOCK_INC_CNT, &pool);
	cout << "Server Start" << endl;
	while (true)
	{
		DWORD dwWaitRet = MsgWaitForMultipleObjectsEx(1, &hEvent, 2000, QS_POSTMESSAGE, MWMO_INPUTAVAILABLE);	//������ �޽���ť�� ������ �̺�Ʈ�� ���� �����Ѵ�.

		if (dwWaitRet == WAIT_FAILED)		//�޽��� ��� ����
			break;

		if (dwWaitRet == WAIT_TIMEOUT)		//������ Ÿ�ӽð����� �޽����� �°� ����
		{
			//Ÿ�Ӿƿ��� �߻�������,�̴� ���� Ǯ�� ũ�⸦ ���ҽ�ų �� �ִ� ��ȸ(����Ǯ�� �⺻������ �ּ� 4�������� ���� �ִ� 10�� ���� �þ�� ����)
			if (pool.size() > SOCKPOOL_MIN)
			{
				SOCK_SET::iterator it = pool.begin();
				PSOCK_ITEM psi = *it;
				pool.erase(it);
				closesocket(psi->_sock);
				delete psi;

				cout << "Timeout pool = " << pool.size() << " ,conn = " << conn.size() << endl;
			}
			continue;
		}

		if (dwWaitRet == 1)					//������ �޽��� ť�� �ִ� �޽����� ���� Ȯ��(�� �޽����� ���������� �̺�Ʈ�� �˸�) ������ �̺�Ʈ ��ȣ�� �ƴ� �޽���ť ��ȣ�� ���� = 1
		{
			MSG msg;
			if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				continue;

			PSOCK_ITEM psi = (PSOCK_ITEM)msg.lParam;
			if (msg.message == TM_PROG_EXIT)
				break;

			if (msg.message == TM_SOCK_CONNECTED)
			{
				pool.erase(psi);
				conn.insert(psi);
				cout << "Client Connect pool = " << pool.size() << "conn = " << conn.size() << endl;
			}
			else if (msg.message == TM_SOCK_DISCONNECT)
			{
				conn.erase(psi);
				if (pool.size() > SOCKPOOL_MIN)
				{
					closesocket(psi->_sock);
					delete psi;
				}
				else
				{
					LPFN_DISCONNECTEX pfnDisconnectEx = (LPFN_DISCONNECTEX)GetSockExtAPI(psi->_sock, WSAID_DISCONNECTEX);
					pfnDisconnectEx(psi->_sock, NULL, TF_REUSE_SOCKET, 0);	//������ ����

					LPFN_ACCEPTEX pfnAcceptEx = (LPFN_ACCEPTEX)GetSockExtAPI(psi->_sock, WSAID_ACCEPTEX);
					if (pfnAcceptEx(hsoListen, psi->_sock, psi->_buff, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, NULL, (LPOVERLAPPED)psi) == FALSE)
					{
						DWORD nErrCode = WSAGetLastError();
						if (nErrCode != WSA_IO_PENDING)
						{
							cout << "AcceptEx failed : " << WSAGetLastError() << endl;
							closesocket(psi->_sock);
							delete psi;
							continue;
						}
					}
					pool.insert(psi);
				}
				cout << "Sock Connection released, pool = " << pool.size() << " ,conn = " << conn.size() << endl;
			}
		}
		else			//������ �̺�Ʈ�� ��ȣ ���°� �Ǿ��� ��ȯ�� = 0 �� ���� Ǯ�� ũ�⸦ �÷��� ���� �ǹ�
		{
			WSANETWORKEVENTS ne;
			WSAEnumNetworkEvents(hsoListen, hEvent, &ne);	//�߻��� �̺�Ʈ�޽����� �޾ƿ��� �̺�Ʈ�� �ٽ� ���ȣ ���·� ��ȯ
			if (ne.lNetworkEvents & FD_ACCEPT)	//���� �̺�Ʈ�� �߻��� �ٵ� ����Ǯ�� ũ�Ⱑ ������(�� ����Ǯ�� ����ִ�.)
			{
				if (pool.size() < SOCKPOOL_MAX)
					MakeSockPool(hsoListen, SOCK_INC_CNT, &pool);
				cout << "Listen Event Singnaled, pool = " << pool.size() << ", conn = " << conn.size() << endl;
			}
		}
	}

	CloseHandle(ie._iocp);
	WaitForMultipleObjects(ie.ThreadCnt, ie.hThread, TRUE, INFINITE);
	for (DWORD i = 0; i < ie.ThreadCnt; i++)
		CloseHandle(ie.hThread[i]);

	closesocket(hsoListen);
	CloseHandle(hEvent);
	
	for (SOCK_SET::iterator it = conn.begin(); it != conn.end(); it++)
	{
		PSOCK_ITEM psi = *it;
		closesocket(psi->_sock);
		delete psi;
	}
	for (SOCK_SET::iterator it = pool.begin(); it != pool.end(); it++)
	{
		PSOCK_ITEM psi = *it;
		closesocket(psi->_sock);
		delete psi;
	}

	cout << "Sever End" << endl;
	WSACleanup();
	return 0;
}