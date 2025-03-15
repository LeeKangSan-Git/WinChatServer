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

//���� ���� ������ �޽���
//#define TM_PROG_EXIT			WM_USER + 1
//#define TM_SOCK_CONNECTED		WM_USER + 2
//#define TM_SOCK_DISCONNECT	WM_USER + 3

//���Ӱ� ������ �޽���
#define CMD_DELETE	0	//������ ����,closesocket�� ���� ������ �ݴ´�.
#define CMD_NEW		1	//������ ���� �����Ǿ�����,���� TP_IO��ü�� �����Ѵ�.
#define CMD_REUSE	2	//���� ����,DisconnectEx ȣ�� �� �ٽ� AcceptEx�� ȣ���Ѵ�.

#define IOKEY_LISTEN	1
#define IOKEY_CHILD		2
#define MAX_TH			3
#define SOCKPOOL_MAX	32	//����Ǯ �ִ� ������
#define SOCKPOOL_MIN	4	//����Ǯ �ּ� ������
#define SOCK_INC_CNT	4	//������ ũ�� ���� ������

struct SOCK_ITEM : OVERLAPPED
{
	SOCKET _sock;
	PTP_IO pio;
	char _buff[128];
	char NickName[64];
	BOOL NickCheck;

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
	//HANDLE _iocp;
	//HANDLE hThread[MAX_TH];
	PTP_IO pio;
	PTP_WAIT pwi;	//���� Ǯ�� ��� ������ ������ ��Ȳ���� ���ο� ������ ������ ��� ������ ó���� ���� ��ü
	SOCKET hsoListen;
	HANDLE hEvent; //���� Ǯ ���� Ÿ�̹��� ���� �̺�Ʈ
	SOCK_SET conn, pool;
	DWORD MainThrId;
	WSAEVENT wse; //���� �̺�Ʈ �߻��� ȣ��� ���� �Լ��� ���� �̺�Ʈ
	CRITICAL_SECTION cs;
	//DWORD ThreadCnt;
	IOCP_ENV(SOCKET Listen)
	{
		//memset(this, 0, sizeof(*this));
		hsoListen = Listen;
		wse = WSACreateEvent();
		hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		InitializeCriticalSection(&cs);
	}
};
using PIOCP_ENV = IOCP_ENV*;

PVOID GetSockExtAPI(SOCKET sock, GUID guidFn);
SOCKET GetListen(short Port, int nBacklog = SOMAXCONN);
//DWORD MakeIocp(SOCKET hDevice, PIOCP_ENV pi, int Th = MAX_TH);
//DWORD MakeSockPool(SOCKET hsoListen, DWORD Sc, SOCK_SET* ss);
//DWORD WINAPI IocpCallBack(PVOID pParam);


bool SocketManger(UINT uCmd, PIOCP_ENV pie, PSOCK_ITEM psi, PTP_CALLBACK_INSTANCE pInst = NULL);
VOID CALLBACK MakeSockPool(PTP_CALLBACK_INSTANCE pci, PVOID pCtx, PTP_WAIT ptpWait, TP_WAIT_RESULT tResult);
VOID CALLBACK ListenSockCallback(PTP_CALLBACK_INSTANCE pci, PVOID pCtx, PVOID pol, ULONG IoResult, ULONG_PTR dwTrBytes, PTP_IO pio);
VOID CALLBACK ChiledSockCallBack(PTP_CALLBACK_INSTANCE Instance, PVOID pctx, PVOID pol, ULONG IoResult, ULONG_PTR dwTrBytes, PTP_IO pio);