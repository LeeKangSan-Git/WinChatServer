#pragma once
#include <set>
using namespace std;

#ifndef STATUS_LOCAL_DISCONNECT
#	define STATUS_LOCAL_DISCONNECT	((NTSTATUS)0xC000013BL)	//ERROR_NETNAME_DELETED
#endif
#ifndef STATUS_REMOTE_DISCONNECT
#	define STATUS_REMOTE_DISCONNECT	((NTSTATUS)0xC000013CL)	//ERROR_NETNAME_DELETED
#endif
#ifndef STATUS_CONNECTION_RESET
#	define STATUS_CONNECTION_RESET	((NTSTATUS)0xC000020DL)	//ERROR_NETNAME_DELETED
#endif
#ifndef STATUS_CANCELLED
#	define STATUS_CANCELLED			((NTSTATUS)0xC0000120L)	//ERROR_OPERATION_ABORTED
#endif

//유저 정의 스레드 메시지
#define TM_PROG_EXIT			WM_USER + 1
#define TM_SOCK_CONNECTED		WM_USER + 2
#define TM_SOCK_DISCONNECT		WM_USER + 3

#define IOKEY_LISTEN	1
#define IOKEY_CHILD		2
#define MAX_TH			3
#define SOCKPOOL_MAX	32	//소켓풀 최대 사이즈
#define SOCKPOOL_MIN	4	//소켓풀 최소 사이즈
#define SOCK_INC_CNT	4	//소켓의 크기 증가 사이즈

struct SOCK_ITEM : OVERLAPPED
{
	SOCKET _sock;
	char _buff[128];

	SOCK_ITEM(SOCKET sock)
	{
		memset(this, 0, sizeof(*this));
		_sock = sock;
	}
};
using PSOCK_ITEM = SOCK_ITEM*;
using SOCK_SET = set<PSOCK_ITEM>;
struct IOCP_ENV
{
	HANDLE _iocp;
	HANDLE hThread[MAX_TH];
	SOCK_SET* conn;
	DWORD MainThrId;
	DWORD ThreadCnt;
	IOCP_ENV()
	{
		memset(this, 0, sizeof(*this));
	}
};
using PIOCP_ENV = IOCP_ENV*;

PVOID GetSockExtAPI(SOCKET sock, GUID guidFn);
SOCKET GetListen(short Port, int nBacklog = SOMAXCONN);
DWORD MakeIocp(SOCKET hDevice, PIOCP_ENV pi, int Th = MAX_TH);
DWORD MakeSockPool(SOCKET hsoListen, DWORD Sc, SOCK_SET* ss);
DWORD WINAPI IocpCallBack(PVOID pParam);