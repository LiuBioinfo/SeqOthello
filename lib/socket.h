/*
 *   C++ sockets on Unix and Windows
 */

#ifndef __SOCKET_UTIL_H
#define __SOCKET_UTIL_H

#include <exception>
#include <string>
#include <cstring>
#include <cstdlib>

using namespace std;

static string TYPE_CONTAINMENT = "CONTAINMENT";
static string TYPE_COVERAGE = "COVERAGE";
class Socket
{
public:
    ~Socket();
    string getLocalAddr();
    unsigned short getLocalPort(); 
    void setLocalPort(unsigned short localPort);
private:
    Socket(const Socket &sock);
    void operator=(const Socket &sock);
protected:
    int sockDesc;              // Socket descriptor
    Socket(int type, int protocol);
    Socket(int sockDesc);
};


class TCPSocket : public Socket
{
public:
    TCPSocket(); 
    TCPSocket(const string &foreignAddr, unsigned short foreignPort);
    void connect(const string &foreignAddr, unsigned short foreignPort);
    unsigned short getForeignPort();
    void sendmsg(const string &str);
    void sendmsg(const char * buf, uint32_t len);
    static constexpr int BUFLEN=65536;
    char recvbuf[BUFLEN+64];
    bool recvmsg(string &msg);
    string getForeignAddr();
private:
    void send(const void *buffer, int bufferLen);
    int recv(void *buffer, int bufferLen);
    friend class TCPServerSocket;
    TCPSocket(int newConnSD);
};

class TCPServerSocket : public Socket
{
public:
    TCPServerSocket(unsigned short localPort, int queueLen = 5);
    TCPSocket *accept();
};


#endif
