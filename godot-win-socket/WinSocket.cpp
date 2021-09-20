/*************************************************************************/
/*  WinSocket.cpp  		                                                 */
/*************************************************************************/
/*                       This file is part of:                           */
/*                       GODOT WINSOCK PLUGIN                            */
/*             https://github.com/flodihn/GodotWinSocket                 */
/*************************************************************************/
/* Copyright (c) 2021 Christian Flodihn.				                 */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include <iostream>
#include <cstdarg>
#include <stdio.h>
#include <string>

#include <GodotGlobal.hpp>
#include <PoolArrays.hpp>

#include "WinSocket.hpp"

#define RECEIVE_FLAGS 0
#define SEND_FLAGS 0

/*
 * WinSocket exposes a low level socket implementation based on winsock2.
 * It provides functions to connect to a host and port, receive and send 
 * data.

 * Data can be received by either calling receive or receive_message.

 * receive is the most simple method, requires the user to provide a
 * StreamPeerBuffer, a starting index where to start writing data into
 * the buffer and the number of bytes to receive. Before using this
 * method, the set_receive_buffer method must be called with a size
 * big enough to receive whatever message is expected to be received.

 * receive_message provides a bit more convenience, but requires to
 * set_message_header_size and set_message_buffer must and be called 
 * before the receive_message function.
 * set_header_size must provide a size of either 2 or 4 bytes.
 * When receiving the message, the first amount of bytes is expected 
 * to contain the size of the message in bytes.
 * set_message_buffer expects the user to provide a StreamPeerBuffer
 * which will be used to write the received packet.

 * After connecting to a server by calling connect_to_host, the socket
 * can be set in either blocking(default) or non blocking mode.
 * In blocking mode, the WinSock class will block until the data all
 * data has been received, probably causing whole Godot to freeze.
 * In non blocking mode, the receive and receive_message will always
 * immediately, but might return that 0 bytes was received, then it is
 * up to the user to call the method until a message size, a value
 * larger than 0 is returned.

 * Future improvements:
 * For every message a new buffer is created on the	heap by calling 
 * malloc. It would probably be more efficient by giving the option to 
 * set and use a constant buffer.
 * Also, a platform independent alternative of the plugin by using boost
 * or maybe SDL would be a good idea.
 * Use varargs for debug_print method.
 */
void WinSocket::_register_methods()
{
	godot::register_method("_init", &WinSocket::_init);

	godot::register_method("winsock_init", &WinSocket::winsock_init);
	godot::register_method("winsock_cleanup", &WinSocket::winsock_cleanup);

	godot::register_method("connect_to_host", &WinSocket::connect_to_host);
	godot::register_method("secure_connect_to_host", &WinSocket::secure_connect_to_host);
	godot::register_method("disconnect", &WinSocket::disconnect);

	godot::register_method("set_message_header_size", &WinSocket::set_message_header_size);
	godot::register_method("set_message_buffer", &WinSocket::set_message_buffer);
	godot::register_method("set_blocking", &WinSocket::set_blocking);
	godot::register_method("set_receive_buffer", &WinSocket::set_receive_buffer);
	godot::register_method("set_debug", &WinSocket::set_debug);

	godot::register_method("receive_message", &WinSocket::receive_message);
	godot::register_method("receive", &WinSocket::receive);
	
	godot::register_method("ntohs", &WinSocket::ntohs);
	godot::register_method("htonl", &WinSocket::htonl);
	
	godot::register_property<WinSocket, bool>("debug", &WinSocket::debug, false);
}

void WinSocket::_init()
{
}

int WinSocket::winsock_init()
{
	debug_print("WinSock.winsock_init: Initializing winsock2...\n");
	WSADATA wsaData;
	WORD versionRequested = MAKEWORD(2, 2);

	int error = WSAStartup(versionRequested, &wsaData);
	winSocket = INVALID_SOCKET;

	if (error > 0)
	{
		debug_print("WinSock.winsock_init: Failed to initialize winsock2.\n");
		return error;
	}

	debug_print("WinSock.winsock_init: Successfully initialized winsock2.\n");
	wsaInitialized = true;
	return 0;
}

void WinSocket::winsock_cleanup()
{
	WSACleanup();
}

int WinSocket::connect_to_host(godot::String hostName, int port)
{
	struct sockaddr_in serverAddr;
	const char* hostNameCStr = (const char*) hostName.ascii().get_data();
	
	if ((winSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		int wsaError = WSAGetLastError();
		char errorMessage[64];
		sprintf(errorMessage, "WinSocket.connect_to_host: Could not create socket: %d!\n", wsaError);
		debug_print(errorMessage);
	}

	char ip[255];

	resolve_addr(&serverAddr, hostNameCStr, ip);

	if(ip == NULL)
	{
		int wsaError = WSAGetLastError();
		char errorMessage[64];
		sprintf(errorMessage, "WinSocket.connect_to_host: Failed to resolve host %s with error %d!\n", hostNameCStr, wsaError);
		debug_print(errorMessage);
	}

	serverAddr.sin_addr.s_addr = inet_addr(ip);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);

	int connectError = ::connect(winSocket, (struct sockaddr*) &serverAddr, sizeof(serverAddr));

	if (connectError < 0)
	{
		int wsaError = WSAGetLastError();
		char errorMessage[64];
		sprintf(errorMessage, "WinSocket.connect_to_host: Socket connect failed with error: %d!\n", wsaError);
		debug_print(errorMessage);
		return wsaError;
	}

	char successMessage[64];
	sprintf(successMessage, "WinSocket.connect_to_host: Socket successfully connected to: %s:%d.\n", ip, port);
	debug_print(successMessage);
	return 0;
}

int WinSocket::secure_connect_to_host(godot::String, int port)
{
	struct sockaddr* serverAddr = NULL;
	struct addrinfoW aiHints = {0};
	struct addrinfoW* aiList = NULL;
	SOCKET_SECURITY_SETTINGS securitySettings;
	int iResult = 0;
	int sockErr = 0;
	DWORD flags = 0;
	ULONG settingsLen = 0;
	ULONG serverAddrLen = 0;
	SOCKET_PEER_TARGET_NAME* peerTargetName = NULL;
	
	wchar_t* serverHost = L"localhost";
	wchar_t* serverPort = L"2000";

	securitySettings.SecurityProtocol = SOCKET_SECURITY_PROTOCOL_DEFAULT;
	securitySettings.SecurityFlags = SOCKET_SETTINGS_GUARANTEE_ENCRYPTION;

	aiHints.ai_family = AF_INET;
	aiHints.ai_socktype = SOCK_STREAM;
	aiHints.ai_protocol = IPPROTO_TCP;

    // Set hostname
	DWORD result = GetAddrInfoW(serverHost, serverPort, &aiHints, &aiList);
	serverAddr = aiList->ai_addr;
	serverAddrLen = (ULONG) aiList->ai_addrlen;

    // Create a TCP socket
	winSocket = WSASocket(serverAddr->sa_family, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
	if (winSocket == INVALID_SOCKET) {
		iResult = WSAGetLastError();
		wprintf(L"WSASocket returned error %ld\n", iResult);
		//goto cleanup;
	}

	sockErr = WSASetSocketSecurity(winSocket, &securitySettings, settingsLen, NULL, NULL);
	if (sockErr == SOCKET_ERROR) {
		iResult = WSAGetLastError();
		wprintf(L"WSASetSocketSecurity returned error %ld\n", iResult);
		//goto cleanup;
	}
	
	/*
	peerTargetName = (SOCKET_PEER_TARGET_NAME *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, peerTargetNameLen);
	if (!peerTargetName) {
		iResult = ERROR_NOT_ENOUGH_MEMORY;
		wprintf(L"Out of memory\n");
		//goto cleanup;
	}
	*/

	peerTargetName->SecurityProtocol = securitySettings->SecurityProtocol;
	// Specify the server SPN 
	peerTargetName->PeerTargetNameStringLen = serverSpnStringLen;

	/*
	sockErr = WSASetSocketPeerTargetName(winSocket, peerTargetName, peerTargetNameLen, NULL, NULL);
	if (sockErr == SOCKET_ERROR) {
		iResult = WSAGetLastError();
		wprintf(L"WSASetSocketPeerTargetName returned error %ld\n", iResult);
		//goto cleanup;
	}
	*/

	sockErr = WSAConnect(winSocket, serverAddr, serverAddrLen, NULL, NULL, NULL, NULL);
	if (sockErr == SOCKET_ERROR) {
		iResult = WSAGetLastError();
		wprintf(L"WSAConnect returned error %ld\n", iResult);
		//goto cleanup;
	}
	
	// At this point a secure connection must have been established.
	wprintf(L"Secure connection established to the server\n");

    DWORD bytesRecvd = 0;
    WSABUF wsaBuf = { 0 };
    const int RECV_DATA_BUF_SIZE = 5;
    char recvBuf[RECV_DATA_BUF_SIZE] = { 0 };

    wsaBuf.len = RECV_DATA_BUF_SIZE;
    wsaBuf.buf = recvBuf;
    sockErr = WSARecv(winSocket, &wsaBuf, 1, &bytesRecvd, &flags, NULL, NULL);
    if (sockErr == SOCKET_ERROR) {
        iResult = WSAGetLastError();
        wprintf(L"WSARecv returned error %ld\n", iResult);
    }
    wprintf(L"Received %d bytes of data from the server\n", bytesRecvd);

	return 0;
}

void WinSocket::disconnect()
{
	int result = shutdown(winSocket, SD_SEND);
	if (result == SOCKET_ERROR) {
		int wsaError = WSAGetLastError();
		char errorMessage[64];
		sprintf(errorMessage, "WinSocket.disconnect: Clean shutdown failed, forcing disconnect: %d!\n", wsaError);
		debug_print(errorMessage);
	}
	closesocket(winSocket);
}

void WinSocket::set_blocking(bool blocking)
{
	if (winSocket == INVALID_SOCKET) {
		return;
	}

	// 0 for blocking, 1 for non blocking;
	unsigned long ioctl_blocking_mode;

	if (blocking)
	{
		ioctl_blocking_mode = 0;
		ioctlsocket(winSocket, FIONBIO, (unsigned long*) &ioctl_blocking_mode);
	}
	else {
		ioctl_blocking_mode = 1;
		ioctlsocket(winSocket, FIONBIO, (unsigned long*) &ioctl_blocking_mode);
	}

	isBlocking = blocking;
}

void WinSocket::set_message_header_size(int size)
{
	if (!(size == 2 || size == 4)) {
		debug_print("WinSocket.set_header error: Size must be 2 or 4 bytes.\n");
		return;
	}

	if (headerSize > 0) {
		::free(headerBuffer.buf);
	}

	headerSize = size;
	headerBuffer.len = headerSize;

	if (size > 0) {
		headerBuffer.buf = (char*) malloc(headerSize);
	}
}

void WinSocket::set_message_buffer(godot::Ref<godot::StreamPeerBuffer> messageBufferRef)
{
	messageBuffer = messageBufferRef.ptr();
}

void WinSocket::set_receive_buffer(unsigned size)
{
	if(receiveBuffer.buf != NULL) {
		::free(receiveBuffer.buf);
	}

	receiveBuffer.buf = (char*) malloc(size);
	receiveBuffer.len = size;
}

void WinSocket::set_debug(bool debug)
{
	this->debug = debug;
}

void WinSocket::debug_print(const char* output)
{
	if (!debug) return;
	godot::Godot::print(output);
}

int WinSocket::receive_message()
{
	wprintf(L"Receive message...\n");
	if (headerSize == 0) {
		return 0;
	}

	if (headerBufferIndex == 0)
	{
		bytesLeft = headerSize;
	}

	if(isBlocking)
	{
		wprintf(L"Receive message header\n");
		blocking_receive_header();
		wprintf(L"Received full header\n");
		return blocking_receive_message();
	} else {
		if (headerBufferIndex < headerSize) {
			non_blocking_receive_header();
		}

		return non_blocking_receive_message();
	}
}

void WinSocket::blocking_receive_header()
{
	if (headerSize == 2)
	{
		blocking_receive(&headerBuffer);
		bytesLeft = ntohs((UINT16) headerBuffer.buf);
	}
	else if (headerSize == 4)
	{
		blocking_receive(&headerBuffer);
		bytesLeft = (UINT32) headerBuffer.buf;
	}
}

int WinSocket::blocking_receive_message()
{
	wprintf(L"blocking_receive_message\n");
	WSABUF tmpBuffer;
	tmpBuffer.buf = (char*) malloc(bytesLeft);
	tmpBuffer.len = bytesLeft;

	wprintf(L"created tmpBuffer\n");
	int numBytesReceived = blocking_receive(&tmpBuffer);
	wprintf(L"receved full message\n");
	fill_message_buffer((byte*) tmpBuffer.buf, bytesLeft);
	wprintf(L"filled message buffer\n");
	::free(tmpBuffer.buf);
	wprintf(L"freeing tmpBuffer\n");
	return numBytesReceived;
}

int WinSocket::blocking_receive(WSABUF* buffer)
{
	DWORD numBytesReceived = 0;
	ULONG bytesLeft = buffer->len;
		
	DWORD lpFlags = MSG_WAITALL;
	while(bytesLeft > 0) {
		WSARecv(winSocket, buffer, 1, &numBytesReceived, &lpFlags, NULL, NULL);
		bytesLeft -= numBytesReceived;

		wprintf(L"WSARecv numBytesReceived: %ld\n", numBytesReceived);
		wprintf(L"WSARecv bytesLeft: %d\n", bytesLeft);
	}
	
	return (int) numBytesReceived;
}

void WinSocket::non_blocking_receive_header()
{
	int numBytesReceived = 0;

	if (headerSize == 2) {
		numBytesReceived = non_blocking_receive((byte*) &shortHeader, &headerBufferIndex, bytesLeft);
	}
	else if (headerSize == 4)
	{
		numBytesReceived = non_blocking_receive((byte*) &intHeader, &headerBufferIndex, bytesLeft);
	}

	bytesLeft -= numBytesReceived;

	if (headerBufferIndex == headerSize)
	{
		if (headerSize == 2) {
			bytesLeft = ntohs(shortHeader);
		} else if(headerSize == 4) {
			bytesLeft = htonl(intHeader);
		}

		if(tmpMessageBuffer != NULL) {
			::free(tmpMessageBuffer);
		}

		if(bytesLeft > 0) {
			tmpMessageBuffer = (byte*) malloc(bytesLeft);
		}
	}
}

int WinSocket::non_blocking_receive_message()
{
	int numBytesReceived = non_blocking_receive(tmpMessageBuffer, &messageBufferIndex, bytesLeft);
	bytesLeft -= numBytesReceived;

	if (bytesLeft == 0) {
		int messageSize = messageBufferIndex;
		headerBufferIndex = 0;
		messageBufferIndex = 0;

		fill_message_buffer(tmpMessageBuffer, messageSize);
		::free(tmpMessageBuffer);
		tmpMessageBuffer = NULL;
		return messageSize;
	}

	return 0;
}

int WinSocket::non_blocking_receive(byte* buffer, unsigned int* bufferIndex, int numBytes)
{
	int numBytesReceived = recv(winSocket, (char*)buffer + *bufferIndex, numBytes, SEND_FLAGS);
	if (numBytesReceived == SOCKET_ERROR) {
		int error = WSAGetLastError();
		if (error == WSAEWOULDBLOCK) {
			return 0;
		}
	}
	else {
		*bufferIndex += numBytesReceived;
		return numBytesReceived;
	}

	return 0;
}

void WinSocket::fill_message_buffer(byte* sourceBuffer, int numBytes)
{
	messageBuffer->seek(0);
	godot::PoolByteArray* poolByteArray = &messageBuffer->get_data_array();
	for (int i = 0; i < numBytes; i++) {
		poolByteArray->set(i, sourceBuffer[i]);
	}
}

int WinSocket::receive(godot::Ref<godot::StreamPeerBuffer> streamPeerBufferRef, unsigned bufferIndex, unsigned int numBytes)
{
	if(receiveBuffer.buf == NULL)
	{
		receiveBuffer.buf = (char*) malloc(WIN_SOCKET_DEFAULT_BUFFER_SIZE);
		receiveBuffer.len = WIN_SOCKET_DEFAULT_BUFFER_SIZE;
	}

	godot::StreamPeerBuffer* streamPeerBuffer = streamPeerBufferRef.ptr();
	godot::PoolByteArray* poolByteArray = &streamPeerBuffer->get_data_array();

	int numBytesReceived = recv(winSocket, receiveBuffer.buf, numBytes, RECEIVE_FLAGS);
	if (numBytesReceived == SOCKET_ERROR) {
		int wsaError = WSAGetLastError();

		if(isBlocking && wsaError == WSAEWOULDBLOCK) {
			return 0;
		}

		char errorMessage[32];
		sprintf(errorMessage, "WinSocket.receive: WSA error: %d!\n", wsaError);
		debug_print(errorMessage);
		return SOCKET_ERROR;
	}

	if(numBytesReceived > 0) {
		for (int i = 0; i < numBytesReceived; i++) {
			poolByteArray->set(i + bufferIndex, receiveBuffer.buf[i]);
		}
	}

	char errorMessage[64];
	sprintf(errorMessage, "WinSocket.receive: Successfully received: %d bytes.\n", numBytesReceived);
	debug_print(errorMessage);

	return numBytesReceived;
}

void WinSocket::send_message(const char* message)
{
	if (headerSize == 0) {
		send(winSocket, message, sizeof(message), SEND_FLAGS);
		return;
	}

	if (headerSize == 2) {
		UINT16 header = sizeof(message);
		send(winSocket, (const char*) &header, sizeof(UINT16), SEND_FLAGS);
		send(winSocket, message, sizeof(message), SEND_FLAGS);
	}

	if (headerSize == 4) {
		UINT32 header = sizeof(message);
		send(winSocket, (const char*)&header, sizeof(UINT32), SEND_FLAGS);
		send(winSocket, message, sizeof(message), SEND_FLAGS);
	}
}

void WinSocket::resolve_addr(struct sockaddr_in* serverAddr, const char* hostName, char* ip)
{
	struct hostent* he;

	if ((he = gethostbyname(hostName)) == NULL)
	{
		return;
	}

	struct in_addr** addr_list;
	addr_list = (struct in_addr**) he->h_addr_list;

	for (int i = 0; addr_list[i] != NULL; i++)
	{
		strcpy(ip, inet_ntoa(*addr_list[i]));
	}
}

short WinSocket::ntohs(short var)
{
	return(::ntohs(var));
}

int WinSocket::htonl(int var)
{
	return(::htonl(var));
}