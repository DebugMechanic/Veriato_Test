// Veriato_Test.cpp : Defines the entry point for the console application.

/*
	Date: 03/22/2018
	Author: Debug Mechanic
	OS: All code bases, tests and debugging were completed on x64 Windows 7 Professional
	Server: Veriato_Test.cpp

	Test Deliverable:

		Code a solution that does the following:

		When a user runs the Windows Notepad application and a saves a file via “Save” or “Save As”, capture the path and size of the file.
		Send this information to a server application that displays the data it receives and stores it on disk.
*/

#include <SDKDDKVer.h>
#include <stdio.h>
#include <tchar.h>
#include <conio.h>
#include <WinSock2.h>
#include <Windows.h>
#include <vector>
#include <TlHelp32.h>
#include <Psapi.h>
#include "Log.h"
#include "Server.h"


// Globals
typedef struct tag_master{
	bool bThreadInUse;
	DWORD dwThreadID;
	HANDLE hThread;
} MASTER;


// Prototypes
int Initialize(WSADATA *wd);


int _tmain(int argc, _TCHAR* argv[])
{
	MASTER ServerThread;		
	DWORD dwThreadState = 0;
	
	/* Clear Log File */
	Sleep(2000);
	ClearLog();
	Sleep(2000);

	Log("\n*********************************************************************\n");
	Log("*************************  Veriato Log  *****************************\n");
	Log("*********************************************************************\n");

	/* Initialize Winsock 2 */
	WSADATA WSA_Data;
	Initialize(&WSA_Data);
		
	Log("[Main]: Starting Server\n");	
	ServerThread.hThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)Server, NULL, NULL, &ServerThread.dwThreadID);
	if (ServerThread.hThread && ServerThread.dwThreadID)
	{
		dwThreadState = WaitForSingleObject(ServerThread.hThread, INFINITE);
		switch (dwThreadState) {
			case WAIT_FAILED:
				Log("[Main]: Server Thread Failed...\n"); break;
			case WAIT_TIMEOUT:
				Log("[Main]: Server Thread Timed Out...\n"); break;
			case WAIT_OBJECT_0:
				Log("[Main]: Server Thread Completed...\n"); break;
		}
		
		CloseHandle(ServerThread.hThread);
	}
	return 0;
}


int Initialize(WSADATA *wd)
{	
	int nResult = NO_ERROR;
	if ((nResult = WSAStartup(MAKEWORD(2, 2), wd)) != NO_ERROR) 
	{
		Log("\n\n[Initialize]: WSAStartup Error : %d\n", WSAGetLastError());
		return 1;
	
	} else {
		Log("[Initialize]: WSAStartup Successful!\n");
	}

	return 0;
}

