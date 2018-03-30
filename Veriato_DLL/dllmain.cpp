// dllmain.cpp : Defines the entry point for the DLL application.


/*
	Date: 03/22/2018
	Author: Debug Mechanic
	OS: All code bases, tests and debugging were completed on x64 Windows 7 Professional
	Dll: Injectable DLL (Veriato_DLL)

	Test Deliverable:

		Code a solution that does the following:

			When a user runs the Windows Notepad application and a saves a file via “Save” or “Save As”, capture the path and size of the file. 
			Send this information to a server application that displays the data it receives and stores it on disk.
*/


#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>
#include <strsafe.h>
#include <Aclapi.h>
#include <winternl.h>
#include <winsock2.h>
#include "Global.h"
#include "BlackMagic.h"
#include "Log.h"


/* ( Debugging Switches )

	Add To Project Settings: 
		Configuration Properties -> C/C++ -> Preprocessor

	Preprocessor definitions: 
		DEBUG_THREADS
		DEBUG_CONSOLE
		DEBUG_BLACKMAGIC
		DEBUG_PROLOG_HOOK
		DEBUG_PROLOG_DETOUR
		DEBUG_EPILOG_HOOK
		DEBUG_EPILOG_DETOUR
		DEBUG_NET
*/


// .ASM Globals 
extern "C" 
{	
	// Prolog Setup
	void Prolog_WriteFile(void);
	void Prolog_Fun(void);	
	DWORD64 hFile_Param1;
	DWORD64	lpBuffer_Param2;
	DWORD64 nNumOfBytesToWrite_Param3;
	DWORD64 Fixup_Prolog_ASM_Return;
	
	// Epilog Setup
	void Epilog_WriteFile(void);
	void Epilog_Fun(void);
	DWORD64 Fixup_Epilog_ASM_Return;
}


// Packet
typedef struct tag_packet{
	DWORD  PacketLen;
	CHAR   Path[MAX_PATH + 1];
	size_t PathLen;
	LARGE_INTEGER FileSize;	
} PACKET, *PPACKET;


/*******************************************************************************************************
	The Macro, typedef, structure, and assignment are for PlayTime()'s duplication handle functionality.
*******************************************************************************************************/
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

typedef NTSTATUS(WINAPI * PFN_NTQUERYINFORMATIONFILE)(
	IN HANDLE FileHandle,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	OUT PVOID FileInformation,
	IN ULONG Length,
	IN FILE_INFORMATION_CLASS FileInformationClass
);

// FILE_NAME_INFORMATION contains name of queried file object.
typedef struct _FILE_NAME_INFORMATION {
	ULONG FileNameLength;
	WCHAR FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;

// Undocumented FILE_INFORMATION_CLASS: FileNameInformation
const FILE_INFORMATION_CLASS FileNameInformation = (FILE_INFORMATION_CLASS)9;

/******************************************************************************************************/


// Externs
PGLOBAL pGlobal;


// Prototypes
int Initialize(WSADATA *wd);
BOOL WriteFile_Prolog_Hook();
BOOL WriteFile_Epilog_Hook();
BOOL SendPackage(PPACKET* pPackage);


DWORD WINAPI MasterThread(void)
{
	BOOL result = 0;

#if DEBUG_CONSOLE
	fprintf_s(stdout, "\n\n[MasterThread]: Starting On [%s][%s]...\n", __DATE__, __TIME__);
	Log("\n\n[MasterThread]: Starting On [%s][%s]...\n", __DATE__, __TIME__);
#endif
	
	EnableDebugPriv(); // Enable Debug Privileges
	pGlobal->g_Pseudo_Handle = GetCurrentProcess(); // has PROCESS_ALL_ACCESS access rights
	pGlobal->g_Process_ID    = Get_PID_From_Process_Handle(pGlobal->g_Pseudo_Handle);

	Suspend_All_Threads(pGlobal->g_Process_ID);

	result = WriteFile_Prolog_Hook(); // Filter, Alter, Block Input Parameters.
	if (result != 0)
	{
		result = WriteFile_Epilog_Hook(); // Filter Return Values.
		if (result != 0)
		{
			Resume_All_Threads(pGlobal->g_Process_ID);
			return 0; // :) Good
		}
	}
	
	return 1; // :( Error
}


BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{

#if DEBUG_CONSOLE
	if (AllocConsole() != 0) {
		freopen("CONOUT$", "w", stdout); 
	}
#endif

	// Allocate Our Global Structure
	pGlobal = (PGLOBAL)malloc(sizeof(GLOBAL));
	memset(pGlobal, 0x0, sizeof(GLOBAL));

	// Reserve Injected .dll Handle
	pGlobal->g_DLL_Handle = (DWORD64)hModule;

	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			// Create Our Master Thread.
			pGlobal->g_Thread_Handle = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MasterThread, 0, 0, 0); 
			break;
		case DLL_PROCESS_DETACH:
			free(pGlobal);
			break;
		default:
			break;
	}

	return TRUE;
}


void Prolog_Fun(void)
{
#if DEBUG_CONSOLE
	fprintf_s(stdout, "[Prolog_Fun] Hello From Prolog_Fun()\n");
#endif

	size_t ret = 0;	
	PPACKET Package = (PPACKET)malloc(sizeof(PACKET));
	memset(Package, 0x0, sizeof(PACKET));

	// Grab the ntdll.dll handle
	HMODULE hNt = GetModuleHandle("ntdll.dll");
	if (hNt)
	{
		// Grab the NtQueryInformationFile address.
		PFN_NTQUERYINFORMATIONFILE NtQueryInformationFile = (PFN_NTQUERYINFORMATIONFILE)GetProcAddress(hNt, "NtQueryInformationFile");
		if (NtQueryInformationFile)
		{
			// Duplicate the handle in the current process.
			HANDLE hCopy;
			if (DuplicateHandle(GetCurrentProcess(), (HANDLE)hFile_Param1, GetCurrentProcess(), &hCopy, MAXIMUM_ALLOWED, FALSE, 0))
			{
				// Retrieve the file name information about the file object.
				IO_STATUS_BLOCK ioStatus;
				DWORD dwInfoSize = MAX_PATH * 2 * 2;
				PFILE_NAME_INFORMATION pNameInfo = (PFILE_NAME_INFORMATION)malloc(dwInfoSize);				
								
				if (NtQueryInformationFile(hCopy, &ioStatus, pNameInfo, dwInfoSize, FileNameInformation) == STATUS_SUCCESS)
				{
					// NtQueryInformationFile() returns the file information in unicode.
					WCHAR wszFileName[MAX_PATH + 1];
					StringCchCopyNW(wszFileName, MAX_PATH + 1,
						pNameInfo->FileName, /*must be WCHAR*/
						pNameInfo->FileNameLength /*in bytes*/ / 2);
					
					// Unicode to Ascii
					Package->PathLen = pNameInfo->FileNameLength / 2;
					ret = wcstombs(Package->Path, pNameInfo->FileName, Package->PathLen);
					if (ret == Package->PathLen)
						Package->Path[Package->PathLen] = '\0';

					// Debug Information
#if DEBUG_PROLOG_DETOUR
					fprintf_s(stdout, "\n[Prolog_Fun] WriteFile(Handle: %016llX, pText: %p, Length: %d)\n", VictimHFile, VictimText, VictimLength);	
					fprintf_s(stdout, "[Prolog_Fun] pNameInfo-> FileName: %p, Length: %d\n", pNameInfo, pNameInfo->FileNameLength);
					fprintf_s(stdout, "[Prolog_Fun] File Location: %s\n", Package->Path);
#endif					
				}
				free(pNameInfo);
				CloseHandle(hCopy);

				// Send
				SendPackage(&Package);
			}

			free(Package);
		}
	}
}


void Epilog_Fun(void)
{
#if DEBUG_CONSOLE
	fprintf_s(stdout, "[Epilog_Fun] Hello From Epilog_Fun()\n");
#endif

	PPACKET Package = (PPACKET)malloc(sizeof(PACKET));
	memset(Package, 0x0, sizeof(PACKET));

	// Duplicate the handle in the current process
	HANDLE hCopy;
	if (DuplicateHandle(GetCurrentProcess(), (HANDLE)hFile_Param1, GetCurrentProcess(), &hCopy, MAXIMUM_ALLOWED, FALSE, 0))
	{
		// Grab File Size
		GetFileSizeEx(hCopy, &Package->FileSize);
#if DEBUG_EPILOG_DETOUR
		fprintf_s(stdout, "[Epilog_Fun] File Size: %d bytes.\n", Package->FileSize.QuadPart);
#endif
		CloseHandle(hCopy);

		// Send
		SendPackage(&Package);
	}

	free(Package);
}


BOOL SendPackage( PPACKET* pPackage )
{
	WSADATA WSA_Data;
	PPACKET Package = *pPackage;

#if DEBUG_NET
	fprintf_s(stdout, "[SendPackage] Package: %p\n", Package);
	DebugBreak();
#endif

	// Calculating the Packet Length.
	Package->PacketLen = sizeof(PACKET);
	
	// Initialize Winsock 2			
	Initialize(&WSA_Data);

	// Socket Creation
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock != INVALID_SOCKET)
	{
		// Address Setup and Connection Information.
		sockaddr_in Address;
		Address.sin_family = AF_INET;
		Address.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
		Address.sin_port = htons(55555);
		if (SOCKET_ERROR != connect(sock, (SOCKADDR*)&Address, sizeof(Address)))
		{
			// Sending Packet To Server.
			DWORD dwFlags = 0;
			int iBytesSent = -1;
			iBytesSent = send(sock, (char*)Package, sizeof(PACKET), dwFlags);
			if (iBytesSent == -1)
			{
#if DEBUG_NET
				fprintf_s(stdout, "[SendPackage] Sending Error: %d\n", WSAGetLastError());
#endif
				return 0; // :( Error
			}

		} else {
#if DEBUG_NET
			fprintf_s(stdout, "[SendPackage] Connection Failed To Server\n");
#endif
			return 0; // :( Error
		}

	} else {
#if DEBUG_NET
		fprintf_s(stdout, "[SendPackage] Error Socket Failed: %d\n", WSAGetLastError());
#endif
		return 0; // :( Error
	}

	closesocket(sock);
	WSACleanup();
	return 1; // :) Good
}


int Initialize(WSADATA *wd)
{	
	int nResult;
	if ((nResult = WSAStartup(MAKEWORD(2, 2), wd)) != NO_ERROR)
	{
#if DEBUGLOG_NET
		fprintf_s(stdout, "\n[Initialize]: WSAStartup Error : %d\n", WSAGetLastError());
#endif
		return 1; // :( Error
	} 
		
	return 0; // :) Good
}


BOOL WriteFile_Prolog_Hook()
{
	// WriteFile() Prologue Hook - To Filter, Alter and Block Input Parameters.
	BLACKMAGIC_MODEL* pProlog = new BLACKMAGIC_MODEL();
	pProlog->Runtime_Dynamic_Linking("WriteFile", "Kernel32.dll");
	pProlog->Define_Original_Signature(P_SIG_SIZE, "\xFF\xF3\x48\x83\xEC\x30\x33\xDB\x4D\x85\xC9\x74\x03\x41\x89", "xxxxx?xxxxxx?xxx");
	pProlog->Setup_Detour_Patch(P_PATCH_OFFSET, P_PATCH_SIZE, "\x48\xB8\xDE\xC0\xBE\xBA\xFE\xCA\xED\xFE\x50\xC3\x90"); // Technique: mov rax, push rax, ret
	pProlog->Assign_Detour((BYTE*)Prolog_WriteFile);
	if (pProlog->Set_Memory_Permissions_On_SystemCall(P_PERM_OFFSET, PAGE_EXECUTE_READWRITE))
	{
		if (pProlog->Verify_Signature(P_PLACEMENT_OFFSET))
		{
			pProlog->Save_Original_Bytes(P_ORG_BYTE_OFFSET);
			pProlog->Detour_CafeBabe_Fixup(P_CAFEBABE_FIXUP_OFFSET);
			pProlog->Setup_Landing_Point(P_LANDINGPOINT_OFFSET, NULL);

			if (pProlog->Set_Memory_Permissions_On_StolenBytes(PAGE_EXECUTE_READWRITE))
			{
				pProlog->Save_Stolen_Bytes();
				pProlog->Setup_StolenByte_Detour_Patch("\x48\xB8\xDE\xC0\xBE\xBA\xFE\xCA\xED\xFE\x50\xC3\x90", P_RTN_PATCH_SIZE, P_RTN_PATCH_OFFSET); // Technique: mov rax, push rax, ret
				pProlog->StolenByte_CafeBabe_Fixup(P_RTN_CAFEBABE_FIXUP_OFFSET);
				Fixup_Prolog_ASM_Return = pProlog->Get_Return_Address_To_Original(); // Setup Return Home For ASM File.
#if DEBUG_PROLOG_HOOK
				Log("Fixup_Prolog_ASM_Return: %016llX\n", Fixup_Prolog_ASM_Return);
				DebugBreak(); // Check Everything Is Correct.
#endif												
				pProlog->Activate_Detour(); // Activate Hook
			
			} else {
#if DEBUG_PROLOG_HOOK
				Log("Stolen Byte Permission Failed!\n");
#endif
				return 0; // :( Error
			}
		}
		else {
#if DEBUG_PROLOG_HOOK
			Log("Signature Verification Failed!\n");
#endif
			return 0; // :( Error
		}
	}
	else {
#if DEBUG_PROLOG_HOOK
		Log("System Call Permissions Failed!\n");
#endif
		return 0; // :( Error
	}
	//delete pProlog; // Holds Our Patch Work. If you free it. The hook won't work.

	return 1; // :) Good
}


BOOL WriteFile_Epilog_Hook()
{
	// Setup WriteFile() Epilog Hook - To Filter Return Values.
	BLACKMAGIC_MODEL* pEpilog = new BLACKMAGIC_MODEL();
	pEpilog->Runtime_Dynamic_Linking("WriteFile", "Kernel32.dll");
	pEpilog->Define_Original_Signature(0xD, "\xE8\x0E\xFC\xFF\xFF\x48\x83\xC4\x30\x5B\xC3\x90\x90", "x????xxxxxxxx");
	pEpilog->Setup_Detour_Patch(0x3B, 0xD, "\x48\xB8\xDE\xC0\xBE\xBA\xFE\xCA\xED\xFE\x50\xC3\x90"); // Technique: mov rax, push rax, ret
	pEpilog->Assign_Detour((BYTE*)Epilog_WriteFile);
	if (pEpilog->Set_Memory_Permissions_On_SystemCall(0x3B, PAGE_EXECUTE_READWRITE))
	{
		if (pEpilog->Verify_Signature(0x31))
		{
			pEpilog->Save_Original_Bytes(0x3B);
			pEpilog->Detour_CafeBabe_Fixup(0x2); // Offset 2
			
			if (pEpilog->Set_Memory_Permissions_On_StolenBytes(PAGE_EXECUTE_READWRITE))
			{
				pEpilog->Save_Stolen_Bytes();
				pEpilog->Setup_StolenByte_Detour_Patch("\xC3\x90", 0x2, 0x0); // Technique: ret, nop				
				Fixup_Epilog_ASM_Return = pEpilog->Get_Return_Address_To_Original(); // Setup Return Home For ASM File.
#if DEBUG_EPILOG_HOOK
				Log("Fixup_Epilog_ASM_Return: %016llX\n", Fixup_Epilog_ASM_Return);
				DebugBreak(); // Check Everything Is Correct, Replace with NOP and Run To See If It Works.
#endif
				pEpilog->Activate_Detour(); // Activate Hook

			} else {
#if DEBUG_EPILOG_HOOK
				Log("Stolen Byte Permission Failed!\n");
#endif
				return 0; // :( Error
			}

		} else {
#if DEBUG_EPILOG_HOOK
			Log("Signature Verification Failed!\n");
#endif
			return 0; // :( Error
		}

	} else {
#if DEBUG_EPILOG_HOOK
		Log("System Call Permissions Failed!\n");
#endif
		return 0; // :( Error
	}
	// delete pEpilog; // Holds Our Patch Work. If you free it. The hook won't work.

	return 1; // :) Good
}

