
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

		numWorkers = std::thread::hardware_concurrency();
		if (numWorkers == 0) numWorkers = 3; // set a default value		

		std::string str(_host);

		debug("Sockets init done! Configurated " + std::to_string(numWorkers) + " workers and 1 listener ...");
		debug("host: " + str + " and port: " + std::to_string(_port));
	}

	Sockets::~Sockets()
	{
		_isRunning.store(false);			
		_queue.stop();

		socketClose();
		
		debug("socket closed ... ");

		if (_listener.joinable())
			_listener.join();
		else
			_listener.detach();
		
		for (auto& t : _workers)
		{	
			if (t.joinable())
				t.join();
			else
				t.detach();
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

		_listener = std::thread(&Sockets::listener, this);
		
		for (int id = 1; id <= numWorkers; ++id)
			_workers.push_back(std::thread(&Sockets::Worker, this, id));
		
		debug("Socket init succefully");

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
		
		struct timeval timeout;
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;

		fd_set readfds;
		int max_sd = _sock;
		
		while (_isRunning.load())
		{	
			FD_ZERO(&readfds);
			FD_SET(_sock, &readfds);

			int client_fd = -1;
			sockaddr_in client_address = sockaddr_in();
			socklen_t client_len = sizeof(client_address);

			int iResult = select(max_sd + 1, &readfds, NULL, NULL, &timeout);
			if (iResult == -1)
			{
				socketClose();
				break;
			}
			else if (iResult == 0)			
				continue;
						
#ifdef _WIN32				
			client_fd = accept(_sock, reinterpret_cast<sockaddr*>(&client_address), &client_len);
#else
			client_fd = accept4(_sock, reinterpret_cast<sockaddr*>(&client_address), &client_len, SOCK_NONBLOCK);
#endif			
			if (client_fd == INVALID_SOCKET)
			{
				close(client_fd, false);
				continue;
			}

			if (!socketSetBlocking(client_fd, _block))
			{
				close(client_fd, false);
				continue;
			}

			std::string key = std::to_string(client_fd);
			std::unique_ptr<HTTPData> client = std::make_unique<HTTPData>(0, client_fd, key);

			client->IP.resize(INET_ADDRSTRLEN);
			struct in_addr ipAddr = client_address.sin_addr;
			inet_ntop(AF_INET, &ipAddr, client->IP.data(), INET_ADDRSTRLEN);
			client->IP.resize(strlen(client->IP.data()));

			_queue.push(std::move(client));
		}

		debug("listener stopped ... ");
	}

	void Sockets::Worker(int id)
	{
		debug("worker " + std::to_string(id) + " starting ... ");

		while (_isRunning.load())
		{
			std::unique_ptr<HTTPData> data = _queue.pop();
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
				case 3: // Response File
					if (!sendFile(data))
						continue;
					break;
				default: // Not supported or error!
					clearClient(data);
					continue;
			}

			_queue.push(std::move(data));
		}
		
		debug("worker " + std::to_string(id) + " stopped");		
	}
	
	bool Sockets::receiveData(std::unique_ptr<HTTPData>& data)
	{
		int byte_count = 0;
		int result = Receive(data, byte_count);
		if (result == 0)
			return false;
		else if (result == -1)
			return true;
				
		data->phase = 1; //Response					
		_http->elaborateData(data);
			
		//data->buffer.clear();
		//std::string body = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n\r\nHello, World!";		
		//data->buffer.insert(data->buffer.begin(), body.begin(), body.end());
				
		return true;
	}

	bool Sockets::sendData(std::unique_ptr<HTTPData>& data)
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

		data.reset(new HTTPData(0, data->fd, data->key, data->IP));
				
		return true;
	}

	bool Sockets::sendFile(std::unique_ptr<HTTPData>& data)
	{
		if (data->response.fileSize < 0)
		{
			debug("fileSize < 0!");			
			clearClient(data);
			return false;
		}

		int len = 0;
		int result = 0;
		if (!data->response.sentHeader)
		{
			result = Send(data, len);
			if (result == 0)
				return false;
			else if (result == -1)
				return true;

			if (len < data->length)
			{
				debug("are you sure about the lenght of the header?");				
				clearClient(data);
				return false;
			}

			data->response.sentHeader = true;
			data->buffer.clear();
			data->startTime = setTime();
		}
		else if (data->buffer.size() > 0)
		{
			result = Send(data, len);
			if (result == 0)
				return false;
			else if (result == -1)
				return true;			

			data->response.sent += len;
			data->buffer.clear();
		}

		if (data->response.sent >= data->response.fileSize)
		{
			data.reset(new HTTPData(0, data->fd, data->key, data->IP));
			return true;
		}

		std::fstream file(data->response.fileName, std::ios::in | std::ifstream::binary);
		if (file.fail())
		{
			debug("are you sure about the file?");			
			clearClient(data);
			return false;
		}

		size_t ssize = std::min(static_cast<size_t>(data->response.fileSize - data->response.sent), MaxFileSize);

		if (data->response.cursor != std::streampos())
			file.seekg(data->response.cursor);

		data->buffer.resize(ssize);

		if (!file.read(data->buffer.data(), ssize))
		{
			debug("did you touch the file? " + data->response.fileName +
				"\nError reading file.Error flags : " + std::to_string(file.rdstate()));

			clearClient(data);
			return false;
		}

		data->startTime = setTime();
		data->response.cursor = file.tellg();
		file.close();
				
		return true;
	}

	int Sockets::Receive(std::unique_ptr<HTTPData>& data, int& total_bytes)
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
		
	int Sockets::Send(std::unique_ptr<HTTPData>& data, int& len)
	{
		len = send(data->fd, data->buffer.data(), data->buffer.size(), 0);
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

	void Sockets::clearClient(std::unique_ptr<HTTPData>& data)
	{		
		close(data->fd, false);
		debug(data->key + " with fd " + std::to_string(data->fd) + " session from " + data->IP + " closed!");

		data.reset();			
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

	void Sockets::socketClose()
	{		
#ifdef _WIN32
		shutdown(_sock, SD_BOTH);
		closesocket(_sock);
		WSACleanup();			
#endif		
	}
}