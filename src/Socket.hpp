#ifndef _SOCKET_HPP_
#define _SOCKET_HPP_

#include <vector>
#include <netinet/in.h>

// ------------------------------------------------------------------
// 継承用のクラス

class ASocket
{
  protected:
    int                 fd_;
    struct sockaddr_in  sockaddr_;

  public:
    ASocket();
    ASocket(const ASocket& src);
    virtual ~ASocket();
    ASocket& operator=(const ASocket& rhs);

    int   GetFd() const;
    void  SetFd(int fd);
    int   SetNonBlocking() const;
    struct sockaddr_in*  GetSockaddr();
};

// ------------------------------------------------------------------
// 通信用のソケット

class ConnSocket : public ASocket
{
  private:
    std::vector<char> recv_buffer_;
    std::vector<char> send_buffer_;

    void OnMessageReceived();
    bool IsMessageComplete() const;

  public:
    ConnSocket();
    ConnSocket(const ConnSocket& src);
    ~ConnSocket();
    ConnSocket& operator=(const ConnSocket& rhs);

    int OnReadable(int recv_flag);
    int OnWritable();
};

// ------------------------------------------------------------------
// listen用のソケット

class ListenSocket : public ASocket
{
  private:

  public:
    ListenSocket();
    ListenSocket(const ListenSocket& src);
    ~ListenSocket();
    ListenSocket& operator=(const ListenSocket& rhs);

    int         Create();
    int         Passive(int port);
    ConnSocket* Accept();
};

#endif