#include "Server.h"
#include "Network.h"
#include "Log.h"
#include <stdio.h>


DWORD WINAPI Server(_In_ LPVOID lpParameter)
{	
	SOCKET Listen, Client;	
	Server_Model* Server = new Server_Model();

	/************
	* ( Server )
	*************/
	Server->CreateListenSocket("127.0.0.1", LOCAL_PORT);
	Server->ListenToSocket();
	Server->SetListenNonBlocking(1);

	while (true) 
	{
		/**************************************
		* Listen For New Incoming Connections 
		***************************************/		
		Listen = Server->GetSocket();
		if ((Client = wait_for_connections(Listen)) != INVALID_SOCKET)
		{			
			/*********************
			* Service Connections
			**********************/			
			service_connections(Client);
		}
	}

	return 0; // :) Good
}


SOCKET wait_for_connections(SOCKET descriptor)
{
	SOCKET newsock;	
	DWORD dwLastError = 0;	
	ULONG block  = 1;

	newsock = accept(descriptor, NULL, NULL);
	if (newsock == INVALID_SOCKET)
	{
		dwLastError = WSAGetLastError();
		if (dwLastError != WSAEWOULDBLOCK)
		{
			switch (dwLastError) 
			{
				case WSAECONNABORTED: Log("[wait_for_connections]: [accept] Connection Aborted.\n");  break;
				case WSAECONNRESET:	  Log("[wait_for_connections]: [accept] Connection Reset.\n");    break;
				case WSAESHUTDOWN:    Log("[wait_for_connections]: [accept] Connection Shutdown.\n"); break;
				case WSAENOTSOCK:     Log("[wait_for_connections]: [accept] Not A Socket.\n");        break;
				case WSAEFAULT:       Log("[wait_for_connections]: Invalid Pointer Address.\n");      break;
				default: Log("[wait_for_connections]: [accept] Failed, Error #: %d!\n", dwLastError); break;
			}			
		}
		return INVALID_SOCKET; // :( Error
	}
		
	Log("[wait_for_connections]: Client Connection Found !!!\n");

	if (SOCKET_ERROR == ioctlsocket(newsock, FIONBIO, &block)) {
		Log("[wait_for_connections]: ioctlsocket(FIONBIO) Error %d\n", WSAGetLastError());
	}

	return newsock; // :) Good
}


int service_connections(SOCKET client)
{
	DWORD dwLastError = 0;
	int iReadySocketHandles = 0;
	SOCKET maxfd;		
	fd_set ReadSet;	
	
	// Check Our Client Socket
	if (client == INVALID_SOCKET) {
		return -1; // :( Error
	}

	// Socket Descriptors
	maxfd = client;
	maxfd++;
		
	while (true)
	{	
		/******************************
		* Check Which Socket To Read
		*******************************/
		FD_ZERO(&ReadSet);		
		FD_SET(client, &ReadSet);
						
		/*****************************************
		* Select Total Number Of Sockets To Read
		******************************************/
		iReadySocketHandles = select(maxfd + 1, &ReadSet, NULL, NULL, NULL);
		if (iReadySocketHandles < 0) 
		{
			dwLastError = WSAGetLastError();
			switch (dwLastError) 
			{
				// We Can Add Other Error Cases Later As They Arise. 
				case SOCKET_ERROR: Log("[service_connections]: [select] Socket Error: [%d]\n", dwLastError); break;				
			}
			return -1; // :( Error, Exit on 10038. Normal Processing. Return Control To Server().
		} 
		
		/******************************
		* Read Client's Data and Store
		*******************************/		
		if (FD_ISSET(client, &ReadSet))
		{			
			Interceptor(client, iReadySocketHandles);			
		} 		
	}

	return 0; // :) Good
}


int Interceptor(SOCKET client, int iReadySocketHandles)
{
	char* pTrashCan = new char[BUFSIZE](); // Value-Initialisation, Was Introduced In C++03.
	int iCheck = 0, iDataLen = 0;
	PPACKET Package = NULL;	
	DWORD size = 0;

	iReadySocketHandles--;

	if (client != INVALID_SOCKET)
	{
		// Receive As Much Junk As Possible
		iCheck = Recv(client, pTrashCan, &iDataLen);
		if (iCheck == -1) {
			closesocket(client); // Done Processing Client.		
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
		closesocket(client); // Done Processing Client.
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

