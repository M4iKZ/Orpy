
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

		for (auto& t : _workers)
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
		if (!setSocket(_port, _sock, _socketAddress, _socketAddress_len))
		{
			printf("Problems with setSocket for HTTP!");
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
#ifdef _WIN32
			_workers.push_back(std::thread(&Sockets::Worker, this, id));
#else
		{
			_worker_args[id] = { id, this };
			pthread_create(&_workers[id], nullptr, &Sockets::worker_helper, &_worker_args[id]);
		}
#endif

		return true;
	}

	bool Sockets::setSocket(int port, socket_t sock, sockaddr_in& socketAddress, int& socketAddress_len)
	{
		memset(&socketAddress, 0, sizeof(sockaddr_in));

		socketAddress.sin_family = AF_INET;
		socketAddress.sin_port = htons(port);

		socketAddress.sin_addr.s_addr = inet_pton(AF_INET, _host, &socketAddress.sin_addr);
		if (socketAddress.sin_addr.s_addr == INADDR_NONE)
		{
			printf("Address Error!\n");
			socketClose();
			return false;
		}

		socketAddress_len = sizeof(socketAddress);

#ifndef _WIN32
		int optval = 1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
#endif 
		int reuse_addr = 1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr, sizeof(reuse_addr));

		if (bind(sock, (sockaddr*)&socketAddress, socketAddress_len) < 0)
		{
			printf("Cannot connect socket to address\n");
			socketClose();
			return false;
		}

		if (listen(sock, SOMAXCONN) < 0)
		{
			printf("Socket listen failed\n");
			socketClose();
			return false;
		}

		if (!socketSetBlocking(sock, _block))
		{
			printf("Problems with SetBlocking");
			socketClose();
			return false;
		}

		return true;
	}

	void Sockets::listener()
	{
		debug("ready to listen ... ");
				
		while (_isRunning.load())
		{
			int client_fd = -1;
			sockaddr_in client_address;
			socklen_t client_len = sizeof(client_address);

			fd_set readfds;
			FD_ZERO(&readfds);
			FD_SET(_sock, &readfds);

			int max_sd = _sock;

			struct timeval timeout;
			timeout.tv_sec = 1;
			timeout.tv_usec = 0;

			int iResult = select(max_sd + 1, &readfds, NULL, NULL, &timeout);
			if (iResult == -1)
			{
				socketClose();
				break;
			}
			else if (iResult == 0)
			{
				if (_isRunning.load())
					continue;
				else
					break;
			}
						
#ifdef _WIN32				
			client_fd = accept(_sock, (sockaddr*)&client_address, &client_len);
#else
			client_fd = accept4(_sock, (sockaddr*)&client_address, &client_len, SOCK_NONBLOCK);
#endif			
			if (client_fd == INVALID_SOCKET)
			{
				close(client_fd, false);
				continue;
			}

			if (!socketSetBlocking(client_fd, _block))
				continue;

			std::string key = std::to_string(client_fd);
			
			HTTPData* client = new HTTPData(0, client_fd, key);
			
			char client_ip[INET_ADDRSTRLEN];
			struct in_addr ipAddr = client_address.sin_addr;
			inet_ntop(AF_INET, &ipAddr, client_ip, INET_ADDRSTRLEN);

			std::string ip(client_ip);
			client->IP = ip;

			addClient(client);

			_sync.push(key);
		}

		debug("listener stopped ... ");
	}

	void Sockets::Worker(int id)
	{
		debug("worker " + std::to_string(id) + " starting ... ");

		while (_isRunning.load())
		{
			std::string key = _sync.waitCondition(id);
			if (key != std::string())
			{
				auto data = _clients.find(key);
				if (data != _clients.end())
				{
					int phase = data->second->phase;
					if (data->second->startTime < std::time(nullptr))
						clear(key);
					else if (phase == 0)
						receiveData(data->second);
					else if (phase == 1)
						sendData(data->second);
					else if (phase == 2)
						elaborate(data->second);
					else if (phase == 3)
						sendFile(data->second);
					else
						clear(key);
				}
			}
		}

		debug("worker " + std::to_string(id) + " stopped");
	}

	void Sockets::elaborate(HTTPData* data)
	{
		data->phase = 1; //Response
		_http->elaborateData(data);

		_sync.push(data->key);
	}

	void Sockets::receiveData(HTTPData* data)
	{
		int byte_count = 0;
		if (!Receive(data, byte_count))
			return;
				
		data->phase = 2; //Elaborate!			
		elaborate(data);
	}

	void Sockets::sendData(HTTPData* data)
	{
		int byte_count = 0;
		if (!Send(data, byte_count))
		{
			clear(data->key);
			return;
		}

		std::string key = data->key;
		_clients[key] = new HTTPData(0, data->fd, data->key, data->IP);
		delete data;

		_sync.push(key);		
	}

	void Sockets::sendFile(HTTPData* data)
	{
		if (data->fileSize < 0)
		{
			debug("fileSize < 0!");
			clear(data->key);
			return;
		}

		int len = 0;
		if (!data->sentHeader)
		{
			if (!Send(data, len))
				return;

			if (len < data->length)
			{
				debug("are you sure about the lenght of the header?");
				clear(data->key);
				return;
			}

			data->sentHeader = true;
			data->buffer.clear();
			data->startTime = setTime();
		}
		else if (data->buffer.size() > 0)
		{
			if (!Send(data, len))
				return;

			data->sent += len;
			data->buffer.clear();
		}

		if (data->sent >= data->fileSize)
		{
			std::string key = data->key;
			_clients[key] = new HTTPData(0, data->fd, data->key, data->IP);
			delete data;

			_sync.push(key);
			return;
		}

		std::fstream file(data->fileName, std::ios::in | std::ifstream::binary);
		if (file.fail())
		{
			debug("are you sure about the file?");
			clear(data->key);
			return;
		}

		size_t ssize = std::min((data->fileSize - data->sent), (size_t)MaxFileSize);

		if (data->cursor != std::streampos())
			file.seekg(data->cursor);

		data->buffer.resize(ssize);

		if (!file.read(data->buffer.data(), ssize))
		{
			debug("did you touch the file? " + data->fileName +
				"\nError reading file.Error flags : " + std::to_string(file.rdstate()));

			clear(data->key);
			return;
		}

		data->startTime = setTime();
		data->cursor = file.tellg();
		file.close();

		_sync.push(data->key);
	}

	bool Sockets::Receive(HTTPData* data, int& total_bytes)
	{
		int bytes_received = 0;
		total_bytes = 0;
		while (true)
		{
			std::vector<char> chunk(ReadSize);
			bytes_received = recv(data->fd, chunk.data(), ReadSize, 0);
			if (bytes_received > 0)
			{
				data->buffer.insert(data->buffer.end(), chunk.begin(), chunk.begin() + bytes_received);
				total_bytes += bytes_received;
				
				if (total_bytes > MaxUploadSize)
					return true;
			}
			else
			{
				if (total_bytes > 0)
					return true;				
#ifdef _WIN32
				if (WSAGetLastError() == WSAEWOULDBLOCK)
#else 
				if (errno == EAGAIN || errno == EWOULDBLOCK)
#endif				
					_sync.push(data->key);
				else
					clear(data->key);

				break;
			}
		}

		return false;
	}
		
	bool Sockets::Send(HTTPData* data, int& len)
	{
		len = send(data->fd, data->buffer.data(), data->buffer.size(), 0);
		if (len <= 0)
		{
#ifdef _WIN32 
			if (WSAGetLastError() == WSAEWOULDBLOCK)
#else 
			if (errno == EAGAIN || errno == EWOULDBLOCK)
#endif							
				_sync.push(data->key);
			else
				clear(data->key);

			return false;
		}

		return true;
	}

	void Sockets::clear(std::string key)
	{
		std::lock_guard<std::mutex> lock(mutex);
		{
			auto data = _clients.find(key);
			if (data != _clients.end())
				clearClient(data->second);
		}
	}

	void Sockets::addClient(HTTPData* data)
	{
		std::lock_guard<std::mutex> lock(mutex);
		{
			if (_sync.getPoolSize() == 0 && _clients.size() > 0)
			{
				for (auto& c : _clients)
					if (c.second->startTime < std::time(nullptr))
						clearClient(c.second);

				if (_clients.size() == 0)
					std::unordered_map<std::string, HTTPData*>().swap(_clients);
			}

			_clients[data->key] = data;
		}
	}

	void Sockets::clearClient(HTTPData* data)
	{
		close(data->fd, false);
		debug(data->key + " with fd " + std::to_string(data->fd) + " session from " + data->IP + " closed, open sessions " + std::to_string(_clients.size() - 1));

		_clients.erase(data->key);
		delete data;
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

	int Sockets::socketClose()
	{
		int status = 0;

#ifdef _WIN32
		status = shutdown(_sock, SD_BOTH);
		if (status == 0)
		{
			status = closesocket(_sock);
			WSACleanup();
		}
#else
		//status = shutdown(_sock, SHUT_RDWR);
		//if (status == 0) { close(_sock); }		
#endif
		return status;
	}
}