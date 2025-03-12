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

#define IOKEY_LISTEN	1
#define IOKEY_CHILD		2
#define MAX_TH			10
#define SOCKPOOL_MAX	10

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
	SOCK_SET pool, conn;
	HANDLE ExtEvent;
	DWORD ThreadCnt = 0;
};
using PIOCP_ENV = IOCP_ENV*;

PVOID GetSockExtAPI(SOCKET sock, GUID guidFn);
SOCKET GetListen(short port, int nBacklog = SOMAXCONN);
PIOCP_ENV MakeIocp(int Th, HANDLE hDevice);
DWORD MakeSockPool(SOCKET hsoListen, int Sc, SOCK_SET ss);
DWORD IocpCallBack(PVOID pParam);