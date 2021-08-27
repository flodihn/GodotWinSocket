#include "WinSocket.hpp"

int main(int argc, char* argv[])
{
	//UINT16 header;
	const char* data[64];
	ZeroMemory(data, sizeof(data));

	godot::WinSocket* winSocketCls = new godot::WinSocket();;
	//int error = init_winsocket();
	winSocketCls->_init();
	printf("Program connecting to server...\n");
	winSocketCls->connect_to_host("localhost", 2000);
	winSocketCls->set_blocking(true);
	winSocketCls->set_message_header_size(4);
	int attempts = 0;

	while (attempts < 10) {
		int result = winSocketCls->non_blocking_receive_message();
		printf("non_blocking_receive_message attempt: %d (result %d).\n", attempts, result);

		if (result > 0) {
			memcpy(data, winSocketCls->dataBuffer, result);
			//data[11] = '\0';
			printf("Received message: %s.\n", (char*)data);
		}
		attempts += 1;
	}

	while (attempts < 10) {
		int result = winSocketCls->non_blocking_receive_message();
		printf("non_blocking_receive_message attempt: %d (result %d).\n", attempts, result);
		if (result > 0) {
			memcpy(data, winSocketCls->dataBuffer, result);
			//data[11] = '\0';
			printf("Received message: %s.\n", (char*)data);
			return 0;
		}
		attempts += 1;
	}

	winSocketCls->send_message("Hello world from WinSock!");
	
	//winSocketCls->receive(13);
	

	/*
	winSocketCls->blocking_receive((byte*) &header, 2);
	header = ntohs(header);
	printf("Received header containing message size: %d.\n", header);
	winSocketCls->blocking_receive((byte*)&data, header);
	
	*/
	printf("Connecting finished.\n");

	/*
	winSocket = INVALID_SOCKET;

	if (error > 0)
	{
		debug_print("Failed to initialize Winsock2.\n", WIN_SOCKET_DEBUG_MODE_STDOUT);
		return error;
	}

	debug_print("Successfully initialized Winsock2.\n", WIN_SOCKET_DEBUG_MODE_STDOUT);

	init_winsocket();

	SOCKET socket = connect_socket("localhost", 2000, WIN_SOCKET_DEBUG_MODE_STDOUT);
	set_non_blocking_socket(socket);
	char* helloWorld[13];
	ZeroMemory(helloWorld, 13);
	int attempts = 0;
	int bytesLeft = 2;
	
	while (headerBufferIndex < 2) {
		int numBytesReceived = non_blocking_receive(socket, (byte*) &headerBuffer, &headerBufferIndex, bytesLeft);
		bytesLeft -= numBytesReceived;

		attempts += 1;
		if (attempts > 20) {
			printf("Receive header aborted after %d attempts.\n", attempts);
			return 1;
		}
	}

	attempts = 0;
	headerBuffer = ntohs(headerBuffer);
	printf("Received header with size: %d.\n", headerBuffer);

	bytesLeft = headerBuffer;

	while (bytesLeft > 0) {
		int numBytesReceived = non_blocking_receive(socket, (byte*) helloWorld, &dataBufferIndex, bytesLeft);
		bytesLeft -= numBytesReceived;

		printf("Received %d bytes (%d)\n", numBytesReceived, headerBufferIndex);
		attempts += 1;
		if (attempts > 20) {
			printf("Receive data aborted after %d attempts.\n", attempts);
			break;
		}
	}

	helloWorld[12] = '\0';
	if (helloWorld != NULL) {
		printf("%s.\n", helloWorld);
	}
	*/

	return 0;
}