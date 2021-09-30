#ifndef SOCKET_WRAPPER_H
#define SOCKET_WRAPPER_H

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

class SocketWrapper {
private:
    boost::asio::streambuf buffer;
    void receive(int numBytes);
public:
    boost::system::error_code lastError;
    bool sslEnabled = false;
    bool blocking = true;
    int connect(const char* hostname, int port);
    unsigned short receive_ushort();
    const char* receive_bytes(int numBytes);

    int send_ushort(unsigned short ushort);
    int send_bytes(const char* bytes, int numBytes);
};

#endif

