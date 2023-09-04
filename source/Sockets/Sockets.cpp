
#include "sockets.hpp"

namespace Orpy
{
	std::shared_ptr<ISockets> _sock = nullptr;

	void setSockets(const std::string& host, const int& port)
	{
		_sock = std::make_shared<Sockets>(host, port);
	}

	Sockets::Sockets(const std::string& host, const int& port) : _host(host), _port(port)
	{
		numWorkers = std::thread::hardware_concurrency();
		if (numWorkers == 0) numWorkers = 3; // set a default value		

		_queues.resize(numWorkers);

		debug("Sockets init done! Configurated " + std::to_string(numWorkers) + " workers and 1 listener ...");
		debug("host: " + host + " and port: " + std::to_string(port));
	}

	Sockets::~Sockets()
	{
		_isRunning.store(false);			
		
		socketClose();
		
		debug("socket closed ... ");

		if (_listener.joinable())
			_listener.join();

		for (auto& q : _queues)
			q.reset();
		
		for (auto& t : _workers)			
			if (t.joinable())
				t.join();
						
		debug("Sockets unloaded...");		
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

		_listener = std::thread(&Sockets::listener, this);

		for (int id = 0; id < numWorkers; ++id)
		{
			_queues.insert(_queues.begin() + id, std::make_unique<synchronization::Pool<http::Data>>());
			_workers.emplace_back(&Sockets::Worker, this, id);
		}
						
		debug("Socket init succefully");

		return true;
	}

	bool Sockets::setSocket(int port, socket_t sock, sockaddr_in& socketAddress, int& socketAddress_len)
	{
		memset(&socketAddress, 0, sizeof(sockaddr_in));
		socketAddress.sin_family = AF_INET;
		socketAddress.sin_port = htons(port);
		socketAddress.sin_addr.s_addr = inet_pton(AF_INET, _host.c_str(), &socketAddress.sin_addr);
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
			printf("Problems with SetBlocking\n");
			socketClose();
			return false;
		}

		return true;
	}

	void Sockets::listener()
	{
		debug("ready to listen ... ");
		
		int	max_sd = _sock, accepted, client_fd;
		fd_set listfds, readfds;
		FD_ZERO(&listfds);
		FD_ZERO(&readfds);
		FD_SET(_sock, &listfds);

		sockaddr_in client_address;
		socklen_t client_len;

		struct timeval timeout;
		timeout.tv_sec = 10;
		timeout.tv_usec = 0;
		
		while (_isRunning.load())
		{				
			readfds = listfds;

			client_fd = -1;

			// Reset clientAddress for the next iteration
			memset(&client_address, 0, sizeof(client_address));
			client_len = sizeof(client_address);
			
			accepted = select(max_sd + 1, &readfds, NULL, NULL, &timeout);
			if (accepted == -1)
			{
				socketClose();
				break;
			}
			else if (accepted == 0)
				continue;
			
#ifdef _WIN32				
			client_fd = accept(_sock, reinterpret_cast<sockaddr*>(&client_address), &client_len);								
#else
			client_fd = accept4(_sock, reinterpret_cast<sockaddr*>(&client_address), &client_len, SOCK_NONBLOCK);
#endif			
			if (client_fd == INVALID_SOCKET)
			{
				closeClient(client_fd);
				continue;
			}
			
			if (!socketSetBlocking(client_fd, _block))
			{
				closeClient(client_fd);
				continue;
			}
			
			std::string key = std::to_string(client_fd);			
			std::unique_ptr<http::Data> client = std::make_unique<http::Data>(0, client_fd, key);

			client->IP.resize(INET_ADDRSTRLEN);						
			struct in_addr ipAddr = client_address.sin_addr;			
			inet_ntop(AF_INET, &ipAddr, client->IP.data(), INET_ADDRSTRLEN);			
			client->IP.resize(strlen(client->IP.data()));
			
			push(std::move(client));
		}

		debug("listener stopped ... ");
	}

	void Sockets::Worker(int id)
	{
		debug("worker " + std::to_string(id) + " starting ... ");
		
		while (_isRunning.load())
		{
			std::unique_ptr<http::Data> data = _queues.at(id)->pop();
			if (!data)
				break;
						
			if (data->startTime < std::time(nullptr))
			{
				clearClient(data);
				continue;
			}

			switch (data->phase)
			{
				case 0: // Request
					if (!receiveData(data))
						continue;
					break;
				case 1: // Response
					if (!sendData(data))
						continue;
					break;
				case 2: // Response File
					if (!sendFile(data))
						continue;
					break;
				default: // Not supported or error!
					clearClient(data);
					continue;				
			}

			push(std::move(data), id);
		}
		
		debug("worker " + std::to_string(id) + " stopped");		
	}

	void Sockets::push(std::unique_ptr<http::Data> data, int id)
	{		
		{
			std::unique_lock<std::shared_mutex> lock(_mutex);
			if (!id)
			{
				_queues.at(currWorker)->push(std::move(data));

				++currWorker;
				if (currWorker == numWorkers)
					currWorker = 0;
			}
			else
				_queues.at(id)->push(std::move(data));
		}
	}
	
	bool Sockets::receiveData(std::unique_ptr<http::Data>& data)
	{
		int byte_count = 0;
		int result = Receive(data, byte_count);
		if (result == 0)
			return false;
		else if (result == -1)
			return true;
		
		data->phase = 1; //Response	

		_core->elaborateData(data);
		
		return true;
	}

	bool Sockets::sendData(std::unique_ptr<http::Data>& data)
	{
		int byte_count = 0;
		int result = Send(data, byte_count);
		if (result == 0)
			return false;
		else if (result == -1)
			return true;
		
		if(data->error)
		{
			clearClient(data);
			return false;
		}

		data.reset(new http::Data(0, data->fd, data->key, data->IP));
		
		return true;
	}

	bool Sockets::sendFile(std::unique_ptr<http::Data>& data)
	{
		if (data->response.content.empty())
		{
			size_t ssize = std::min(static_cast<size_t>(data->response.fileSize - data->response.sent), MaxFileSize);
			data->response.content.resize(ssize);

			if (!data->response.file.is_open())
			{
				data->response.file.open(data->response.fileName, std::ios::in | std::ifstream::binary);
				if (data->response.file.fail())
				{
					debug("are you sure about the file?");
					return false;
				}
			}

			if (data->response.cursor != std::streampos())
				data->response.file.seekg(data->response.cursor);

			if (!data->response.file.read(data->response.content.data(), ssize))
			{
				debug("did you touch the file? " + data->response.fileName +
					"\n	Error reading file.Error flags : " + std::to_string(data->response.file.rdstate()));

				return false;
			}

			data->buffer.insert(data->buffer.end(), data->response.content.begin(), data->response.content.end());

			data->response.cursor = data->response.file.tellg();
		}

		if (!data->buffer.empty())
		{
			int len = 0;
			switch (Send(data, len))
			{
			case 1: break;
			case -1:
				return true;
			default:
				return false;
			}

			if (!data->response.sentHeader)
			{
				data->response.sentHeader = true;
				len -= data->length; // Remove the header from sent Data
			}

			data->response.sent += len;
			data->buffer.clear();
			data->response.content.clear();

			data->startTime = setTime();
		}

		if (data->response.sent >= data->response.fileSize)
			data.reset(new http::Data(0, data->fd, data->key, data->IP));

		return true;
	}

	int Sockets::Receive(std::unique_ptr<http::Data>& data, int& total_bytes)
	{
		std::string chunk;
		chunk.resize(ReadSize);
		int bytes_received = 0;
		total_bytes = 0;
		while (true)
		{			
			bytes_received = recv(data->fd, chunk.data(), chunk.size(), 0);
			if (bytes_received > 0)
			{
				data->buffer.insert(data->buffer.end(), chunk.begin(), chunk.begin() + bytes_received);				
				total_bytes += bytes_received;
				
				if (total_bytes > MaxUploadSize)
					return 1;
			}
			else
			{
				if (bytes_received != 0 && total_bytes > 0)
					return 1;				
#ifdef _WIN32
				if (WSAGetLastError() == WSAEWOULDBLOCK)
#else 
				if (errno == EAGAIN || errno == EWOULDBLOCK)
#endif				
					return -1;
								
				break;
			}
		}

		clearClient(data);
		return 0;
	}
		
	int Sockets::Send(std::unique_ptr<http::Data>& data, int& len)
	{
		len = send(data->fd, data->buffer.c_str(), data->buffer.size(), 0);
		if (len <= 0)
		{
#ifdef _WIN32 
			if (WSAGetLastError() == WSAEWOULDBLOCK)
#else 
			if (errno == EAGAIN || errno == EWOULDBLOCK)
#endif						
				return -1;
				
			clearClient(data);
			return 0;
		}

		return 1;
	}

	void Sockets::clearClient(std::unique_ptr<http::Data>& data)
	{		
		closeClient(data->fd);
		debug(data->key + " with fd " + std::to_string(data->fd) + " session from " + data->IP + " closed!");

		data.reset();		
	}

	void Sockets::closeClient(const socket_t& fd)
	{
#ifdef _WIN32  
		closesocket(fd);
#else           
		shutdown(fd, SHUT_WR);
		close(fd);
#endif
	}

	void Sockets::socketClose()
	{	
#ifdef _WIN32		
		shutdown(_sock, SD_BOTH);
		closesocket(_sock);
		WSACleanup();			
#endif		
	}
}