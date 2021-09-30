#include "SocketWrapper.hpp"

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
    } catch(error_code &error) {
        cout << "Connect failed: " << error.message() << endl;
        lastError = error;
        return 1;
    }

    return 0;
}

void SocketWrapper::receive(int numBytes)
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
    }
}

unsigned short SocketWrapper::receive_ushort()
{
    int numBytes = sizeof(uint16_t);
    buffer.prepare(numBytes);
    receive(numBytes);
    const uint16_t* rawData = buffer_cast<const uint16_t*>(buffer.data());
    buffer.consume(numBytes);
    return *rawData;
}

const char* SocketWrapper::receive_bytes(int numBytes)
{
    buffer.prepare(numBytes);
    receive(numBytes);
    const char* rawData = buffer_cast<const char*>(buffer.data());
    buffer.consume(numBytes);
    return rawData;
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
        return 1;
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
        return 1;
    }

    return 0;
}