#ifndef NETWORK_H
#define NETWORK_H

#pragma once

#include <winsock2.h>


class Server_Model
{	
	public:
		
		Server_Model();  // Constructor
		~Server_Model(); // Destructor
		
		SOCKET WSAAPI CreateListenSocket(const char *ip, u_short port);
		int ListenToSocket();
				
		// Getters
		SOCKET GetSocket();
		bool GetIsConnected();
		
		// Setters		
		int  SetListenNonBlocking(ULONG block);
		void SetIsConnected(bool connected);
				
	private:
		
		// Listen Socket Information
		SOCKET ListenSocket;
		int iListenAddrLen;
		sockaddr_in ListenAddress;
		bool bIsConnected;
};


#endif

