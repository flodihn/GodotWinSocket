#include "SocketWrapper.hpp"
#include <GodotGlobal.hpp>

#include <iostream>

#define NON_BLOCKING_RECEIVE_FLAGS 0

using namespace boost::asio;
using error_code = boost::system::error_code;

using std::cout;
using std::endl;

/*
 * Normally it is bad practice to put variables like this
 * outside the class in a C++ project. But because of 
 * issues with the Godot includes breaking the boost
 * includes, having these as class members is not
 * possible. Someone with better knowledge about C++ and
 * boost should probably look into this.
 *
 * Implications of putting variables here is that MULTIPLE
 * INSTANCES of SocketWrapper class WILL NOT WORK.
 * In practice this is not as problem as long as the code
 * is only used as a client.
 */
boost::asio::io_service ioService;
boost::asio::io_context ioContext;
ssl::context sslContext(ssl::context::tls);
ip::tcp::socket tcpSocket(ioService);
ssl::stream<ip::tcp::socket> secureSocket(ioContext, sslContext);

int SocketWrapper::connect(const char* hostname, int port)
{
	try {
		std::string portAsString = std::to_string(port);
		ip::tcp::resolver resolver(ioService);
		auto endpoints = resolver.resolve(hostname, portAsString);

		if(sslEnabled) {
			boost::asio::connect(secureSocket.next_layer(), endpoints);
			secureSocket.set_verify_mode(ssl::verify_none);
			secureSocket.handshake(ssl::stream_base::client);
		} else {
			boost::asio::connect(tcpSocket, endpoints);
		}

		if(blocking == false) {
			boost::system::error_code ec;
			secureSocket.native_non_blocking(true, ec);
		}
		
	} catch(error_code &error) {
		cout << "Connect failed: " << error.message() << endl;
		lastError = error;
		return 1;
	}

	return 0;
}

void SocketWrapper::close()
{
	if(sslEnabled) {
		secureSocket.shutdown();
	} else {
		tcpSocket.shutdown(ip::tcp::socket::shutdown_both);
	}
}

int SocketWrapper::receive(int numBytes)
{
	error_code error;

	if(sslEnabled) {
		read(secureSocket, buffer, transfer_exactly(numBytes), error);
	} else {
		read(tcpSocket, buffer, transfer_exactly(numBytes), error);
	}

	if(error && error != error::eof ) {
		cout << "receive failed: " << error.message() << endl;
		lastError = error;
		return -1;
	}

	return numBytes;
}

const int SocketWrapper::receive_ushort(unsigned short &header)
{
	int numBytes = sizeof(unsigned short);
	buffer.prepare(numBytes);
	int result = receive(numBytes);

	if(result == -1) {
		return -1;
	}

	header = *buffer_cast<const unsigned short*>(buffer.data());

	// char debug_msg[128];
	// sprintf(debug_msg, "Received 2 byte ushort %i.", *data);
	// godot::Godot::print(debug_msg);
	buffer.consume(numBytes);

	// sprintf(debug_msg, "Received 2 byte ushort %i (after consume).", *data);
	// godot::Godot::print(debug_msg);
	// header = data;
	return result;
}

int SocketWrapper::receive_bytes(int numBytes, char* byte_buffer)
{
	buffer.prepare(numBytes);
	int result = receive(numBytes);

	if(result != -1) {
		memcpy(byte_buffer, buffer_cast<const char*>(buffer.data()), numBytes);
		//byte_buffer = (char*) buffer_cast<const char*>(buffer.data());
		for(int i = 0; i < numBytes; i++) {
			cout << byte_buffer[i];
		}
		cout << endl;
		buffer.consume(numBytes);
	} else {
		char debug_msg[128];
		sprintf(debug_msg, "receive_bytes: Failed receiving %i bytes!", numBytes);
		cout << debug_msg << endl;
	}
	return result;
}

int SocketWrapper::send_ushort(unsigned short ushort)
{
	error_code error;
	ushort = htons(ushort);

	if(sslEnabled) {
		boost::asio::write(secureSocket, boost::asio::buffer(&ushort, 2), transfer_all(), error); 
	} else {
		boost::asio::write(tcpSocket, boost::asio::buffer(&ushort, 2), transfer_all(), error);
	}

	cout << "sent ushort: " << error.message() << endl;

	if(error && error != error::eof ) {
		cout << "send failed: " << error.message() << endl;
		lastError = error;
		return -1;
	}

	return 0;
}

int SocketWrapper::send_bytes(const char* bytes, int numBytes)
{
	error_code error;
	if(sslEnabled) {
		boost::asio::write(secureSocket, boost::asio::buffer(bytes, numBytes), transfer_all(), error); 
	} else {
		boost::asio::write(tcpSocket, boost::asio::buffer(bytes, numBytes), transfer_all(), error);
	}

	 cout << "sent bytes: " << error.message() << endl;

	if(error && error != error::eof ) {
		cout << "send failed: " << error.message() << endl;
		lastError = error;
		return -1;
	}

	return 0;
}