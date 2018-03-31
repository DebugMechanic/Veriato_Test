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
#include <WinSock2.h>
#include <Windows.h>
#include "Log.h"
#include "Server.h"


// Globals
typedef struct tag_Thread{
	bool bThreadInUse;
	DWORD dwThreadID;
	HANDLE hThread;
	DWORD dwThreadState;
} THREAD, *PTHREAD;


int main(int argc, char* argv[])
{
	THREAD Master;
	memset(&Master, 0x0, sizeof(THREAD));
	
	/* Clear Log File, Sleeps Allow Changes In SnakeTail */
	Sleep(2000);
	ClearLog();
	Sleep(2000);

	Log("\n*********************************************************************\n");
	Log("*************************  Veriato Log  *****************************\n");
	Log("*********************************************************************\n");

	Log("[Main]: Starting Server\n");
	Master.hThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)Server, NULL, NULL, &Master.dwThreadID);
	if (Master.hThread && Master.dwThreadID)
	{
		Master.dwThreadState = WaitForSingleObject(Master.hThread, INFINITE);
		switch (Master.dwThreadState) {
			case WAIT_FAILED:
				Log("[Main]: Server Thread Failed...\n"); break;
			case WAIT_TIMEOUT:
				Log("[Main]: Server Thread Timed Out...\n"); break;
			case WAIT_OBJECT_0:
				Log("[Main]: Server Thread Completed...\n"); break;
		}
		
		CloseHandle(Master.hThread);
	}
	return 0;
}

