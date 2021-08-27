// Because using InetPton requires converting to PCWR string which seem impossible.
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <cstdarg>
#include <stdio.h>
#include <string>

#include <GodotGlobal.hpp>
#include <PoolArrays.hpp>

#include "WinSocket.hpp"

#define RECEIVE_FLAGS 0
#define SEND_FLAGS 0

void WinSocket::_register_methods()
{
	//register_method("_process", &WinSocket::_process);
	godot::register_method("_init", &WinSocket::_init);
	godot::register_method("connect_to_host", &WinSocket::connect_to_host);
	godot::register_method("set_message_header_size", &WinSocket::set_message_header_size);
	godot::register_method("set_message_buffer", &WinSocket::set_message_buffer);

	godot::register_method("blocking_receive_message", &WinSocket::blocking_receive_message);
	
	godot::register_property<WinSocket, bool>("debug", &WinSocket::debug, false);
}

void WinSocket::_init()
{
	printf("=== WinSock Initializing ===\n");
	int error = init_winsocket();
	winSocket = INVALID_SOCKET;

	if (error > 0)
	{
		printf("Failed to initialize Winsock2.\n");
		return;
	}

	printf("Successfully initialized Winsock2.\n");
}

void WinSocket::_process(float delta)
{
}

void WinSocket::connect_to_host(godot::String hostName, int port)
{
	struct sockaddr_in serverAddr;
	struct hostent* he;

	if ((winSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d", WSAGetLastError());
	}

	if ((he = gethostbyname((const char*) hostName.ascii().get_data())) == NULL)
	{
		//gethostbyname failed
		printf("gethostbyname failed : %d", WSAGetLastError());
		return;
	}

	//Cast the h_addr_list to in_addr , since h_addr_list also has the ip address in long format only
	struct in_addr** addr_list;
	addr_list = (struct in_addr**)he->h_addr_list;
	char ip[255];

	for (int i = 0; addr_list[i] != NULL; i++)
	{
		//Return the first one;
		strcpy(ip, inet_ntoa(*addr_list[i]));
	}

	printf("%s resolved to : %s\n", hostName, ip);

	serverAddr.sin_addr.s_addr = inet_addr(ip);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);

	//Connect to remote server
	int connectError = ::connect(winSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

	if (connectError < 0)
	{
		printf("Socket connect failed with error: %ld\n", WSAGetLastError());
	}

	printf("Socket successfully connected to: %s %d.\n", ip, port);
}

void WinSocket::set_blocking(bool blocking)
{
	if (winSocket == INVALID_SOCKET) {
		// return error code;
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
		godot::Godot::print("set_header error: Size must be 2 or 4 bytes.\n");
		return;
	}

	if (headerSize > 0) {
		::free(headerBuffer);
	}

	headerSize = size;

	if (size > 0) {
		headerBuffer = (byte*)malloc(headerSize);
	}
}

void WinSocket::set_message_buffer(godot::Ref<godot::StreamPeerBuffer> messageBuffer)
{
	this->messageBuffer = messageBuffer.ptr();
}

void WinSocket::debug_print(const char* output)
{
	if (!debug) return;

	//Godot::print(output);
	printf(output);
}

int WinSocket::init_winsocket()
{
	WSADATA wsaData;
	WORD versionRequested = MAKEWORD(2, 2);

	int error = WSAStartup(versionRequested, &wsaData);

	return error;
}

int WinSocket::non_blocking_receive_message()
{
	if (headerSize == 0) {
		return 0;
	}

	if (headerBufferIndex == 0)
	{
		bytesLeft = headerSize;
	}

	if (headerBufferIndex < headerSize) {
		// if we just call receieve, when we do not need two seperate methods for blocking/non-blockign receive message.
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
		}
	}
	else {
		/*
		int numBytesReceived = non_blocking_receive(dataBuffer, &messageBufferIndex, bytesLeft);
		bytesLeft -= numBytesReceived;

		if (bytesLeft == 0) {
			int messageSize = messageBufferIndex;
			headerBufferIndex = 0;
			messageBufferIndex = 0;
			return messageSize;
		}
		*/
	}

	return 0;
}

void WinSocket::blocking_receive_message()
{
	int messageSize = 0;

	if (headerSize == 2)
	{
		blocking_receive((byte*) &headerBuffer, sizeof(UINT16));
		messageSize = ntohs((UINT16) headerBuffer);
		printf("Received header containg message size: %d.\n", messageSize);
	}
	else if (headerSize == 4)
	{
		blocking_receive((byte*) &headerBuffer, sizeof(UINT32));
		messageSize = (UINT32) headerBuffer;
	}
	else {
		// Return error code, receive message when header size not set is invalid behaviour.
	}

	messageBuffer->seek(0);
	godot::PoolByteArray* poolByteArray = &messageBuffer->get_data_array();
	byte* tmpBuffer = (byte*) malloc(messageSize);
	blocking_receive(tmpBuffer, messageSize);
	for (int i = 0; i < messageSize; i++) {
		poolByteArray->set(i, tmpBuffer[i]);
	}
	::free(tmpBuffer);
}

void WinSocket::blocking_receive(byte* buffer, int numBytes)
{
	int bytesLeft = numBytes;
	while (bytesLeft > 0) {
		int numBytesReceived = recv(winSocket, (char*)buffer, bytesLeft, RECEIVE_FLAGS);
		bytesLeft -= numBytesReceived;
	}
}

int WinSocket::non_blocking_receive(byte* buffer, unsigned int* bufferIndex, int numBytes)
{
	int numBytesReceived = recv(winSocket, (char*)buffer + *bufferIndex, numBytes, RECEIVE_FLAGS);
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

void debug_print(const char* output, int debugMode)
{
	if (debugMode == WIN_SOCKET_DEBUG_MODE_NONE) return;

	if (debugMode == WIN_SOCKET_DEBUG_MODE_GODOT)
		godot::Godot::print(output);

	if (debugMode == WIN_SOCKET_DEBUG_MODE_STDOUT)
		printf(output);
}
