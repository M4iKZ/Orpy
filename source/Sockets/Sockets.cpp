#include <iostream>

#include "Sockets.hpp"

namespace Orpy
{	
	ISockets* allSockets(IHttp* h, const char* host, int& port)
	{
		return new Sockets(h, host, port);
	}
		
	Sockets::Sockets(IHttp* h, const char* host, int& port) : _http(h), _port(port)
	{ 
#ifdef _WIN32
		sprintf_s(_host, "%s", host);
#else
		snprintf(_host, sizeof(_host), "%s", host);
#endif
		
#ifdef _WIN32
		numWorkers = std::thread::hardware_concurrency();

		if (numWorkers == 0) numWorkers = 1; // set a default value
		else numWorkers -= 1;
#else
		numWorkers = sysconf(_SC_NPROCESSORS_ONLN);

		_worker_args.resize(numWorkers);
#endif

		_workers.resize(numWorkers);
		
		std::string str;
		str.assign(_host, sizeof(_host) / sizeof(char));

		debug("Sockets init done! Configurated " + std::to_string(numWorkers + 1) + " workers and 1 listener ...");
		debug("host: " + str + " and port: " + std::to_string(_port));
	}

	Sockets::~Sockets() 
	{ 
		_isRunning.store(false);
		
		socketClose();
		
		std::cout << "socket closed ... " << std::endl;
		
		_sync.notifyClose();
		
#ifdef _WIN32		
		if (_listener.joinable())
			_listener.join();
#else
		pthread_join(_listener, nullptr);
#endif
						
		for (auto&t : _workers)
		{
#ifdef _WIN32	
			if (t.joinable())
				t.join();			
#else
			pthread_join(t, nullptr);
#endif			
		}
				
		debug("Sockets lib unloaded...");		
	}

	int Sockets::socketInit()
	{
#ifdef _WIN32
		WSADATA wsaData;

		int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (result != 0)
		{
			printf("WSAStartup failed with error: %d\n", result);
			return 0;
		}
#endif
		return 1;
	}

	/** Returns true on success, or false if there was an error */
	bool Sockets::socketSetBlocking(int fd, bool blocking)
	{
		if (fd < 0) return false;

#ifdef _WIN32
		//-------------------------
		// Set the socket I/O mode: In this case FIONBIO
		// enables or disables the blocking mode for the 
		// socket based on the numerical value of iMode.
		// If iMode = 0, blocking is enabled; 
		// If iMode != 0, non-blocking mode is enabled.
		u_long iMode = blocking ? 0 : 1;
		return (ioctlsocket(fd, FIONBIO, &iMode) == 0) ? true : false;
#else
		return true;
#endif
	}

	int Sockets::socketClose()
	{
		int status = 0;

#ifdef _WIN32
		status = shutdown(_sock, SD_BOTH);
		if (status == 0) { status = closesocket(_sock); WSACleanup(); }		
#endif
		return status;
	}

	bool Sockets::start(bool block)
	{
		_block = block;

		// Initialize Winsock, returning on failure
		if (!socketInit())
		{
			printf("error in winsock init\n");
			return false;
		}

#ifdef _WIN32
		_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#else
		_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
#endif
		if (_sock < 0)
		{
			printf("Cannot create socket");
			return false;
		}

		// initialize the socket's address
		memset(&_socketAddress, 0, sizeof(sockaddr_in));

		_socketAddress.sin_family = AF_INET;
		_socketAddress.sin_port = htons(_port);

		_socketAddress.sin_addr.s_addr = inet_pton(AF_INET, _host, &_socketAddress.sin_addr);
		if (_socketAddress.sin_addr.s_addr == INADDR_NONE)
		{
			printf("Address Error!\n");
			socketClose();
			return false;
		}

		_socketAddress_len = sizeof(_socketAddress);
						
		int reuse_addr = 1;

		setsockopt(_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr, sizeof(reuse_addr));

#ifndef _WIN32
		int optval = 1;

		setsockopt(_sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
#endif 

		if (bind(_sock, (sockaddr*)&_socketAddress, _socketAddress_len) < 0)
		{
			printf("Cannot connect socket to address\n");
			socketClose();
			return false;
		}

		if (listen(_sock, SOMAXCONN) < 0)
		{
			printf("Socket listen failed\n");
			socketClose();
			return false;
		}

		if (!socketSetBlocking(_sock, _block))
		{
			printf("Problems with SetBlocking");
			socketClose();
			return false;
		}

		_isRunning.store(true);
		
		std::cout << "Socket init succefully" << std::endl;

#ifdef _WIN32
		_listener = std::thread(&Sockets::listener, this);		
#else
		pthread_create(&_listener, nullptr, &Sockets::listener_helper, this);
#endif

		for (int id = 0; id < numWorkers; ++id)
		{
#ifdef _WIN32
			_workers.push_back(std::thread(&Sockets::Worker, this, id));
#else
			_worker_args[id] = { id, this };
			pthread_create(&_workers[id], nullptr, &Sockets::worker_helper, &_worker_args[id]);
#endif
		}

		return true;
	}

	void Sockets::listener()
	{
		debug("ready to listen ... ");

		sockaddr_in client_address;
		while (_isRunning.load())
		{
			socklen_t client_len = sizeof(client_address);

#ifdef _WIN32
			socket_t client_fd = (int)accept(_sock, (sockaddr*)&client_address, &client_len);
#else
			socket_t client_fd = accept4(_sock, (sockaddr*)&client_address, &client_len, SOCK_NONBLOCK);
#endif
			
			if (client_fd < 0) continue;

			if (!socketSetBlocking(client_fd, _block))
			{
				continue;
			}

			HTTPData* data = new HTTPData();
			data->fd = client_fd;

			_sync.push(data);
		}

		debug("listener stopped ... ");
	}

	void Sockets::Worker(int id)
	{
		debug("worker " + std::to_string(id) + " starting ... ");
		
		while (_isRunning.load())
		{
			void* ptr = _sync.waitCondition(id);

			if (ptr != nullptr)
			{
				HTTPData* data = reinterpret_cast<HTTPData*>(ptr);
				if (data->phase == 0)
					receiveData(data);				
				else if (data->phase == 1)
					sendData(data);
				else if (data->phase == 2)
				{
					HTTPData* response = new HTTPData(1);
					response->fd = data->fd;

					_http->elaborateData(data, response);
					_sync.push(response);

					delete data;
				}
				else if (data->phase == 3)
				{
					sendFile(data);
					delete data;
				}
				else
					delete data;
			}
		}

		debug("worker " + std::to_string(id) + " stopped");
	}
		
	void Sockets::receiveData(HTTPData* data)
	{
		HTTPData* request, * response;
		request = data;
				
		int byte_count = Receive(request->fd, request->buffer);
		if (byte_count > 0)
		{		
			request->phase = 2;
			_sync.push(request);
			
			return;
		}		
		else if (byte_count < 0)
		{
#ifdef _WIN32
			if (WSAGetLastError() == WSAEWOULDBLOCK)
#else 
			if (errno == EAGAIN || errno == EWOULDBLOCK)
#endif
			{
				if (request->startTime > std::time(nullptr))
				{
					_sync.push(request);

					return;
				}
			}
		}
		
		close(request->fd, false);
		delete request;
	}
	
	void Sockets::sendData(HTTPData* data)
	{
		HTTPData* request, * response;
		response = data;
				
		int byte_count = Send(response->fd, response->buffer, response->length);
		if (byte_count >= 0)
		{
			request = new HTTPData(0);
			request->fd = response->fd;
						
			_sync.push(request);
						
			delete response;							
		}
		else
		{			
			close(data->fd, false);
			delete response;						
		}
	}

	void Sockets::sendFile(HTTPData* data)
	{
		if (data->fileSize <= 0)
		{
			std::cout << "fileSize == 0!" << std::endl;
			return;
		}

		if (Send(data->fd, data->buffer, data->length) < data->length)
		{
			std::cout << "are you sure about the lenght of the header?" << std::endl;
			return;
		}

		std::ifstream file(data->fileName, std::ios::in | std::ifstream::binary);
		if (file.fail())
		{
			std::cout << "are you sure about the file?" << std::endl;
			return;
		}
		
		bool error = false;
		int64_t i = data->fileSize;		
		while (i != 0)
		{			
			int64_t ssize = __min(i, ReadSize);

			std::vector<char> chunk(ssize);

			if (!file.read(chunk.data(), chunk.size()))
			{
				error = true;
				std::cout << "did you touch the file?" << std::endl;

				break;
			}

			int l = Send(data->fd, chunk, (int)ssize);
			if (l <= 0)
			{
				error = true;
				std::cout << "something went wrong with the connection!" << std::endl;

				break;
			}

			i -= l;			
		}
				
		file.close();

		if (error)
		{
			close(data->fd, false);
			delete data;
		}
		else
		{
			HTTPData* request = new HTTPData(0);
			request->fd = data->fd;

			_sync.push(request);
		}
	}

	void Sockets::receiveAll(HTTPData* data, int size)
	{
		int received = 0;
		while (true)
		{
			int byte_count = Receive(data->fd, data->buffer);
			if (byte_count > 0)
			{	
				received += byte_count;

				if(received == size)
					return;				
			}
			else if (byte_count < 0)
			{
#ifdef _WIN32
				if (WSAGetLastError() == WSAEWOULDBLOCK)
#else 
				if (errno == EAGAIN || errno == EWOULDBLOCK)
#endif
				{
					continue;
				}
			}
		}
	}

	int Sockets::Receive(socket_t fd, std::vector<char>& buffer)
	{
		int bytes_received = 0;
		int total_bytes = 0;
		while (true) 
		{
			std::vector<char> chunk(ReadSize);

			bytes_received = recv(fd, &chunk[0], ReadSize, 0);
						
			if (bytes_received <= 0)
			{				
				if (total_bytes > 0)
					break;
				else if (total_bytes == 0)
					if (bytes_received != 0)
						total_bytes = bytes_received;
				
				break;
			}
			
			buffer.insert(buffer.end(), chunk.begin(), chunk.begin() + bytes_received);
			total_bytes += bytes_received;	
		}

		return total_bytes;
	}

	int Sockets::Send(socket_t fd, std::vector<char> buffer, int length)
	{
		int l = send(fd, buffer.data(), buffer.size(), 0);
		if (l < 0)
		{
#ifdef _WIN32 
			if (WSAGetLastError() == WSAEWOULDBLOCK)
#else 
			if (errno == EAGAIN || errno == EWOULDBLOCK)
#endif				
				return Send(fd, buffer, length);			
			else		
				return l;
		}
		
		return l;		
	}
	
	void Sockets::close(int fd, bool main)
	{
#ifdef _WIN32                
		closesocket(fd);
#else           
		if (main)
			close(fd);
		else
			shutdown(fd, SHUT_WR);
#endif
	}
}