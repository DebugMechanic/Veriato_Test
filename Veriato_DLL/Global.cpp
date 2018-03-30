#include "Global.h"
#include <TlHelp32.h>
#include <stdint.h>
#include "CreatedNtApi.h"
#include "Log.h"


void EnableDebugPriv( void )
{
    HANDLE           hToken ;
    LUID             SeDebugNameValue ;
    TOKEN_PRIVILEGES TokenPrivileges ;

    if( OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) )
    {
        if( LookupPrivilegeValue( NULL, SE_DEBUG_NAME, &SeDebugNameValue ) )
        {			
            TokenPrivileges.PrivilegeCount           = 1 ;
            TokenPrivileges.Privileges[0].Luid       = SeDebugNameValue ;
			TokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            if( AdjustTokenPrivileges( hToken, FALSE, &TokenPrivileges, sizeof(TOKEN_PRIVILEGES), NULL, NULL ) )
            {
#if DEBUG_PRIV
				Log( "Adjustment Of Privileges Completed...\n" );
#endif
                CloseHandle(hToken);
            
			} else {
                CloseHandle(hToken);
#if DEBUG_PRIV
                Log( "AdjustTokenPrivileges() Failed!\n" );
#endif
            }
        
		} else {
            CloseHandle( hToken );
#if DEBUG_PRIV
            Log( "LookupPrivilegeValue() Failed!\n");
#endif
        }

    } else {
#if DEBUG_PRIV
		Log("[EnableDebugPriv] OpenProcessToken Failed, Error: %016llX\n", GetLastError());
#endif
    }
}


DWORD Get_PID_From_Process_Handle( HANDLE process_handle )
{
    PROCESS_BASIC_INFORMATION pbi = {}; 
	ULONG ulSize;

    LONG ( WINAPI *NtQueryInformationProcess )( HANDLE ProcessHandle, ULONG ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength );
    *(FARPROC *)&NtQueryInformationProcess = GetProcAddress( GetModuleHandle("ntdll"), "NtQueryInformationProcess" );

    if( NtQueryInformationProcess != NULL && NtQueryInformationProcess( process_handle, 0, &pbi, sizeof( pbi ), &ulSize ) >= 0 && ulSize == sizeof( pbi ) ) 
	{
        return (DWORD)pbi.UniqueProcessId;
    }
    return 0;
}


DWORD Suspend_All_Threads(DWORD processId)
{
	DWORD dwSuspendCount = 0;
	DWORD retval = 0;
	HANDLE hSnapShot;
	HANDLE hThread;
	THREADENTRY32 te = { 0 };
	te.dwSize = sizeof(THREADENTRY32);

	hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, processId);
	if (INVALID_HANDLE_VALUE == hSnapShot)
	{
#if DEBUG_THREADS
		Log("Invalid Snap Handle\n");
#endif
	}

	if (Thread32First(hSnapShot, &te)) {

		do
		{
			if (te.th32OwnerProcessID == processId) {

				if (0 == retval) {
					retval = (DWORD)te.th32ThreadID;
				}

				hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
				if (NULL != hThread)
				{
					if ((DWORD)te.th32ThreadID != GetThreadId(GetCurrentThread()))
					{
#if DEBUG_THREADS
						Log("Suspending Victim Thread %d\n", (DWORD)te.th32ThreadID);
#endif
						dwSuspendCount = SuspendThread(hThread);
#if DEBUG_THREADS
						Log("Suspend Count: %d\n", dwSuspendCount);
#endif
						CloseHandle(hThread);
					}
				} else {
#if DEBUG_THREADS
					Log("Failed To Open Victim Thread %d\n", (DWORD)te.th32ThreadID);
#endif
				}
			}
		} while (Thread32Next(hSnapShot, &te));

	} else {
#if DEBUG_THREADS
		Log("Thread32First() Failed! %d\n", (DWORD)GetLastError());
#endif
	}

	CloseHandle(hSnapShot);
	return retval;
}


void Resume_All_Threads(DWORD processId)
{
	DWORD dwSuspendCount = 0;
	HANDLE hThread, hSnapShot;
	THREADENTRY32 te = { 0 };
	te.dwSize = sizeof(THREADENTRY32);

	hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, processId);
	if (INVALID_HANDLE_VALUE == hSnapShot)
	{
#if DEBUG_THREADS
		Log("Invalid Snap Handle\n");
#endif
	}

	if (Thread32First(hSnapShot, &te)) 
	{
		do
		{
			if (te.th32OwnerProcessID == processId) {
				hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
				if (NULL != hThread)
				{
#if DEBUG_THREADS
					Log("Resuming Victim Thread %d\n", (DWORD)te.th32ThreadID);
#endif

					do{
						dwSuspendCount = ResumeThread(hThread);
#if DEBUG_THREADS
						Log("Suspend Count: %d\n", dwSuspendCount);
#endif
						if (dwSuspendCount == 1)
						{
#if DEBUG_THREADS
							Log("Thread Restarted\n");
#endif
						}
					} while (dwSuspendCount != 0);
					CloseHandle(hThread);
				} else {
#if DEBUG_THREADS
					Log("Failed To Open Victim Thread %d\n", (DWORD)te.th32ThreadID);
#endif
				}
			}
		} while (Thread32Next(hSnapShot, &te));
	} else {
#if DEBUG_THREADS
		Log("Thread32First Failed! %d\n", (DWORD)GetLastError());
#endif
	}
	CloseHandle(hSnapShot);
}

