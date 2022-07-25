#ifndef SOCKET_WRAPPER_H
#define SOCKET_WRAPPER_H

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

class SocketWrapper {
private:
	boost::asio::streambuf buffer;
	int receive(int numBytes);
public:
	boost::system::error_code lastError;
	bool sslEnabled = false;
	bool blocking = false;
	int connect(const char* hostname, int port);
	void close();
	const int receive_ushort(unsigned short &header);
	int receive_bytes(int numBytes, char* byte_buffer);

	int send_ushort(unsigned short ushort);
	int send_bytes(const char* bytes, int numBytes);
};

#endif

