#include <WinSock2.h>
#include <windows.h>
#include <iostream>
#include <tchar.h>
#include "module.h"
using namespace std;

int main()
{
	WSADATA wsd;
	if (WSAStartup(MAKEWORD(2, 2), &wsd))
	{
		cout << "WSAStartup failed" << endl;
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
	ie._iocp = MakeIocp(3, (HANDLE)hsoListen);
	ie.ExtEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	
	while (true)
	{

	}

	WSACleanup();
	return 0;
}