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
	struct hostent* he;

	if ((winSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		int wsaError = WSAGetLastError();
		char errorMessage[64];
		sprintf(errorMessage, "WinSocket.connect_to_host: Could not create socket: %d!\n", wsaError);
		debug_print(errorMessage);
	}

	if ((he = gethostbyname((const char*) hostName.ascii().get_data())) == NULL)
	{
		int wsaError = WSAGetLastError();
		return wsaError;
	}

	struct in_addr** addr_list;
	addr_list = (struct in_addr**)he->h_addr_list;
	char ip[255];

	for (int i = 0; addr_list[i] != NULL; i++)
	{
		strcpy(ip, inet_ntoa(*addr_list[i]));
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
		::free(headerBuffer);
	}

	headerSize = size;

	if (size > 0) {
		headerBuffer = (byte*)malloc(headerSize);
	}
}

void WinSocket::set_message_buffer(godot::Ref<godot::StreamPeerBuffer> messageBufferRef)
{
	messageBuffer = messageBufferRef.ptr();
}

void WinSocket::set_receive_buffer(unsigned size)
{
	if(receiveBuffer != NULL) {
		::free(receiveBuffer);
	}

	receiveBuffer = (byte*) malloc(size);
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
	if (headerSize == 0) {
		return 0;
	}

	if (headerBufferIndex == 0)
	{
		bytesLeft = headerSize;
	}

	if(isBlocking)
	{
		blocking_receive_header();
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
		blocking_receive((byte*) &headerBuffer, sizeof(UINT16));
		bytesLeft = ntohs((UINT16) headerBuffer);
	}
	else if (headerSize == 4)
	{
		blocking_receive((byte*) &headerBuffer, sizeof(UINT32));
		bytesLeft = (UINT32) headerBuffer;
	}
}

int WinSocket::blocking_receive_message()
{
	byte* tmpBuffer = (byte*) malloc(bytesLeft);
	int numBytesReceived = blocking_receive(tmpBuffer, bytesLeft);
	fill_message_buffer(tmpBuffer, bytesLeft);
	::free(tmpBuffer);
	return numBytesReceived;
}

int WinSocket::blocking_receive(byte* buffer, int numBytes)
{
	int bytesLeft = numBytes;
	while (bytesLeft > 0) {
		int numBytesReceived = recv(winSocket, (char*)buffer, bytesLeft, RECEIVE_FLAGS);
		bytesLeft -= numBytesReceived;
	}

	return numBytes;
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
	if(receiveBuffer == NULL)
	{
		receiveBuffer = (byte*) malloc(WIN_SOCKET_DEFAULT_BUFFER_SIZE);
	}

	godot::StreamPeerBuffer* streamPeerBuffer = streamPeerBufferRef.ptr();
	godot::PoolByteArray* poolByteArray = &streamPeerBuffer->get_data_array();

	int numBytesReceived = recv(winSocket, (char*) receiveBuffer, numBytes, RECEIVE_FLAGS);
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
			poolByteArray->set(i + bufferIndex, receiveBuffer[i]);
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

short WinSocket::ntohs(short var)
{
	return(::ntohs(var));
}

int WinSocket::htonl(int var)
{
	return(::htonl(var));
}


void debug_print(const char* output, int debugMode)
{
	if (debugMode == WIN_SOCKET_DEBUG_MODE_NONE) return;

	if (debugMode == WIN_SOCKET_DEBUG_MODE_GODOT)
		godot::Godot::print(output);

	if (debugMode == WIN_SOCKET_DEBUG_MODE_STDOUT)
		printf(output);
}
