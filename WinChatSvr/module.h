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
//#define TM_PROG_EXIT			WM_USER + 1
//#define TM_SOCK_CONNECTED		WM_USER + 2
//#define TM_SOCK_DISCONNECT	WM_USER + 3

//새롭게 정의한 메시지
#define CMD_DELETE	0	//소켓의 해제,closesocket을 통해 소켓을 닫는다.
#define CMD_NEW		1	//소켓이 새로 생성되었으며,관련 TP_IO객체를 생성한다.
#define CMD_REUSE	2	//소켓 재사용,DisconnectEx 호출 후 다시 AcceptEx를 호출한다.

#define IOKEY_LISTEN	1
#define IOKEY_CHILD		2
#define MAX_TH			3
#define SOCKPOOL_MAX	32	//소켓풀 최대 사이즈
#define SOCKPOOL_MIN	4	//소켓풀 최소 사이즈
#define SOCK_INC_CNT	4	//소켓의 크기 증가 사이즈

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
	PTP_WAIT pwi;	//소켓 풀의 모든 소켓이 소진된 상황에서 새로운 접속이 들어왔을 경우 통지를 처리를 위한 객체
	SOCKET hsoListen;
	HANDLE hEvent; //소켓 풀 제거 타이밍을 위한 이벤트
	SOCK_SET conn, pool;
	DWORD MainThrId;
	WSAEVENT wse; //리슨 이벤트 발생시 호출될 리슨 함수를 위한 이벤트
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