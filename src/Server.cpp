/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:54:52 by anemet            #+#    #+#             */
/*   Updated: 2025/12/14 22:50:25 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"



/*
	=================================================================
		SOCKET PROGRAMMING CONCEPTS
	=================================================================

	Before diving into the code, let's understand the key concepts:

	1. FILE DESCRIPTORS (fd)
	------------------------
	When we create a socket, we get an fd like 3, 4, 5, etc.
	(0, 1, 2 are reserved for stdin, stdout, stderr)

	2. ADDRESS FAMILY (AF_INET)
	---------------------------
	AF_INET = IPv4 Internet protocols
	AF_INET6 = IPv6 Internet protocols
	AF_UNIX = Local communication (Unix domain sockets)

	We use AF_INET for standard HTTP over IPv4.

	3. SOCKET TYPE (SOCK_STREAM)
	----------------------------
	SOCK_STREAM = TCP (reliable, ordered, connection-based)
	SOCK_DGRAM = UDP (unreliable, unordered, connectionless)

	HTTP uses TCP, so we use SOCK_STREAM.

	4. struct sockaddr_in
	---------------------
	This structure holds an IPv4 socket address:
	- sin_family: Address family (AF_INET)
	- sin_port: Port number (in network byte order!)
	- sin_addr: IP address (in network byte order!)

	5. BYTE ORDER (htons, htonl)
	----------------------------
	Network byte order is BIG-ENDIAN (most significant byte first).
	Your CPU might be LITTLE-ENDIAN (least significant byte first).

		the Endians are from Jonathan Swift: Gulliver's Travels (1726):
			- Big-Endians crack the egg at the big end
			- Little-Endians crack it at the little end

	htons() = Host TO Network Short (16-bit, for ports)
	htonl() = Host TO Network Long (32-bit, for IP addresses)
	ntohs() = Network TO Host Short
	ntohl() = Network TO Host Long

	Always use these when dealing with ports and IP addresses!

	6. BACKLOG (listen queue)
	-------------------------
	When listen() is called, the kernel creates a queue for incoming
	connections. The backlog parameter sets the maximum queue size.

	If the queue is full, new connections are refused.
	Typical values: 128 (SOMAXCONN is often 128 or 4096).
*/


// =================================================================
//  CONSTRUCTORS AND DESTRUCTOR
// =================================================================

// Default Constructor
// Creates a basic server.
// setConfig() should be called before init() for this to work.
Server::Server() :
	_config(NULL),
	_running(false)
{
}

// Parameterized Constructor
// We store a pointer to the Config, so the Config object
// must remain valid for the lifetime of the Server.
Server::Server(const Config& config) :
    _config(&config),
    _running(false)
{
}

/*
	Destructor

	Ensures all sockets are closed when the Server is destroyed.
	This is critical to avoid file descriptor leaks!

	If we don't close sockets:
	- System resources are wasted
	- Port might remain "in use" after program exits
	- Eventually, system runs out of file descriptors
*/
Server::~Server()
{
	stop();
}


// =================================================================
//  CONFIGURATION
// =================================================================

/*
	setConfig() - Set or update the server configuration

	Allows changing configuration after construction.
	Useful for:
	- Default-constructed servers
	- Configuration reloading (advanced feature)
*/
void Server::setConfig(const Config& config)
{
	_config = &config;
}


// =================================================================
//  INITIALIZATION - THE MAIN SETUP FUNCTION
// =================================================================

/*
	init() - Initialize all listening sockets

	This function performs all the setup needed before the server
	can start accepting connections.

	Algorithm:
	1. Validate we have a configuration
	2. For each server in the config:
		a. Check if we already have a socket for this host:port
		b. If not, create a new listening socket
		c. Store the socket info for later use
	3. Report what we're listening on

	Duplicate Prevention:
	---------------------
	Multiple server blocks might listen on the same port.
	Example:
		server { listen 8080; server_name site1.com; }
		server { listen 8080; server_name site2.com; }

	We only create ONE socket for port 8080, but both configs
	can use it. The request routing will use the Host header
	to determine which server block handles each request.

	Returns:
		true:  All sockets created successfully
		false: At least one socket failed (server can't start)
*/
bool Server::init()
{
	// =========================================
	//  Step 1: Validate Configuration
	// =========================================
	if (!_config)
	{
		std::cerr << "Error: No configuration provided" << std::endl;
		return false;
	}

	const std::vector<ServerConfig>& servers = _config->getServers();

	if (servers.empty())
	{
		std::cerr << "Error: No server blocks in configuration" << std::endl;
		return false;
	}

	std::cout << "\n=== Initializing Server ===" << std::endl;
	std::cout << "Found " << servers.size() << " server block(s) in configuration\n" << std::endl;

	// =========================================
	//  Step 2: Create Listening Sockets
	// =========================================
	/*
		Track which host:port combinations we've already created
		sockets for. This prevents duplicate sockets.

		Key: "host:port" string (e.g., "0.0.0.0:8080")
		Value: Index into _listenSockets vector
	*/
	std::map<std::string, size_t> socketMap;

	for (size_t i = 0; i < servers.size(); ++i)
	{
		const ServerConfig& serverConfig = servers[i];

		// Build unique key for this host:port combination
		std::stringstream keyStream;
		keyStream << serverConfig.host << ":" << serverConfig.port;
		std::string key = keyStream.str();

		std::cout << "Processing server block " << (i + 1)
					<< ": " << key << std::endl;

		// Check if we already have a socket for this address
		if (socketMap.find(key) != socketMap.end())
		{
			std::cout << "  -> Already listening on " << key
						<< " (reusing existing socket)" << std::endl;
			continue;
		}

		// Create new listening socket
		int sockfd = createListenSocket(serverConfig.host, serverConfig.port);

		if (sockfd < 0)
		{
			std::cerr << "Error: Failed to create socket for " << key << std::endl;
			closeAllSockets();  // Clean up any sockets we created
			return false;
		}

		// Store socket information
		ListenSocket listenSocket;
		listenSocket.fd = sockfd;
		listenSocket.host = serverConfig.host;
		listenSocket.port = serverConfig.port;
		listenSocket.serverConfig = &serverConfig;

		_listenSockets.push_back(listenSocket);
		socketMap[key] = _listenSockets.size() - 1;

		std::cout << "  -> Created socket fd=" << sockfd
					<< " for " << key << std::endl;
	}

	// =========================================
	//  Step 3: Report Success
	// =========================================
	std::cout << "\n=== Server Initialized ===" << std::endl;
	std::cout << "Listening on " << _listenSockets.size() << " socket(s):" << std::endl;

	for (size_t i = 0; i < _listenSockets.size(); ++i)
	{
		std::cout << "  - http://" << _listenSockets[i].host
					<< ":" << _listenSockets[i].port
					<< " (fd=" << _listenSockets[i].fd << ")" << std::endl;
	}
	std::cout << std::endl;

	return true;
	}


// =================================================================
//  SOCKET CREATION - THE HEART OF STEP 2.1
// =================================================================

/*
	createListenSocket() - Create and configure a listening socket

	This function performs the complete socket setup sequence:

	┌─────────────────────────────────────────────────────────────┐
	│  socket()                                                   │
	│  Create a new socket endpoint                               │
	│  Returns: file descriptor (int) or -1 on error              │
	├─────────────────────────────────────────────────────────────┤
	│  setsockopt() with SO_REUSEADDR                             │
	│  Allow reusing address immediately after server restart     │
	│  Without this: "Address already in use" errors              │
	├─────────────────────────────────────────────────────────────┤
	│  setsockopt() with SO_REUSEPORT (optional)                  │
	│  Allow multiple sockets to bind to same port                │
	│  Useful for load balancing (not required for webserv)       │
	├─────────────────────────────────────────────────────────────┤
	│  bind()                                                     │
	│  Assign local address (IP:port) to the socket               │
	│  After bind, the socket has a specific identity             │
	├─────────────────────────────────────────────────────────────┤
	│  fcntl() with O_NONBLOCK                                    │
	│  Set socket to non-blocking mode                            │
	│  CRITICAL for webserv - never block on I/O!                 │
	├─────────────────────────────────────────────────────────────┤
	│  listen()                                                   │
	│  Mark socket as passive (ready to accept connections)       │
	│  Creates the connection backlog queue                       │
	└─────────────────────────────────────────────────────────────┘

	Parameters:
		host: IP address to bind to
				"0.0.0.0" = All interfaces (accept from anywhere)
				"127.0.0.1" = Localhost only
				"192.168.1.1" = Specific interface

		port: Port number (1-65535)
				Ports < 1024 require root privileges
				Common: 80 (HTTP), 443 (HTTPS), 8080 (dev)

	Returns:
		>= 0: Valid socket file descriptor
		-1:   Error (check errno for details)
*/
int Server::createListenSocket(const std::string& host, int port)
{
	// =========================================
	//  Step 1: Create Socket
	// =========================================
	/*
		socket(domain, type, protocol)

		domain:   AF_INET for IPv4
		type:     SOCK_STREAM for TCP
		protocol: 0 = let system choose (TCP for SOCK_STREAM)

		Returns:
			File descriptor (>= 0) on success
			-1 on error (sets errno)

		Common errors:
			EACCES:  Permission denied
			EMFILE:  Process file table overflow
			ENFILE:  System file table overflow
			ENOMEM:  Insufficient memory
	*/
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0)
	{
		std::cerr << "socket() failed: " << strerror(errno) << std::endl;
		return -1;
	}

	std::cout << "  [1/5] socket() created fd=" << sockfd << std::endl;

	// =========================================
	//  Step 2: Set Socket Options
	// =========================================
	/*
		setsockopt(sockfd, level, optname, optval, optlen)

		SO_REUSEADDR: Allow reusing a local address immediately
		---------------------------------------------------------
		Without this option:
		- If server crashes or restarts
		- The port stays in TIME_WAIT state for ~2 minutes
		- bind() fails with "Address already in use"

		With SO_REUSEADDR:
		- Can bind immediately after restart
		- Essential for development and production

		level:   SOL_SOCKET (socket-level option)
		optname: SO_REUSEADDR
		optval:  Pointer to option value (1 = enable)
		optlen:  Size of option value
	*/
	int optval = 1;  // Enable the option

	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
	{
		std::cerr << "setsockopt(SO_REUSEADDR) failed: " << strerror(errno) << std::endl;
		close(sockfd);
		return -1;
	}

	std::cout << "  [2/5] setsockopt(SO_REUSEADDR) enabled" << std::endl;

	/*
		SO_REUSEPORT: Allow multiple sockets on same port
		--------------------------------------------------
		This is optional but useful:
		- Multiple processes can bind to same port
		- Kernel load-balances incoming connections
		- Enables multi-process server architecture

		Note: Not available on all systems. We ignore errors.
	*/
#ifdef SO_REUSEPORT
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0)
	{
		// Not critical - just log and continue
		std::cout << "  [*] setsockopt(SO_REUSEPORT) not available (optional)" << std::endl;
	}
	else
	{
		std::cout << "  [*] setsockopt(SO_REUSEPORT) enabled" << std::endl;
	}
#endif

	// =========================================
	//  Step 3: Prepare Address Structure
	// =========================================
	/*
		struct sockaddr_in - IPv4 socket address

		sin_family: Address family (AF_INET for IPv4)
		sin_port:   Port number in NETWORK byte order (use htons!)
		sin_addr:   IP address in NETWORK byte order

		memset: Zero out the structure first (good practice)

		htons(): Convert port from host to network byte order
		Example: 8080 decimal = 0x1F90,
		x86 stores values in memory in little-endian =>
						Host:    0x1F90 (little-endian)
						Network: 0x901F (big-endian)
		Conceptually:
			uint16_t htons(uint16_t x)
			{
				if (host_is_little_endian)
					return byte_swap_16(x);
				else
					return x;
			}

		inet_pton(): Convert IP string to binary format
					"127.0.0.1" -> 0x7F000001
	*/
	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);  // Convert to network byte order!

	/*
		Convert host string to binary IP address

		Special case: "0.0.0.0" means INADDR_ANY
		- Bind to ALL available network interfaces
		- Accept connections from anywhere

		Other values use inet_pton() for conversion
	*/
	if (host == "0.0.0.0" || host.empty())
	{
		serverAddr.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		/*
			inet_pton(af, src, dst)

			af:  Address family (AF_INET)
			src: String representation of IP ("127.0.0.1")
			dst: Where to store binary result

			Returns:
				1:  Success
				0:  Invalid address string
				-1: Error (invalid af)
		*/
		if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) <= 0)
		{
			std::cerr << "Invalid address: " << host << std::endl;
			close(sockfd);
			return -1;
		}
	}

	// =========================================
	//  Step 4: Bind Socket to Address
	// =========================================
	/*
		bind(sockfd, addr, addrlen)

		Assigns a local address to a socket.

		sockfd:		Socket file descriptor
		addr:		Pointer to sockaddr structure (cast from sockaddr_in)
		addrlen:	Size of the address structure

		After bind():
		- Socket has a specific local address
		- Other programs can connect to this address

		Common errors:
			EADDRINUSE:		Address already in use (another program)
			EACCES:			Can't bind to privileged port (< 1024)
			EADDRNOTAVAIL:	Address not available on this machine
	*/
	if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
	{
		std::cerr << "bind() failed for " << host << ":" << port
					<< " - " << strerror(errno) << std::endl;

		// Provide helpful hints for common errors
		if (errno == EADDRINUSE)
		{
			std::cerr << "  Hint: Another process is using this port. "
						<< "Try 'lsof -i :" << port << "' to find it." << std::endl;
		}
		else if (errno == EACCES)
		{
			std::cerr << "  Hint: Ports below 1024 require root privileges. "
						<< "Try a port >= 1024 or run as root." << std::endl;
		}

		close(sockfd);
		return -1;
	}

	std::cout << "  [3/5] bind() to " << host << ":" << port << std::endl;

	// =========================================
	//  Step 5: Set Non-Blocking Mode
	// =========================================
	/*
		Why Non-Blocking Mode?
		----------------------
		In blocking mode:
			accept() blocks until a connection arrives
			recv() blocks until data is available
			send() blocks until buffer space is available

		With multiple clients, blocking is a disaster:
			Client 1 connects → Server calls recv()
			recv() blocks waiting for Client 1's data
			Client 2 tries to connect → Can't! Server is blocked!

		In non-blocking mode:
			All operations return immediately
			If no data available: returns -1 with errno = EAGAIN
			We use poll() to know when operations will succeed

		The webserv subject REQUIRES non-blocking I/O!
		Blocking on read/write = grade of 0!
	*/
	if (!setNonBlocking(sockfd))
	{
		std::cerr << "Failed to set non-blocking mode" << std::endl;
		close(sockfd);
		return -1;
	}

	std::cout << "  [4/5] fcntl() set non-blocking mode" << std::endl;

	// =========================================
	//  Step 6: Start Listening
	// =========================================
	/*
		listen(sockfd, backlog)

		Marks socket as a passive socket that will accept connections.

		sockfd:  Socket file descriptor
		backlog: Maximum length of pending connection queue

		The backlog queue:
		- Holds connections that are waiting to be accept()ed
		- If queue is full, new connections are refused
		- Typical values: 128, 512, or SOMAXCONN

		SOMAXCONN is defined in <sys/socket.h>:
		- Linux: usually 128 or 4096
		- We use 128 as a safe default

		After listen():
		- Socket is ready for accept()
		- Clients can now connect
		- But we must call accept() to actually accept them
	*/
	const int BACKLOG = 128;  // Queue size for pending connections

	if (listen(sockfd, BACKLOG) < 0)
	{
		std::cerr << "listen() failed: " << strerror(errno) << std::endl;
		close(sockfd);
		return -1;
	}

	std::cout << "  [5/5] listen() with backlog=" << BACKLOG << std::endl;

	// =========================================
	//  Success!
	// =========================================
	return sockfd;
}


// =================================================================
//  NON-BLOCKING MODE
// =================================================================

/*
	setNonBlocking() - Configure file descriptor for non-blocking I/O

	Uses fcntl() to manipulate file descriptor flags.

	fcntl(fd, cmd, arg)

	cmd = F_GETFL: Get current flags
	cmd = F_SETFL: Set new flags

	O_NONBLOCK flag:
	- When set, I/O operations return immediately
	- If operation would block, returns -1 with errno = EAGAIN

	This is ESSENTIAL for webserv's event-driven architecture!

	Parameters:
		fd: File descriptor to configure

	Returns:
		true:  Successfully set non-blocking
		false: Failed (check errno)
*/
bool Server::setNonBlocking(int fd)
{
	// Get current flags
	int flags = fcntl(fd, F_GETFL, 0);

	if (flags < 0)
	{
		std::cerr << "fcntl(F_GETFL) failed: " << strerror(errno) << std::endl;
		return false;
	}

	// Add O_NONBLOCK flag
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		std::cerr << "fcntl(F_SETFL) failed: " << strerror(errno) << std::endl;
		return false;
	}

	return true;
}


// =================================================================
//  SERVER LIFECYCLE
// =================================================================

/*
	run() - Main server loop (placeholder for Step 2.2)

	This will be implemented fully in Step 2.2 (Poll/Epoll).

	For now, it just demonstrates that sockets are working
	by running a simple accept loop.
*/
void Server::run()
{
	if (_listenSockets.empty())
	{
		std::cerr << "Error: No listening sockets. Call init() first." << std::endl;
		return;
	}

	_running = true;
	std::cout << "\n=== Server Running ===" << std::endl;
	std::cout << "Press Ctrl+C to stop\n" << std::endl;

	/*
		Simple demonstration loop.
		In Step 2.2, this will be replaced with a proper poll/epoll loop.

		WARNING: This blocking loop is only for testing!
		It doesn't handle multiple clients properly.
	*/
	while (_running)
	{
		// Try to accept on first socket (blocking for demo)
		struct sockaddr_in clientAddr;
		socklen_t clientLen = sizeof(clientAddr);

		int clientFd = accept(_listenSockets[0].fd,
								(struct sockaddr*)&clientAddr,
								&clientLen);

		if (clientFd >= 0)
		{
			// Get client IP for logging
			char clientIP[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);

			std::cout << "Accepted connection from " << clientIP
						<< ":" << ntohs(clientAddr.sin_port)
						<< " (fd=" << clientFd << ")" << std::endl;

			// Send a simple response (for testing)
			const char* response =
				"HTTP/1.1 200 OK\r\n"
				"Content-Type: text/plain\r\n"
				"Content-Length: 45\r\n"
				"Connection: close\r\n"
				"\r\n"
				"Hello from webserv! Socket setup is working!\n";

			send(clientFd, response, strlen(response), 0);
			close(clientFd);

			std::cout << "Sent response and closed connection\n" << std::endl;
		}
		else if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			// Real error (not just "would block")
			std::cerr << "accept() error: " << strerror(errno) << std::endl;
		}

		// Small delay to prevent CPU spin
		usleep(10000);  // 10ms
	}
}

/*
	stop() - Gracefully shut down the server

	Closes all sockets and marks server as stopped.
	Safe to call multiple times.
*/
void Server::stop()
{
	if (_running)
	{
		std::cout << "\n=== Stopping Server ===" << std::endl;
		_running = false;
	}

	closeAllSockets();
}

/*
	closeAllSockets() - Close all listening sockets

	Important for cleanup to avoid resource leaks.
	Each socket holds a file descriptor which is a limited resource.
*/
void Server::closeAllSockets()
{
	for (size_t i = 0; i < _listenSockets.size(); ++i)
	{
		if (_listenSockets[i].fd >= 0)
		{
			std::cout << "Closing socket fd=" << _listenSockets[i].fd
						<< " (" << _listenSockets[i].host
						<< ":" << _listenSockets[i].port << ")" << std::endl;

			close(_listenSockets[i].fd);
			_listenSockets[i].fd = -1;
		}
	}

	_listenSockets.clear();
}


// =================================================================
//  GETTERS
// =================================================================

bool Server::isRunning() const
{
	return _running;
}

const std::vector<ListenSocket>& Server::getListenSockets() const
{
	return _listenSockets;
}
