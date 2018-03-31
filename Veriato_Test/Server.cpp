#include "Server.h"
#include "Network.h"
#include "Log.h"
#include <stdio.h>


DWORD WINAPI Server(_In_ LPVOID lpParameter)
{	
	SERVER_MODEL* Server = new SERVER_MODEL();	

	/**************
	* Setup Server
	***************/
	Server->Init_Winsock();
	Server->CreateListenSocket("127.0.0.1", LOCAL_PORT);
	Server->ListenToSocket();
	Server->SetListenNonBlocking(1);

	/*********************
	* Service Connections
	**********************/
	Service_Connections(Server);
	
	delete Server;
	Log("\n[Server]: Quit Successfully...\n");
	return 0; // :) Good
}


int Service_Connections(SERVER_MODEL* Server)
{
	DWORD dwLastError = 0;
	int iReadyHandles = 0, iMaxfd = 0, iNewfd, iResult = 0;
	SOCKET Listener;
	fd_set ReadSet, MasterSet;
	int iSetIndex = 0;
	
	// Zero Sets
	memset(&ReadSet, 0x0, sizeof(fd_set));
	memset(&MasterSet, 0x0, sizeof(fd_set));

	// Get Listen Socket
	Listener = Server->GetListenSocket();

	// Add listener to MasterSet
	FD_SET(Listener, &MasterSet);
	iMaxfd = (int)Listener; // Track File Descriptors
	
	// Main Server Loop	
	while (true)
	{	
		ReadSet = MasterSet; // Copy MasterSet
						
		/****************************************
		* Select Total Number Of Sockets To Read.
		*****************************************/
		iReadyHandles = select(iMaxfd + 1, &ReadSet, NULL, NULL, NULL);
		if (iReadyHandles < 0)
		{
			dwLastError = WSAGetLastError();
			switch (dwLastError) 
			{
				// We Can Add Other Error Cases Later As They Arise. 
				case SOCKET_ERROR: Log("[Service_Connections]: [select] Socket Error: [%d]\n", dwLastError); break;				
			}
			return -1; // :( Error
		} 
		
		/**********************************************************
		* Processing Connections, Looking For Data To Read or Send.
		***********************************************************/
		for (iSetIndex = 0; iSetIndex <= iMaxfd; iSetIndex++)
		{
			if (FD_ISSET(iSetIndex, &ReadSet)) // Socket In Read State
			{
				if (iSetIndex == Listener) // Check Socket For Listen State
				{
					/***************************
					* Accepting New Connections
					****************************/
					iNewfd = (int)Accept_New_Connections(Listener);
					if (iNewfd == INVALID_SOCKET) {
						Log("[Service_Connections]: Waiting For New Clients, Failed...\n");					
					
					} else {
						FD_SET(iNewfd, &MasterSet); // Add New Client To MasterSet.
						if (iNewfd > iMaxfd) {
							iMaxfd = iNewfd; // Keep Track Of New Connections.
						}
						Log("[Service_Connections]: New Client Connection, Socket %d !!!\n", iNewfd);
					}
				
				} else {
					/******************************
					* Read Client's Data and Store
					*******************************/
					iResult = Interceptor(iSetIndex, iReadyHandles);
					if (iResult == -1){
						Log("[Service_Connections]: Error, Socket %d\n", iSetIndex);
						closesocket(iSetIndex);
						FD_CLR(iSetIndex, &MasterSet);
					} else {
						Log("[Service_Connections]: Done Processing Socket %d\n", iSetIndex);
						closesocket(iSetIndex);
						FD_CLR(iSetIndex, &MasterSet);
					}
				} // Check For Listener
			} // Read State
		} // For Loop, Index Search
	} // Main Server Loop

	return 0; // :) Good
}


SOCKET Accept_New_Connections(SOCKET descriptor)
{
	SOCKET newsock;
	DWORD dwLastError = 0;
	ULONG block = 1;

	// Accept New Clients
	newsock = accept(descriptor, NULL, NULL);
	if (newsock == INVALID_SOCKET)
	{
		dwLastError = WSAGetLastError();
		if (dwLastError != WSAEWOULDBLOCK)
		{
			switch (dwLastError)
			{
				case WSAECONNABORTED: Log("[Accept_New_Connections]: [accept] Connection Aborted.\n");  break;
				case WSAECONNRESET:   Log("[Accept_New_Connections]: [accept] Connection Reset.\n");    break;
				case WSAESHUTDOWN:    Log("[Accept_New_Connections]: [accept] Connection Shutdown.\n"); break;
				case WSAENOTSOCK:     Log("[Accept_New_Connections]: [accept] Not A Socket.\n");        break;
				case WSAEFAULT:       Log("[Accept_New_Connections]: [accept] Invalid Pointer Address.\n");      break;
				default: Log("[Accept_New_Connections]: [accept] Failed, Error #: %d!\n", dwLastError); break;
			}
		}
		return INVALID_SOCKET; // :( Error
	}

	// Enable Non-Blocking Mode On New Client
	if (SOCKET_ERROR == ioctlsocket(newsock, FIONBIO, &block)) {
		Log("[Accept_New_Connections]: ioctlsocket(FIONBIO) Nonblocking Mode Error %d\n", WSAGetLastError());
	}

	return newsock; // :) Good
}


int Interceptor(SOCKET client, int iReadyHandles)
{
	char* pTrashCan = new char[BUFSIZE](); // Value-Initialisation, Was Introduced In C++03.
	int iCheck = 0, iDataLen = 0;
	PPACKET Package = NULL;	
	DWORD size = 0;

	iReadyHandles--;

	if (client != INVALID_SOCKET)
	{
		// Receive As Much Junk As Possible
		iCheck = Recv(client, pTrashCan, &iDataLen);
		if (iCheck == -1) {				
			return -1; // :( Connection Error. Also For No Data.
		}

		// Assign Buffer To Package
		Package = (PPACKET)pTrashCan;
		
		if (strlen(Package->Path) != 0)
		{
			// Display & Store Location
			fprintf_s(stdout, "[%s][%s]   [Interceptor]: Location %s\n", __DATE__, __TIME__, Package->Path);
			Log("[%s][%s]   [Interceptor]: Location %s\n", __DATE__, __TIME__, Package->Path);
		}

		if (Package->FileSize.QuadPart != 0)
		{
			// Display & Store File Size
			fprintf_s(stdout, "[%s][%s]   [Interceptor]: File Size is %d Bytes\n", __DATE__, __TIME__, Package->FileSize.QuadPart);
			Log("[%s][%s]   [Interceptor]: File Size is %d Bytes\n", __DATE__, __TIME__, Package->FileSize.QuadPart);
		}
				
		delete[] pTrashCan;
		pTrashCan = NULL;
		Package = NULL;		
		return 0; // :) Good
	}
		
	delete[] pTrashCan;
	pTrashCan = NULL;	
	return -1; // :( Error
}


int Recv(_In_ SOCKET sock, _In_ char* pTrashCan, _In_ int* iTrashLength)
{
	char* pChunk = new char[BUFSIZE](); // Value-Initialisation, Was Introduced In C++03.
	int iLastError = 0, iFlags = 0;
	int nBytesReceived = 0;	
	
	/* Receiving Loop */
	do
	{
		nBytesReceived = recv(sock, pChunk, BUFSIZE, iFlags);
		if (nBytesReceived > 0 && nBytesReceived != 0 && nBytesReceived != -1 && *iTrashLength != -1)
		{				
			memcpy(pTrashCan + *iTrashLength, pChunk, nBytesReceived);
		}
		else
			break; // Break Out Of Receiving Loop
		*iTrashLength += nBytesReceived;			
	} while (1);

	/* Problem If No Data Present */
	if (*iTrashLength <= 0)
	{
		iLastError = WSAGetLastError();
		if (iLastError != WSAEWOULDBLOCK)
		{
			switch (iLastError)
			{
				case WSAECONNABORTED: Log("[Recv]: Connection Aborted.\n");          break;
				case WSAECONNRESET:	  Log("[Recv]: Connection Reset.\n");            break;
				case WSAESHUTDOWN:    Log("[Recv]: Connection Shutdown.\n");         break;
				case WSAENOTSOCK:     Log("[Recv]: Not A Socket.\n");                break;
				default: Log("[Recv]: No Data On Line, Error #: %d!\n", iLastError); break;
			}
		}

		/* Connection Error */
		delete[] pChunk;
		pChunk = NULL;
		return -1; // :( Error
	}

	/* Connection Is Good */
	delete[] pChunk;
	pChunk = NULL;
	return 0; // :) Good
}

