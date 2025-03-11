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

	WSACleanup();
}