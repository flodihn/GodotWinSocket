#define _WINSOCK_DEPRECATED_NO_WARNINGS
/*************************************************************************/
/*  WinSocket.hpp  		                                                 */
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

#ifndef GODOT_WIN_SOCKET_H
#define GODOT_WIN_SOCKET_H

#ifndef _WIN32_WINNT
#define  _WIN32_WINNT
#endif

#include <stdio.h>
#include <boost/asio.hpp>

#include <Godot.hpp>
#include <Node.hpp>
#include <StreamPeerBuffer.hpp>


class WinSocket : public godot::Node {
	GODOT_CLASS(WinSocket, godot::Node)

private:
	//SOCKET winSocket = INVALID_SOCKET;
	//WSABUF headerBuffer = {0};
	
	//unsigned int headerSize = 0;
	// Used in non blocking mode to remember how many bytes we are waiting for between calls.
	//unsigned int bytesLeft = 0;
	//unsigned int headerBufferIndex;
	//unsigned int messageBufferIndex;

	UINT16 shortHeader;
	INT32 intHeader;

	godot::StreamPeerBuffer* messageBuffer;
	// tmpMessabeBuffer is allocated and used when receiving messages in non-blocking mode.
	//byte* tmpMessageBuffer = NULL;
	// receiveBuffer is only used when the low level method receive is called instead of the receive_message method.
	//WSABUF receiveBuffer = {0};
	//byte* receiveBuffer = NULL;

	//bool wsaInitialized = false;
	bool debug = false;
	bool isBlocking = true;

	void debug_print(const char * output);
		
	void blocking_receive_header();
	int blocking_receive_message();
	//int blocking_receive(WSABUF* buffer);

	void non_blocking_receive_header();
	int non_blocking_receive_message();
	int non_blocking_receive(byte* dataBuffer, unsigned int* dataBufferIndex, int numBytes);

	void fill_message_buffer(byte* sourceBuffer, int messageSize);
	//void resolve_addr(struct sockaddr_in* serverAddr, const char* hostName, char* ip);

public:
	static void _register_methods();
	void _init();
	void _process(float delta);
	
	int winsock_init();
	void winsock_cleanup();
	
	int connect_to_host(godot::String hostName, int port);
	int secure_connect_to_host(godot::String hostName, int port);

	void disconnect();
	void set_message_header_size(int size);
	void set_message_buffer(godot::Ref<godot::StreamPeerBuffer> messageBufferRef);
	void set_blocking(bool blocking);
	void set_debug(bool debug);

	int receive_message();
	int receive(godot::Ref<godot::StreamPeerBuffer> streamPeerBufferRef, unsigned bufferIndex, unsigned int numBytes);
	void set_receive_buffer(unsigned size);

	void send_message(const char* data);

	short ntohs(short var);
	int htonl(int var);
};

#endif