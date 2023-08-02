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

#else
# include <arpa/inet.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <sys/time.h>
# include <netdb.h>
# include <unistd.h>
# include <cstring>

# include <pthread.h>

#define __min(a,b) (((a) < (b)) ? (a) : (b))

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

#include "ISockets.hpp"
#include "Synchronization.hpp"

namespace Orpy
{
	class Sockets : public ISockets
	{
	protected:
#ifdef _WIN32
		typedef SOCKET socket_t;
		typedef int socklen_t;
		/* POSIX ssize_t is not a thing on Windows */
		typedef signed long long int ssize_t;
#else
		typedef int socket_t;
		// winsock has INVALID_SOCKET which is returned by socket(),
		// this is the POSIX replacement
# define INVALID_SOCKET -1
#endif /* _WIN32 */

		IHttp* _http;

		socket_t _sock = INVALID_SOCKET;

		int _timeout_in_seconds = 2 * 60;

		char _host[10];
		short _port;

		struct sockaddr_in _socketAddress = sockaddr_in();
		int _socketAddress_len = 0;

		bool _block = false;

		int  socketInit();
		bool socketSetBlocking(int, bool);
		void socketClose();

		bool setSocket(int, socket_t, sockaddr_in&, int&);

		void listener();
		void Worker(int);

		bool receiveData(std::unique_ptr<HTTPData>&);
		bool sendData(std::unique_ptr<HTTPData>&);
				
		int Receive(std::unique_ptr<HTTPData>&, int&);
		int Send(std::unique_ptr<HTTPData>&, int&);
		
		// Sends a file
		bool sendFile(std::unique_ptr<HTTPData>&);

		void clear(std::unique_ptr<HTTPData>&);
		void closeClient(const socket_t&);

		//thread vars
		std::atomic<bool> _isRunning;		
		int numWorkers;
		
		ThreadPool<HTTPData> _queue;
		
		std::vector<std::thread> _workers;
		std::thread _listener;

	public:
		Sockets(IHttp*, const char*, int&);
		~Sockets() override;

		bool start(bool = false) override;
	};
}