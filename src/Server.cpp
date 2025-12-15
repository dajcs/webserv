/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:54:52 by anemet            #+#    #+#             */
/*   Updated: 2025/12/15 15:17:41 by anemet           ###   ########.fr       */
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
	An x86 CPU is LITTLE-ENDIAN (stores least significant byte first).

	The Endians are from Jonathan Swift: Gulliver's Travels (1726):
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

/*
	=====================
		EPOLL CONCEPTS
	=====================

	What is epoll?
	--------------
	epoll is a Linux-specific I/O event notification mechanism.
	It's designed for applications that need to monitor many file
	descriptors efficiently (like web servers).

	The Three Epoll Syscalls:
	-------------------------

	1. epoll_create1(flags)
		- Creates a new epoll instance
		- Returns a file descriptor for the instance
		- flags: 0 or EPOLL_CLOEXEC (close on exec)
		- The returned fd must be closed with close()

	2. epoll_ctl(epfd, op, fd, event)
		- Controls the epoll instance
		- epfd: epoll file descriptor from epoll_create1
		- op: EPOLL_CTL_ADD, EPOLL_CTL_MOD, or EPOLL_CTL_DEL
		- fd: file descriptor to add/modify/remove
		- event: pointer to epoll_event struct

	3. epoll_wait(epfd, events, maxevents, timeout)
		- Waits for events on the epoll instance
		- epfd: epoll file descriptor
		- events: array to store returned events
		- maxevents: max events to return
		- timeout: -1 block, 0 return immediately, >0 wait ms
		- Returns: number of ready FDs, 0 on timeout, -1 on error

	The epoll_event Structure:
	--------------------------
	struct epoll_event {
		uint32_t		events;	// Epoll events (EPOLLIN, EPOLLOUT, etc.)
		epoll_data_t	data;	// User data variable
	};

	The data field is a union - we typically use data.fd to store
	the file descriptor, so we know which FD triggered the event.

	Why is epoll faster than poll/select?
	-------------------------------------
	1. Kernel maintains the interest list - no copying every call
	2. Only returns FDs that are ready - O(1) per event
	3. Uses efficient red-black tree internally
	4. Edge-triggered mode avoids redundant notifications
*/

// =================================================================
//  CONSTRUCTORS AND DESTRUCTOR
// =================================================================

// Default Constructor
// Creates a basic server.
// setConfig() should be called before init() for this to work.
Server::Server() : _config(NULL),
				   _running(false),
				   _epollFd(-1)
{
}

// Parameterized Constructor
// We store a pointer to the Config, so the Config object
// must remain valid for the lifetime of the Server.
Server::Server(const Config &config) : _config(&config),
									   _running(false),
									   _epollFd(-1)
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
void Server::setConfig(const Config &config)
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

	const std::vector<ServerConfig> &servers = _config->getServers();

	if (servers.empty())
	{
		std::cerr << "Error: No server blocks in configuration" << std::endl;
		return false;
	}

	std::cout << "\n=== Initializing Server ===" << std::endl;
	std::cout << "Found " << servers.size() << " server block(s) in configuration\n"
			  << std::endl;

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
		const ServerConfig &serverConfig = servers[i];

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
			closeAllSockets(); // Clean up any sockets we created
			return false;
		}

		// Store socket information
		ListenSocket listenSocket;
		listenSocket.fd = sockfd;
		listenSocket.host = serverConfig.host;
		listenSocket.port = serverConfig.port;
		listenSocket.serverConfig = &serverConfig;

		_listenSockets.push_back(listenSocket);
		_listenFds.insert(sockfd); // Add to quick lookup set
		socketMap[key] = _listenSockets.size() - 1;

		std::cout << "  -> Created socket fd=" << sockfd
				  << " for " << key << std::endl;
	}

	// =========================================
	//  Step 3: Initialize Epoll
	// =========================================
	if (!initEpoll())
	{
		std::cerr << "Error: Failed to initialize epoll" << std::endl;
		closeAllSockets();
		return false;
	}

	// =========================================
	//  Step 4: Report Success
	// =========================================
	std::cout << "\n=== Server Initialized ===" << std::endl;
	std::cout << "Listening on " << _listenSockets.size() << " socket(s):" << std::endl;

	for (size_t i = 0; i < _listenSockets.size(); ++i)
	{
		std::cout << "  - http://" << _listenSockets[i].host
				  << ":" << _listenSockets[i].port
				  << " (fd=" << _listenSockets[i].fd << ")" << std::endl;
	}
	std::cout << "\nEpoll fd=" << _epollFd << " initialized" << std::endl;
	std::cout << std::endl;

	return true;
}

// =================================================================
//  EPOLL INITIALIZATION (Step 2.2)
// =================================================================

/*
	initEpoll() - Create and configure the epoll instance

	This function:
	1. Creates the epoll instance using epoll_create1()
	2. Adds all listening sockets to the epoll interest list
	3. Sets up monitoring for incoming connections (EPOLLIN)

	EPOLLIN for listening sockets:
	------------------------------
	For a listening socket, EPOLLIN means "a connection is waiting".
	When epoll_wait() returns with EPOLLIN on a listening socket,
	we call accept() to get the new client connection.

	Returns:
		true:  Epoll initialized successfully
		false: Failed to create/configure epoll
*/
bool Server::initEpoll()
{
	std::cout << "\n--- Initializing Epoll ---" << std::endl;

	// =========================================
	//  Step 1: Create Epoll Instance
	// =========================================
	/*
		epoll_create1(flags)

		Creates a new epoll instance and returns a file descriptor.

		flags:
			0:             Standard behavior
			EPOLL_CLOEXEC: Close FD automatically on exec()

		The returned file descriptor is used for all subsequent
		epoll operations. It must be closed with close() when done.

		Note: epoll_create(size) is deprecated. epoll_create1()
		ignores the size hint and is the modern API.

		Returns:
			>= 0: Valid epoll file descriptor
			-1:   Error (check errno)

		Common errors:
			EMFILE: Process file table full
			ENFILE: System file table full
			ENOMEM: Insufficient kernel memory
	*/
	_epollFd = epoll_create1(0);

	if (_epollFd < 0)
	{
		std::cerr << "epoll_create1() failed: " << strerror(errno) << std::endl;
		return false;
	}

	std::cout << "  [1/2] epoll_create1() -> fd=" << _epollFd << std::endl;

	// =========================================
	//  Step 2: Add Listening Sockets to Epoll
	// =========================================
	/*
		For each listening socket, we add it to epoll with:
		- EPOLLIN: Notify us when connections are waiting

		When a client connects, the listening socket becomes
		"readable" - meaning accept() will succeed.
	*/
	for (size_t i = 0; i < _listenSockets.size(); ++i)
	{
		int listenFd = _listenSockets[i].fd;

		if (!addToEpoll(listenFd, EPOLLIN))
		{
			std::cerr << "Failed to add listening socket " << listenFd
					  << " to epoll" << std::endl;
			closeEpoll();
			return false;
		}

		std::cout << "  [2/2] Added listen socket fd=" << listenFd
				  << " to epoll (port " << _listenSockets[i].port << ")"
				  << std::endl;
	}

	std::cout << "--- Epoll Initialized ---\n"
			  << std::endl;
	return true;
}

/*
	addToEpoll() - Add a file descriptor to epoll monitoring

	Uses EPOLL_CTL_ADD to register a new FD with epoll.

	The epoll_event structure:
	--------------------------
	struct epoll_event {
		uint32_t events;      // Events to monitor
		epoll_data_t data;    // User data (we use data.fd)
	};

	We store the FD in data.fd so that when epoll_wait() returns
	an event, we know which FD it's for.

	Parameters:
		fd:     File descriptor to monitor
		events: Event flags (EPOLLIN, EPOLLOUT, etc.)

	Returns:
		true:  Successfully added
		false: Failed to add
*/
bool Server::addToEpoll(int fd, uint32_t events)
{
	struct epoll_event ev;

	/*
		Initialize the epoll_event structure

		events: What events to monitor
			EPOLLIN  - Ready for reading
			EPOLLOUT - Ready for writing
			EPOLLERR - Error occurred (always monitored)
			EPOLLHUP - Hang up occurred (always monitored)

		data.fd: Store the FD so we can identify it later
	*/
	ev.events = events;
	ev.data.fd = fd;

	/*
		epoll_ctl(epfd, op, fd, event)

		EPOLL_CTL_ADD: Register the target fd with epoll
		EPOLL_CTL_MOD: Change the event for registered fd
		EPOLL_CTL_DEL: Remove (deregister) fd from epoll

		Returns 0 on success, -1 on error

		Common errors:
			EBADF:  Invalid fd or epfd
			EEXIST: fd already registered (for ADD)
			ENOENT: fd not registered (for MOD/DEL)
			ENOMEM: No memory available
	*/
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &ev) < 0)
	{
		std::cerr << "epoll_ctl(ADD) failed for fd=" << fd
				  << ": " << strerror(errno) << std::endl;
		return false;
	}

	return true;
}

/*
	modifyEpoll() - Change the events monitored for an FD

	Used when we need to change what we're watching for.
	Example: After reading a request, we want to wait for
	write-ready to send the response.

	Parameters:
		fd:     File descriptor already in epoll
		events: New event flags

	Returns:
		true:  Successfully modified
		false: Failed to modify
*/
bool Server::modifyEpoll(int fd, uint32_t events)
{
	struct epoll_event ev;
	ev.events = events;
	ev.data.fd = fd;

	if (epoll_ctl(_epollFd, EPOLL_CTL_MOD, fd, &ev) < 0)
	{
		std::cerr << "epoll_ctl(MOD) failed for fd=" << fd
				  << ": " << strerror(errno) << std::endl;
		return false;
	}

	return true;
}

/*
	removeFromEpoll() - Remove an FD from epoll monitoring

	Called before closing a socket to clean up the epoll state.

	Note: Since Linux 2.6.9, closing an FD automatically removes
	it from epoll. But explicit removal is good practice and
	makes the code clearer.

	Parameters:
		fd: File descriptor to remove

	Returns:
		true:  Successfully removed
		false: Failed to remove
*/
bool Server::removeFromEpoll(int fd)
{
	/*
		For EPOLL_CTL_DEL, the event parameter is ignored
		since Linux 2.6.9, but we pass a valid pointer
		for compatibility with older kernels.
	*/
	struct epoll_event ev;
	ev.events = 0;
	ev.data.fd = fd;

	if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, &ev) < 0)
	{
		// ENOENT is okay - fd might already be removed
		if (errno != ENOENT)
		{
			std::cerr << "epoll_ctl(DEL) failed for fd=" << fd
					  << ": " << strerror(errno) << std::endl;
			return false;
		}
	}

	return true;
}

/*
	closeEpoll() - Close the epoll instance

	Releases the epoll file descriptor.
	Should be called during shutdown.
*/
void Server::closeEpoll()
{
	if (_epollFd >= 0)
	{
		std::cout << "Closing epoll fd=" << _epollFd << std::endl;
		close(_epollFd);
		_epollFd = -1;
	}
}

// =================================================================
//  MAIN EVENT LOOP (Step 2.2 - The Heart of the Server!)
// =================================================================

/*
	run() - The main server event loop

	The event loop:
	1. Waits for events using epoll_wait()
	2. Processes each event:
		- Listening socket: Accept new connection
		- Client socket: Handle read/write/errors
	3. Periodically cleans up timed-out connections
	4. Repeats until stop() is called

	The Event Loop Pattern:
	-----------------------
	┌────────────────────────────────────────────┐
	│                                            │
	│    ┌─────────────────────────────────┐     │
	│    │      epoll_wait()               │     │
	│    │  (blocks until events ready)    │     │
	│    └─────────────────────────────────┘     │
	│                   │                        │
	│                   ▼                        │
	│    ┌─────────────────────────────────┐     │
	│    │    For each ready FD:           │     │
	│    │    - Is it a listen socket?     │     │
	│    │      → accept() new connection  │     │
	│    │    - Is it a client socket?     │     │
	│    │      → handle read/write        │     │
	│    └─────────────────────────────────┘     │
	│                   │                        │
	│                   ▼                        │
	│    ┌─────────────────────────────────┐     │
	│    │    Cleanup timed-out clients    │     │
	│    └─────────────────────────────────┘     │
	│                   │                        │
	│                   ▼                        │
	│              Loop back                     │
	│                                            │
	└────────────────────────────────────────────┘

	This is non-blocking:
	- We never wait for a single client
	- All I/O happens only when ready
	- Thousands of clients can be handled efficiently
*/
void Server::run()
{
	// Validate we're ready to run
	if (_listenSockets.empty())
	{
		std::cerr << "Error: No listening sockets. Call init() first." << std::endl;
		return;
	}

	if (_epollFd < 0)
	{
		std::cerr << "Error: Epoll not initialized. Call init() first." << std::endl;
		return;
	}

	_running = true;
	std::cout << "\n=== Server Running (epoll event loop) ===" << std::endl;
	std::cout << "Press Ctrl+C to stop\n"
			  << std::endl;

	/*
		Event buffer for epoll_wait()

		epoll_wait() fills this array with events that occurred.
		We process MAX_EPOLL_EVENTS at a time, but there could
		be more - they'll be returned in the next call.
	*/
	struct epoll_event events[MAX_EPOLL_EVENTS];

	/*
		Track time for periodic cleanup
		We don't want to check timeouts on every loop iteration
		(that would be wasteful), so we do it every few seconds.
	*/
	time_t lastCleanup = time(NULL);
	const int CLEANUP_INTERVAL_SEC = 10;

	// =========================================
	//  THE MAIN EVENT LOOP
	// =========================================
	while (_running)
	{
		/*
			epoll_wait(epfd, events, maxevents, timeout)

			Waits for events on the epoll instance.

			Parameters:
				epfd:      Epoll file descriptor
				events:    Array to store returned events
				maxevents: Maximum events to return (buffer size)
				timeout:   -1 = block forever
						   0  = return immediately (polling)
						   >0 = wait up to N milliseconds

			Returns:
				>0: Number of FDs with events
				0:  Timeout occurred (no events)
				-1: Error occurred

			The returned events array contains:
				events[i].events: What happened (EPOLLIN, EPOLLOUT, etc.)
				events[i].data.fd: Which file descriptor

			NOTE: We use a timeout so the loop can check _running
			periodically and handle cleanup. Without timeout,
			Ctrl+C might not work smoothly.
		*/
		int numEvents = epoll_wait(_epollFd, events, MAX_EPOLL_EVENTS, EPOLL_TIMEOUT_MS);

		// Handle epoll_wait errors
		if (numEvents < 0)
		{
			// EINTR means interrupted by signal (e.g., Ctrl+C)
			// This is normal and we should just continue
			if (errno == EINTR)
			{
				std::cout << "epoll_wait interrupted by signal" << std::endl;
				continue;
			}

			// Other errors are real problems
			std::cerr << "epoll_wait() failed: " << strerror(errno) << std::endl;
			break;
		}

		// Log event count (useful for debugging, can be removed later)
		if (numEvents > 0)
		{
			std::cout << "[epoll] " << numEvents << " event(s) ready" << std::endl;
		}

		// =========================================
		//  Process Each Event
		// =========================================
		for (int i = 0; i < numEvents; ++i)
		{
			int fd = events[i].data.fd;
			uint32_t eventMask = events[i].events;

			// Log the event (for debugging)
			std::cout << "  Event on fd=" << fd << ": ";
			if (eventMask & EPOLLIN)
				std::cout << "EPOLLIN ";
			if (eventMask & EPOLLOUT)
				std::cout << "EPOLLOUT ";
			if (eventMask & EPOLLERR)
				std::cout << "EPOLLERR ";
			if (eventMask & EPOLLHUP)
				std::cout << "EPOLLHUP ";
			std::cout << std::endl;

			// -----------------------------------------
			//  Case 1: Event on a LISTENING socket
			// -----------------------------------------
			if (isListenSocket(fd))
			{
				/*
					A listening socket has EPOLLIN = connection waiting
					We need to accept() the new connection.

					NOTE: There might be multiple connections waiting!
					In level-triggered mode, epoll will notify us again
					if there are more. In edge-triggered mode, we'd need
					to loop until accept() returns EAGAIN.
				*/
				if (eventMask & EPOLLIN)
				{
					int clientFd = acceptNewConnection(fd);
					if (clientFd >= 0)
					{
						std::cout << "  -> Accepted new client fd=" << clientFd << std::endl;
					}
				}

				// Handle errors on listening socket (rare but possible)
				if (eventMask & (EPOLLERR | EPOLLHUP))
				{
					std::cerr << "Error on listening socket fd=" << fd << std::endl;
					// Don't close listen socket - try to recover
				}
			}
			// -----------------------------------------
			//  Case 2: Event on a CLIENT socket
			// -----------------------------------------
			else
			{
				/*
					Client socket events:
					- EPOLLIN:  Data available to read
					- EPOLLOUT: Ready to write (buffer has space)
					- EPOLLERR: Error on socket
					- EPOLLHUP: Client disconnected
				*/
				bool keepConnection = handleClientEvent(fd, eventMask);

				if (!keepConnection)
				{
					closeClientConnection(fd);
				}
			}
		}

		// =========================================
		//  Periodic Cleanup
		// =========================================
		time_t now = time(NULL);
		if (now - lastCleanup >= CLEANUP_INTERVAL_SEC)
		{
			cleanupTimedOutConnections();
			lastCleanup = now;
		}
	}

	std::cout << "\n=== Event Loop Ended ===" << std::endl;
}



// =================================================================
//  CONNECTION HANDLING
// =================================================================

/*
	acceptNewConnection() - Accept an incoming client connection

	Called when a listening socket has EPOLLIN (connection waiting).

	The accept() call:
	------------------
	accept(sockfd, addr, addrlen)

	Parameters:
		sockfd:  Listening socket
		addr:    Where to store client's address
		addrlen: Size of addr buffer (in/out parameter)

	Returns:
		>= 0: New socket FD for this client
		-1:   Error (check errno)

	IMPORTANT: The returned socket is a NEW file descriptor.
	The original listening socket continues to listen.

	We then:
	1. Set the new socket to non-blocking mode
	2. Add it to epoll for monitoring
	3. Store client info for tracking

	Parameters:
		listenFd: The listening socket with pending connection

	Returns:
		>= 0: Client socket FD
		-1:   Error (connection not accepted)
*/
int Server::acceptNewConnection(int listenFd)
{
	// Prepare to store client's address
	struct sockaddr_in clientAddr;
	socklen_t clientLen = sizeof(clientAddr);

	/*
		accept() creates a new connected socket

		The new socket:
		- Has its own file descriptor
		- Is connected to the specific client
		- Inherits some properties from listening socket

		The listening socket:
		- Continues listening for more connections
		- Is not affected by this call
	*/
	int clientFd = accept(listenFd, (struct sockaddr*)&clientAddr, &clientLen);

	if (clientFd < 0)
	{
		// EAGAIN/EWOULDBLOCK = no connection ready (normal in non-blocking mode)
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			return -1;  // Not an error, just no connection ready
		}

		std::cerr << "accept() failed: " << strerror(errno) << std::endl;
		return -1;
	}

	// Get client info for logging
	char clientIP[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
	int clientPort = ntohs(clientAddr.sin_port);

	std::cout << "  New connection from " << clientIP << ":" << clientPort
				<< " (fd=" << clientFd << ")" << std::endl;

	// =========================================
	//  Step 1: Set Non-Blocking Mode
	// =========================================
	/*
		CRITICAL: Client sockets MUST be non-blocking!

		If we don't set non-blocking:
		- recv() could block waiting for data
		- send() could block if buffer is full
		- The entire server would freeze!

		With non-blocking:
		- recv()/send() return immediately
		- If not ready: errno = EAGAIN
		- We use epoll to know when ready
	*/
	if (!setNonBlocking(clientFd))
	{
		std::cerr << "Failed to set client socket non-blocking" << std::endl;
		close(clientFd);
		return -1;
	}

	// =========================================
	//  Step 2: Add to Epoll
	// =========================================
	/*
		Monitor the client socket for:
		- EPOLLIN: Data available to read (client sent something)

		We don't monitor EPOLLOUT yet because:
		- We haven't read the request yet
		- EPOLLOUT would trigger immediately (buffer is empty)
		- We'll add EPOLLOUT when we have data to send
	*/
	if (!addToEpoll(clientFd, EPOLLIN))
	{
		std::cerr << "Failed to add client to epoll" << std::endl;
		close(clientFd);
		return -1;
	}

	// =========================================
	//  Step 3: Store Client Info
	// =========================================
	/*
		Track this connection:
		- For timeout management
		- For logging/debugging
		- For routing to correct server config
	*/
	const ListenSocket* listenSock = getListenSocketByFd(listenFd);

	ClientInfo clientInfo;
	clientInfo.fd = clientFd;
	clientInfo.connectTime = time(NULL);
	clientInfo.lastActivity = time(NULL);
	clientInfo.serverPort = listenSock ? listenSock->port : 0;
	clientInfo.clientIP = clientIP;
	clientInfo.clientPort = clientPort;

	_clients[clientFd] = clientInfo;

	std::cout << "  Client fd=" << clientFd << " added to epoll (total clients: "
				<< _clients.size() << ")" << std::endl;

	return clientFd;
}


/*
	handleClientEvent() - Process events on a client socket

	This is where we handle actual client communication.
	For Step 2.2, we implement a simple echo/demo.
	Full HTTP handling comes in later steps.

	Parameters:
		clientFd: The client socket with events
		events:   The epoll events that occurred

	Returns:
		true:  Keep the connection open
		false: Close the connection
*/
bool Server::handleClientEvent(int clientFd, uint32_t events)
{
	// Find client info
	std::map<int, ClientInfo>::iterator it = _clients.find(clientFd);
	if (it == _clients.end())
	{
		std::cerr << "Unknown client fd=" << clientFd << std::endl;
		return false;  // Close unknown connections
	}

	ClientInfo& client = it->second;

	// =========================================
	//  Handle Errors and Disconnection
	// =========================================
	/*
		EPOLLERR: Error on socket
		EPOLLHUP: Client hung up (disconnected)

		These events always close the connection.
	*/
	if (events & (EPOLLERR | EPOLLHUP))
	{
		std::cout << "  Client fd=" << clientFd;
		if (events & EPOLLERR) std::cout << " error";
		if (events & EPOLLHUP) std::cout << " hung up";
		std::cout << std::endl;
		return false;  // Close connection
	}

	// =========================================
	//  Handle Readable (EPOLLIN)
	// =========================================
	/*
		Client sent data - read it!

		For Step 2.2, we do a simple read and send a demo response.
		Full HTTP parsing comes in Step 4 (Request.cpp).
	*/
	if (events & EPOLLIN)
	{
		char buffer[4096];
		ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

		if (bytesRead < 0)
		{
			// EAGAIN = no data ready (shouldn't happen after EPOLLIN)
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				return true;  // Keep connection, try again later
			}

			std::cerr << "  recv() error on fd=" << clientFd
						<< ": " << strerror(errno) << std::endl;
			return false;  // Close on error
		}

		if (bytesRead == 0)
		{
			// Client closed connection gracefully
			std::cout << "  Client fd=" << clientFd << " closed connection" << std::endl;
			return false;
		}

		// Update activity timestamp
		client.lastActivity = time(NULL);

		// Null-terminate for printing (assuming text data)
		buffer[bytesRead] = '\0';

		std::cout << "  Received " << bytesRead << " bytes from fd=" << clientFd << std::endl;
		std::cout << "  Data: " << buffer << std::endl;

		// =========================================
		//  Demo Response (Step 2.2)
		// =========================================
		/*
			Send a simple HTTP response to prove the server works.
			In later steps, Router.cpp will generate proper responses.
		*/
		const char* response =
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html\r\n"
			"Content-Length: 126\r\n"
			"Connection: close\r\n"
			"\r\n"
			"<html><body>"
			"<h1>Welcome to Webserv!</h1>"
			"<p>Step 2.2: Epoll event loop is working!</p>"
			"<p>Connection handled successfully.</p>"
			"</body></html>";

		ssize_t bytesSent = send(clientFd, response, strlen(response), 0);

		if (bytesSent < 0)
		{
			std::cerr << "  send() error: " << strerror(errno) << std::endl;
		}
		else
		{
			std::cout << "  Sent " << bytesSent << " bytes to fd=" << clientFd << std::endl;
		}

		// Close connection after response (HTTP/1.0 style for demo)
		// In full implementation, check Connection: keep-alive header
		return false;
	}

	// =========================================
	//  Handle Writable (EPOLLOUT)
	// =========================================
	/*
		Socket is ready for writing.
		Used when we have buffered data to send.

		For Step 2.2, we don't use this - we send immediately.
		In full implementation, large responses are buffered.
	*/
	if (events & EPOLLOUT)
	{
		// For now, just log it
		std::cout << "  fd=" << clientFd << " is writable" << std::endl;
	}

	return true;  // Keep connection open
}


/*
	closeClientConnection() - Clean up a client connection

	Properly closes a client connection:
	1. Remove from epoll (stop monitoring)
	2. Close the socket (release FD)
	3. Remove from client tracking map
*/
void Server::closeClientConnection(int clientFd)
{
	std::cout << "  Closing client fd=" << clientFd << std::endl;

	// Remove from epoll first (before closing FD)
	removeFromEpoll(clientFd);

	// Close the socket
	close(clientFd);

	// Remove from tracking
	_clients.erase(clientFd);

	std::cout << "  Client closed (remaining: " << _clients.size() << ")" << std::endl;
}


/*
	cleanupTimedOutConnections() - Remove stale connections

	Connections that have been idle for too long are closed.
	This prevents resource exhaustion from abandoned connections.

	Called periodically from the main event loop.
*/
void Server::cleanupTimedOutConnections()
{
	time_t now = time(NULL);
	std::vector<int> toClose;

	// Find timed-out connections
	for (std::map<int, ClientInfo>::iterator it = _clients.begin();
			it != _clients.end(); ++it)
	{
		const ClientInfo& client = it->second;

		if (now - client.lastActivity > CONNECTION_TIMEOUT_SEC)
		{
			std::cout << "  Client fd=" << client.fd << " timed out ("
						<< (now - client.lastActivity) << " seconds idle)" << std::endl;
			toClose.push_back(client.fd);
		}
	}

	// Close timed-out connections
	for (size_t i = 0; i < toClose.size(); ++i)
	{
		closeClientConnection(toClose[i]);
	}

	if (!toClose.empty())
	{
		std::cout << "  Cleaned up " << toClose.size() << " timed-out connection(s)" << std::endl;
	}
}


// =================================================================
//  HELPER FUNCTIONS
// =================================================================

/*
	isListenSocket() - Check if FD is a listening socket

	Quick lookup using the _listenFds set.
*/
bool Server::isListenSocket(int fd) const
{
	return _listenFds.find(fd) != _listenFds.end();
}


/*
	getListenSocketByFd() - Find ListenSocket info by file descriptor
*/
const ListenSocket* Server::getListenSocketByFd(int fd) const
{
	for (size_t i = 0; i < _listenSockets.size(); ++i)
	{
		if (_listenSockets[i].fd == fd)
		{
			return &_listenSockets[i];
		}
	}
	return NULL;
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
int Server::createListenSocket(const std::string &host, int port)
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
	int optval = 1; // Enable the option

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
	serverAddr.sin_port = htons(port); // Convert to network byte order!

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
	if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
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
	const int BACKLOG = 128; // Queue size for pending connections

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

// ====================
//  SERVER LIFECYCLE
// ====================

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

	// Close all client connections first
	std::vector<int> clientFds;
	for (std::map<int, ClientInfo>::iterator it = _clients.begin();
			it != _clients.end(); ++it)
	{
		clientFds.push_back(it->first);
	}

	for (size_t i = 0; i < clientFds.size(); ++i)
	{
		closeClientConnection(clientFds[i]);
	}

	// Close epoll
	closeEpoll();

	// Close listening sockets
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
	_listenFds.clear();
}

// =================================================================
//  GETTERS
// =================================================================

bool Server::isRunning() const
{
	return _running;
}

const std::vector<ListenSocket> &Server::getListenSockets() const
{
	return _listenSockets;
}


int Server::getEpollFd() const
{
	return _epollFd;
}

size_t Server::getClientCount() const
{
	return _clients.size();
}
