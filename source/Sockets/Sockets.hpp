#pragma once

// Windows
#ifdef _WIN32

#define NOMINMAX

# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif /* WIN32_LEAN_AND_MEAN */
# ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#  define _WINSOCK_DEPRECATED_NO_WARNINGS
# endif /* _WINSOCK_DEPRECATED_NO_WARNINGS */
# include <winsock2.h>
# include <ws2tcpip.h>
# include <iphlpapi.h>
# include <windows.h>
# pragma comment(lib, "Ws2_32.lib")
# pragma comment(lib, "Mswsock.lib")
# pragma comment(lib, "AdvApi32.lib")

typedef SOCKET socket_t;
typedef int socklen_t;
/* POSIX ssize_t is not a thing on Windows */

typedef signed long long int ssize_t;

#else

typedef int socket_t;

// winsock has INVALID_SOCKET which is returned by socket(),
// this is the POSIX replacement
# define INVALID_SOCKET -1

# include <arpa/inet.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <sys/time.h>
# include <netdb.h>
# include <unistd.h>
# include <cstring>

# include <pthread.h>

#endif 

#include <algorithm>
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <array>
#include <atomic>
#include <fstream>
#include <iostream>
#include <memory>
#include <thread>
#include <shared_mutex>

#include "Common/common.hpp"
#include "Common/ISockets.hpp"
#include "Common/ICore.hpp"

#include "Pool/synchronization.hpp"

namespace Orpy
{
	class Sockets : public ISockets
	{
	protected:
		socket_t _sock = INVALID_SOCKET;

		std::string _host = "locahost";
		int _port = 8888;

		struct sockaddr_in _socketAddress = sockaddr_in();
		int _socketAddress_len = 0;

		bool _block = false;

		int  socketInit();
		bool socketSetBlocking(int, bool);
		void  socketClose();

		bool setSocket(int, socket_t, sockaddr_in&, int&);

		void listener();
		void Worker(int);

		bool receiveData(std::unique_ptr<http::Data>&);
		bool sendData(std::unique_ptr<http::Data>&);

		// Send a file
		bool sendFile(std::unique_ptr<http::Data>&);

		int Receive(std::unique_ptr<http::Data>&, int&);
		int Send(std::unique_ptr<http::Data>&, int&);
				
		void clearClient(std::unique_ptr<http::Data>&);
		void closeClient(const socket_t&);

		void push(std::unique_ptr<http::Data>, int = 0);
				
		std::atomic<bool> _isRunning;		
		int numWorkers = 3;
		int currWorker = 1;
		
		std::vector<std::unique_ptr<synchronization::Pool<http::Data>>> _queues;
		
		std::vector<std::thread> _workers;
		std::thread _listener;

		std::shared_mutex _mutex;

	public:
		Sockets(const std::string&, const int&);
		~Sockets();

		bool start(bool = false) override;
		bool receivePOST(std::unique_ptr<http::Data>&) override;
	}; 
}