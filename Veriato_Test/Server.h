#ifndef SERVER_H
#define SERVER_H

#pragma once

#include <winsock2.h>
#include "Network.h"

#define LOCAL_PORT 55555
#define BUFSIZE 65545 // 1 Kilobyte

// Packet
typedef struct tag_packet{
	DWORD  PacketLen;
	CHAR   Path[MAX_PATH + 1];
	size_t PathLen;
	LARGE_INTEGER FileSize;
} PACKET, *PPACKET;

// Server Functionality
DWORD WINAPI Server(_In_ LPVOID lpParameter);
SOCKET Accept_New_Connections(SOCKET descriptor);
int Service_Connections(SERVER_MODEL* Server);
int Interceptor(SOCKET client);
int Recv(_In_ SOCKET sock, _In_ char* pTrashCan, _In_ int* iTrashLength);

#endif

