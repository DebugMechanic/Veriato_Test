#ifndef GLOBAL_H
#define GLOBAL_H

#include <Windows.h>


typedef struct tagGlobal
{
	DWORD64 g_DLL_Handle;     // Injected .dll Handle 
	DWORD64 g_Base_Address;   // Base Address
	HANDLE  g_Process_Handle; // Handle To Process
	DWORD   g_Process_ID;     // Process ID
	HANDLE  g_Pseudo_Handle;  // All Access Psuedo Handle	
	HANDLE  g_Thread_Handle;  // Thread Handle
} GLOBAL, *PGLOBAL;
extern PGLOBAL pGlobal;

void EnableDebugPriv( void );
DWORD Get_PID_From_Process_Handle( HANDLE process_handle );

DWORD Suspend_All_Threads(DWORD processId);
void Resume_All_Threads(DWORD processId);


#endif

