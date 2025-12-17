/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:57:30 by anemet            #+#    #+#             */
/*   Updated: 2025/12/17 21:50:06 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

/*
	=================================================================
		SERVER CLASS - NETWORK INFRASTRUCTURE FOR WEBSERV
	=================================================================

	The Server class is responsible for:
	1. Creating listening sockets for each configured port
	2. Managing the main event loop (poll/epoll)
	3. Accepting new client connections
	4. Coordinating request/response handling

	The key system calls we use:
	- socket():    Create a new socket
	- setsockopt(): Configure socket options
	- bind():      Assign address to socket
	- listen():    Mark socket as accepting connections
	- accept():    Accept incoming connection

	STEP 2.2: EPOLL-BASED EVENT LOOP
	================================
	1. select():
		- Limited to 1024 file descriptors (FD_SETSIZE)
		- O(n) complexity - scans all FDs each time
		- Must rebuild FD sets after each call

	2. poll():
		- No FD limit
		- Still O(n) - kernel scans all FDs
		- Better than select, but not scalable

	3. epoll():
		- No FD limit
		- O(1) for events - only returns ready FDs
		- Maintains state in kernel - no rebuilding
		- Linux-specific (kqueue on BSD/macOS)

	For a high-performance web server, epoll is the clear choice!

	1. epoll_create1(): Create an epoll instance (returns fd)
	2. epoll_ctl():     Add/modify/remove FDs to monitor
	3. epoll_wait():    Wait for events (blocking or timeout)

	The epoll instance maintains an "interest list" of FDs.
	When events occur, they're added to a "ready list".
	epoll_wait() returns only the FDs that have events.

	This is much more efficient than poll/select which must
	check ALL file descriptors every time!
*/

#include <string>
#include <vector>
#include <map>
#include <set>

#include "Config.hpp"
#include "Connection.hpp"

// Standard C/C++ headers
#include <iostream>		// std::cout, std::cerr for logging
#include <cstring>		// memset(), strerror()
#include <cerrno>		// errno for error codes
#include <cstdlib>		// exit()
#include <ctime>		// time() for connection timestamps

// POSIX/System headers for networking
#include <sys/socket.h>	// socket(), bind(), listen(), accept(), setsockopt(), SO_REUSEPORT
#include <sys/types.h>	// Various type definitions
#include <netinet/in.h>	// struct sockaddr_in, INADDR_ANY, htons()
#include <arpa/inet.h>	// inet_pton(), inet_ntoa() for IP address conversion
#include <unistd.h>		// close(), fcntl()
#include <fcntl.h>		// fcntl(), O_NONBLOCK
#include <sys/epoll.h>	// epoll_create1(), epoll_ctl(), epoll_wait()


/*
	=================================================================
		EPOLL EVENT FLAGS REFERENCE
	=================================================================

	EPOLLIN:	Data available for reading (or connection waiting)
	EPOLLOUT:	Ready for writing (send buffer has space)
	EPOLLERR:	Error condition on the FD
	EPOLLHUP:	Hang up (peer closed connection)
	EPOLLRDHUP:	Peer closed writing half of connection (Linux 2.6.17+)
	EPOLLET:	Edge-triggered mode (vs default level-triggered)
	EPOLLONESHOT:	Disable FD after one event (must re-arm)

	Level-triggered vs Edge-triggered:
	----------------------------------
	Level-triggered (default):
		- epoll_wait() returns if FD IS ready
		- Will keep returning until condition clears
		- Safer, simpler to use

	Edge-triggered (EPOLLET):
		- epoll_wait() returns when FD BECOMES ready
		- Only one notification per state change
		- More efficient but tricky to use correctly
		- Must drain all data or you'll miss events!

	We'll use level-triggered for simplicity and safety.
*/


/*
	=================================================================
		CONSTANTS
	=================================================================
*/

// Maximum events to process in one epoll_wait() call
// This is just a buffer size, not a limit on connections
static const int MAX_EPOLL_EVENTS = 64;

// Timeout for epoll_wait() in milliseconds
// -1 = block forever (not recommended)
//  0 = return immediately (polling)
// >0 = wait up to N milliseconds
static const int EPOLL_TIMEOUT_MS = 1000;  // 1 second

// Connection timeout in seconds (for cleanup)
#ifdef DEBUG
	static const int CONNECTION_TIMEOUT_SEC = 3600; // 1 hour
#else
	static const int CONNECTION_TIMEOUT_SEC = 60;   // 1 minute
#endif

/*
	=================================================================
		SOCKET INFORMATION STRUCTURE
	=================================================================

	Stores metadata about each listening socket.
	We need this to:
	1. Track which socket belongs to which server config
	2. Know the address/port for logging
	3. Manage socket lifecycle (creation, cleanup)
*/
struct ListenSocket
{
	int					fd;				// Socket file descriptor
	std::string			host;			// IP address (e.g., "0.0.0.0", "127.0.0.1")
	int					port;			// Port number (e.g., 8080)
	const ServerConfig*	serverConfig;	// Pointer to associated config

	ListenSocket() : fd(-1), host(""), port(0), serverConfig(NULL) {}
};



/*
	=================================================================
		CLIENT CONNECTION INFO (for tracking connected clients)
	=================================================================

	We need to track information about each connected client:
	- File descriptor for communication
	- Connection timestamp for timeout handling
	- Server port they connected to (for routing)
	- Client address for logging
*/
struct ClientInfo
{
	int				fd;				// Client socket file descriptor
	time_t			connectTime;	// When connection was established
	time_t			lastActivity;	// Last read/write activity
	int				serverPort;		// Which server port they connected to
	std::string		clientIP;		// Client's IP address (for logging)
	int				clientPort;		// Client's port (for logging)

	ClientInfo() :
		fd(-1),
		connectTime(0),
		lastActivity(0),
		serverPort(0),
		clientIP(""),
		clientPort(0)
	{}
};


/*
	=================================================================
		SERVER CLASS
	=================================================================
*/
class Server
{
public:
	// =====================
	//  Constructors
	// =====================
	Server();
	explicit Server(const Config& config);
	~Server();

	// =====================
	//  Core Operations
	// =====================

	// init() - Initialize all listening sockets
	bool init();

	// run() - Start the main event loop
	void run();

	// stop() - Gracefully shut down the server
	void stop();

	// =====================
	//  Getters
	// =====================
	bool isRunning() const;
	const std::vector<ListenSocket>& getListenSockets() const;

	// =====================
	//  Configuration
	// =====================
	void setConfig(const Config& config);

	// =====================
	//  Epoll Access (for testing)
	// =====================
	int getEpollFd() const;
	size_t getClientCount() const;


private:
	// =====================
	//  Socket Creation
	// =====================

	// createListenSocket() - Create and configure a single listening socket
	int createListenSocket(const std::string& host, int port);

	// setNonBlocking() - Set a file descriptor to non-blocking mode
	bool setNonBlocking(int fd);

	// closeAllSockets() - Close all listening sockets
	void closeAllSockets();


	// =====================
	//  Epoll Management
	// =====================

	/*
		initEpoll() - Create and initialize the epoll instance

		Creates the epoll file descriptor and adds all listening
		sockets to the interest list.

		Returns: true on success, false on failure
	*/
	bool initEpoll();

	/*
		addToEpoll() - Add a file descriptor to epoll monitoring

		Parameters:
			fd:     File descriptor to monitor
			events: Event flags (EPOLLIN, EPOLLOUT, etc.)

		Returns: true on success, false on failure
	*/
	bool addToEpoll(int fd, uint32_t events);

	/*
		modifyEpoll() - Modify events for an existing FD

		Parameters:
			fd:     File descriptor already in epoll
			events: New event flags

		Returns: true on success, false on failure
	*/
	bool modifyEpoll(int fd, uint32_t events);

	/*
		removeFromEpoll() - Remove a file descriptor from epoll

		Parameters:
			fd: File descriptor to remove

		Returns: true on success, false on failure
	*/
	bool removeFromEpoll(int fd);

	/*
		closeEpoll() - Close the epoll instance
	*/
	void closeEpoll();

	// =====================
	//  Connection Handling
	// =====================

	/*
		acceptNewConnection() - Accept a new client connection

		Called when a listening socket has EPOLLIN event.

		Parameters:
			listenFd: The listening socket with pending connection

		Returns: Client FD on success, -1 on failure
	*/
	int acceptNewConnection(int listenFd);

	/*
		handleClientEvent() - Process events on a client socket

		Parameters:
			clientFd: The client socket with events
			events:   The epoll events that occurred

		Returns: true to keep connection, false to close it
	*/
	bool handleClientEvent(int clientFd, uint32_t events);

	/*
		closeClientConnection() - Clean up a client connection

		Removes from epoll, closes socket, removes from tracking.

		Parameters:
			clientFd: The client socket to close
	*/
	void closeClientConnection(int clientFd);

	/*
		cleanupTimedOutConnections() - Remove stale connections

		Called periodically to close connections that have been
		idle for too long.
	*/
	void cleanupTimedOutConnections();

	// =====================
	//  Helper Functions
	// =====================

	/*
		isListenSocket() - Check if FD is a listening socket

		Returns: true if fd is a listening socket, false otherwise
	*/
	bool isListenSocket(int fd) const;

	/*
		getListenSocketByFd() - Get ListenSocket info by FD

		Returns: Pointer to ListenSocket, or NULL if not found
	*/
	const ListenSocket* getListenSocketByFd(int fd) const;

	// processRequest() - Route request and generate response
	void processRequest(Connection& conn);




	// =====================
	//  Member Variables
	// =====================
	const Config*				_config;		// Server configuration
	std::vector<ListenSocket>	_listenSockets;	// All listening sockets
	bool						_running;		// Server state flag

	// Epoll-specific members
	int							_epollFd;		// Epoll instance FD
	std::map<int, ClientInfo>	_clients;		// Active client connections
	std::map<int, Connection>	_connections; 	// Client connections
	std::set<int>				_listenFds;		// Set of listening FDs (for quick lookup)


	// =====================
	//  Disabled Operations
	// =====================
	// Copy operations disabled - Server manages system resources
	Server(const Server& other);
	Server& operator=(const Server& other);
};

#endif // SERVER_HPP
