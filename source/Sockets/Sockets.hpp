#pragma once

// Windows
#ifdef _WIN32
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

#include <thread>

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

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <atomic>
#include <fstream>

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
		int  socketClose();

		void listener();
		void Worker(int);

		void receiveData(HTTPData*);
		void sendData(HTTPData*);
		
		int Receive(socket_t, std::vector<char>&);
		int Send(socket_t, std::vector<char>, int);

		//
		// https://stackoverflow.com/questions/63494014/sending-files-over-tcp-sockets-c-windows
		// 
		// Sends a file
		void sendFile(HTTPData*);

		//thread vars
		std::atomic<bool> _isRunning;
		int numWorkers;

		ThreadSynchronization _sync;

#ifdef _WIN32
		std::thread _listener;
		std::vector<std::thread> _workers;
#else
		pthread_t _listener;
		std::vector<pthread_t> _workers;

		static void* listener_helper(void* context)
		{
			reinterpret_cast<Sockets*>(context)->listener();
			return nullptr;
		}

		struct worker_args 
		{
			int id;
			Sockets* s;
		};

		std::vector<struct worker_args> _worker_args;

		static void* worker_helper(void* context)
		{
			struct worker_args* args = reinterpret_cast<struct worker_args*>(context);
			Sockets* s = args->s;
			s->Worker(args->id);
			return nullptr;
		}
#endif
	public:
		Sockets(IHttp*, const char*, int&);
		~Sockets() override;

		bool start(bool = false) override;
		void close(int, bool = true) override;	

		void receiveAll(HTTPData*, int) override;
	};
}