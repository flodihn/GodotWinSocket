/*************************************************************************/
/*  WinSocket.hpp  		                                                 */
/*************************************************************************/
/*                       This file is part of:                           */
/*                       GODOT WINSOCK PLUGIN                            */
/*             https://github.com/flodihn/GodotWinSocket                 */
/*************************************************************************/
/* Copyright (c) 2021 Christian Flodihn.                                 */
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

#include <stdio.h>

#include <Godot.hpp>
#include <Node.hpp>
#include <StreamPeerBuffer.hpp>

typedef char byte;

#include "SocketWrapper.hpp"

class Socket : public godot::Node {
	GODOT_CLASS(Socket, godot::Node)

private:
	SocketWrapper socketWrapper;

	bool debug = false;
	unsigned int headerSize = 0;

	// Internal buffer for receiving bytes from the socket,
	// will allocated same number of bytes as the godot stream peer buffer.
	char* byte_buffer;

	// External buffer used to send back bytes to Godot when receiving full
	// message with 2 or 4 byte header.
	godot::StreamPeerBuffer* messageBuffer;

	void debug_print(const char * output);
	void fill_message_buffer(char*, int messageSize);
public:
	static void _register_methods();
	void _init();
	void _process(float delta);
	
	int connect_to_host(godot::String hostname, int port);
	void close();

	void set_message_header_size(int size);
	void set_message_buffer(godot::Ref<godot::StreamPeerBuffer> messageBufferRef);
	void set_ssl(bool trueOrFalse);
	void set_blocking(bool trueOrFalse);
	void set_debug(bool trueOrFalse);
	void set_receive_buffer(unsigned size);

	int blocking_receive_message();
	int blocking_receive(int numBytes);

	int send_message(godot::PoolByteArray sendBuffer);
	//int send_bytes(const char* bytes);

	short ntohs(short var);
	int htonl(int var);
};

#endif