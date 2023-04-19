#ifndef _SOCKET_HPP_
#define _SOCKET_HPP_

#include <vector>

// ------------------------------------------------------------------
// 継承用のクラス

class ASocket
{
	private:
    	int fd_;
	
	public:
		ASocket(int fd);
		ASocket(const ASocket& src);
		virtual ~ASocket() {}
		ASocket& operator=(const ASocket& rhs);

		int fd() const;
};

// ------------------------------------------------------------------
// 通信用のソケット

class ConnSocket : public ASocket
{
	private:
    	std::vector<char> recv_buffer_;
    	std::vector<char> send_buffer_;

		void on_message_received();
		bool is_message_complete() const;
	
	public:
		ConnSocket(int fd);
		ConnSocket(const ConnSocket& src);
		~ConnSocket() {}
		ConnSocket& operator=(const ConnSocket& rhs);

		int on_readable(int recv_flag);
		int on_writable();
};

#endif