#ifndef GODOT_WIN_SOCKET_H
#define GODOT_WIN_SOCKET_H

#include <stdio.h>

#include <Godot.hpp>
#include <Node.hpp>
#include <StreamPeerBuffer.hpp>

#include <winsock2.h>

#define WIN_SOCKET_DEBUG_MODE_NONE 0
#define WIN_SOCKET_DEBUG_MODE_GODOT 1
#define WIN_SOCKET_DEBUG_MODE_STDOUT 2

#define WIN_SOCKET_DEFAULT_BUFFER_SIZE 16384

int init_winsocket();
SOCKET connect_socket(const char* hostName, int port, int debugMode);
void set_blocking_socket(SOCKET socket);
void set_non_blocking_socket(SOCKET socket);

void blocking_receive_message(SOCKET socket, byte* dataBuffer, int headerSize);
void blocking_receive(SOCKET socket, byte* dataBuffer, int numBytes);

void non_blocking_receive_message(SOCKET socket, byte* dataBuffer, byte headerBuffer, unsigned int* headerBufferIndex, unsigned int* dataBufferIndex, int headerSize);
int non_blocking_receive(SOCKET socket, byte* dataBuffer, unsigned int* dataBufferIndex, int numBytes);

void debug_print(const char* output, int debug_mode);


	class WinSocket : public godot::Node {
		GODOT_CLASS(WinSocket, godot::Node)

	private:
		SOCKET winSocket = INVALID_SOCKET;

		unsigned int headerSize = 0;
		// Used in non blocking mode to remember how many bytes we are waiting for between calls.
		unsigned int bytesLeft = 0;
		unsigned int headerBufferIndex;
		unsigned int messageBufferIndex;

		UINT16 shortHeader;
		INT32 intHeader;

		godot::StreamPeerBuffer* messageBuffer;

		bool debug = true;
		bool isBlocking = true;

		int init_winsocket();
		void debug_print(const char * output);
		void blocking_receive(byte* buffer, int numBytes);
		int non_blocking_receive(byte* dataBuffer, unsigned int* dataBufferIndex, int numBytes);

	public:
		byte* headerBuffer;

		static void _register_methods();
		void _init();
		void _process(float delta);
		void connect_to_host(godot::String hostName, int port);
		void set_message_header_size(int size);
		void set_message_buffer(godot::Ref<godot::StreamPeerBuffer> messageBuffer);
		void set_blocking(bool blocking);
		void send_message(const char* data);

		void blocking_receive_message();
		int non_blocking_receive_message();
	};

#endif