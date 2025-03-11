#pragma once
#include <set>
using namespace std;

#define IOKEY_LISTEN	1
#define IOKEY_CHILD		2
#define MAX_TH			10
#define SOCKPOOL_MAX	10

struct IOCP_ENV
{
	HANDLE _iocp;
	HANDLE hThread[MAX_TH];
	DWORD ThreadCnt = 0;
};

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
using PIOCP_ENV = IOCP_ENV*;
using SOCK_SET = set<PSOCK_ITEM>;

PVOID GetSockExtAPI(SOCKET sock, GUID guidFn);
SOCKET GetListen(short port, int nBacklog);
PIOCP_ENV MakeIocp(int Th, HANDLE hDevice);
DWORD MakeSockPool(SOCKET hsoListen, int Sc, SOCK_SET ss);
DWORD IocpCallBack(PVOID pParam);