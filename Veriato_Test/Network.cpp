#include "Network.h"
#include "Log.h"


Server_Model::Server_Model()
{
	// Constructor
	ZeroMemory(&this->ListenAddress, sizeof(this->ListenAddress));
	this->iListenAddrLen = sizeof(this->ListenAddress);
	this->bIsConnected = false;
}


Server_Model::~Server_Model()
{
	// Destructor
}


bool Server_Model::GetIsConnected()
{
	return this->bIsConnected;
}


void Server_Model::SetIsConnected(bool connected)
{
	this->bIsConnected = connected;
	return;
}


int Server_Model::SetListenNonBlocking(ULONG block)
{
	// Set the socket to non-blocking mode so the server will not block.
	if ( SOCKET_ERROR == ioctlsocket(this->ListenSocket, FIONBIO, &block ) ) {
		Log( "[SetListenBlocking]: ioctlsocket(FIONBIO) Error %d\n", WSAGetLastError() );
	} else {
		Log( "[SetListenBlocking]: ioctlsocket(FIONBIO) is OK!\n" );
	}
	return 0;
}


SOCKET Server_Model::GetSocket()
{
	return this->ListenSocket;
}


SOCKET WSAAPI Server_Model::CreateListenSocket(const char *ip, u_short port)
{
	// Create Socket
	this->ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == this->ListenSocket ) {
		Log("[CreateListenSocket]: Error: %d\n", WSAGetLastError());
		return SOCKET_ERROR;
	} else {
		Log("[CreateListenSocket]: Successful.\n");
	}

	// Server Information
	this->ListenAddress.sin_family = AF_INET;
	this->ListenAddress.sin_addr.s_addr = inet_addr(ip);
	this->ListenAddress.sin_port = htons(port);

	// Bind
	if (SOCKET_ERROR == bind(this->ListenSocket, (const sockaddr*)&this->ListenAddress, this->iListenAddrLen)) {
		closesocket(this->ListenSocket);
		Log("[CreateListenSocket]: Error occurred while binding.\n");
		return SOCKET_ERROR;
	} else {
		Log("[CreateListenSocket]: Successful.\n");
	}

	return this->ListenSocket;
}


int Server_Model::ListenToSocket()
{
	int result = SOCKET_ERROR;
	if ( SOCKET_ERROR == listen( this->ListenSocket, SOMAXCONN ) ) {
		closesocket(this->ListenSocket);
		Log("[ListenSocket]: Error occurred while listening.\n");
		return result;
	} else {
		Log("[ListenSocket]: Successful.\n");
	}
	return result;
}

