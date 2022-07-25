/*************************************************************************/
/*  WinSocket.cpp  		                                                 */
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

#include <iostream>
#include <cstdarg>
#include <stdio.h>
#include <string>

#include <GodotGlobal.hpp>
#include <PoolArrays.hpp>

#include "Socket.hpp"

/*
 * WinSocket exposes a low level socket implementation based on boost.
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

#define RECEIVE_FLAGS 0
#define SEND_FLAGS 0

using std::string;
using std::cout;
using std::endl;

/*
bool verify_certificate(boost::asio::ssl::verify_context& ctx)
{
	// The verify callback can be used to check whether the certificate that is
	// being presented is valid for the peer. For example, RFC 2818 describes
	// the steps involved in doing this for HTTPS. Consult the OpenSSL
	// documentation for more details. Note that the callback is called once
	// for each certificate in the certificate chain, starting from the root
	// certificate authority.

	// In this example we will simply print the certificate's subject name.
	char subject_name[256];
	X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
	X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
	std::cout << "Verifying " << subject_name << "\n";
	return preverified;
	return true;
}
*/

void Socket::_register_methods()
{
	godot::register_method("_init", &Socket::_init);

	godot::register_method("connect_to_host", &Socket::connect_to_host);
	godot::register_method("close", &Socket::close);

	godot::register_method("set_message_header_size", &Socket::set_message_header_size);
	godot::register_method("set_message_buffer", &Socket::set_message_buffer);
	godot::register_method("set_ssl", &Socket::set_ssl);
	godot::register_method("set_blocking", &Socket::set_blocking);
	godot::register_method("set_debug", &Socket::set_debug);

	godot::register_method("receive_message", &Socket::blocking_receive_message);
	godot::register_method("receive", &Socket::blocking_receive);

	godot::register_method("send_message", &Socket::send_message);
	
	godot::register_method("ntohs", &Socket::ntohs);
	godot::register_method("htonl", &Socket::htonl);
	
	godot::register_property<Socket, bool>("debug", &Socket::debug, false);
}

void Socket::_init()
{
}

int Socket::connect_to_host(godot::String hostname, int port)
{
	debug_print("Connecting to host...");
	int error = socketWrapper.connect(hostname.ascii().get_data(), port);

	if(error) {
		debug_print("Failed to connect.");
	} else {
		debug_print("Connected successfully.");
	}

	return error;
}

void Socket::close()
{
	::free(byte_buffer);
	socketWrapper.close();
}

void Socket::set_ssl(bool trueOrFalse)
{
	socketWrapper.sslEnabled = trueOrFalse;
}

void Socket::set_blocking(bool trueOrFalse)
{
	socketWrapper.blocking = trueOrFalse;
}

void Socket::set_debug(bool trueOrFalse)
{
	debug = trueOrFalse;
}

void Socket::set_message_header_size(int size)
{
	if (!(size == 2 || size == 4)) {
		debug_print("RawSocket.set_header error: Size must be 2 or 4 bytes.\n");
		return;
	}
	headerSize = size;
}

void Socket::set_message_buffer(godot::Ref<godot::StreamPeerBuffer> messageBufferRef)
{
	messageBuffer = messageBufferRef.ptr();
	byte_buffer = (char*) malloc(messageBuffer->get_size());
}

void Socket::debug_print(const char* output)
{
	if (!debug) return;
	godot::Godot::print(output);
}

int Socket::blocking_receive_message()
{
	debug_print("blocking_receive_message: Waiting for header...");
	
	unsigned short rawHeader = 0;
	while(rawHeader == 0) {
		int result = socketWrapper.receive_ushort(rawHeader);
		if(result == -1) {
			char debug_msg[128];
			sprintf(debug_msg, "blocking_receive_message: Warning, received header size %i.", rawHeader);
			debug_print(debug_msg);
			return -1;
		}
	}
	
	int messageSize = ntohs(rawHeader);
	char debug_msg[128];
	sprintf(debug_msg, "blocking_receive_message: Header with size %i received, waiting for message...", messageSize);
	debug_print(debug_msg);
	int result = blocking_receive(messageSize);

	sprintf(debug_msg, "blocking_receive_message: Message received, returning %i.", result);
	debug_print(debug_msg);
	return result;
}

int Socket::blocking_receive(int numBytes)
{
	int result = socketWrapper.receive_bytes(numBytes, byte_buffer);
	
	if(result != -1)
	{
		fill_message_buffer(byte_buffer, numBytes);
		char debug_msg[128];
		sprintf(debug_msg, "blocking_receive: Filled godot message buffer with %i bytes.", numBytes);
		debug_print(debug_msg);
	} else {
		debug_print("blocking_receive: Error receiving message!");
	}

	return result;
}

int Socket::send_message(godot::PoolByteArray sendBuffer)
{
	int numBytes = sendBuffer.size();
	if(headerSize == 2 || headerSize == 4) {
		unsigned short header = numBytes;
		socketWrapper.send_ushort(numBytes);
	}

	const char* bytes = (const char*) sendBuffer.read().ptr();
	socketWrapper.send_bytes(bytes, numBytes);
	return 1;
}

void Socket::fill_message_buffer(char* sourceBuffer, int numBytes)
{
	messageBuffer->seek(0);
	godot::PoolByteArray* poolByteArray = &messageBuffer->get_data_array();
	for (int i = 0; i < numBytes; i++) {
		poolByteArray->set(i, sourceBuffer[i]);
	}
}

short Socket::ntohs(short var)
{
	return(::ntohs(var));
}

int Socket::htonl(int var)
{
	return(::htonl(var));
}