/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:57:30 by anemet            #+#    #+#             */
/*   Updated: 2025/12/14 22:50:42 by anemet           ###   ########.fr       */
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

	Non-blocking I/O:
	-----------------
	In webserv, we MUST use non-blocking I/O. This means:
	- Operations return immediately (don't wait for data)
	- We use poll/epoll to know when data is ready
	- One thread can handle thousands of connections

	Why non-blocking? With blocking I/O:
		Client 1 connects → Server reads (BLOCKS until data arrives)
		While blocked, Client 2 can't connect!

	With non-blocking + poll:
		poll() monitors ALL clients simultaneously
		Only read/write when data is ready
		Server never blocks, stays responsive
*/

#include <string>
#include <vector>
#include <map>

#include "Config.hpp"

// Standard C/C++ headers
#include <iostream>		// std::cout, std::cerr for logging
#include <cstring>		// memset(), strerror()
#include <cerrno>		// errno for error codes
#include <cstdlib>		// exit()

// POSIX/System headers for networking
#include <sys/socket.h>	// socket(), bind(), listen(), accept(), setsockopt(), SO_REUSEPORT
#include <sys/types.h>	// Various type definitions
#include <netinet/in.h>	// struct sockaddr_in, INADDR_ANY, htons()
#include <arpa/inet.h>	// inet_pton(), inet_ntoa() for IP address conversion
#include <unistd.h>		// close(), fcntl()
#include <fcntl.h>		// fcntl(), O_NONBLOCK


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

	/*
		init() - Initialize all listening sockets

		This is the main setup function that:
		1. Reads server configurations
		2. Creates a socket for each unique host:port
		3. Binds, sets options, and starts listening

		Returns: true on success, false on failure
	*/
	bool init();

	/*
		run() - Start the main event loop

		This will be implemented in Step 2.2 (Poll/Epoll).
		For now, it's a placeholder that demonstrates sockets work.
	*/
	void run();

	/*
		stop() - Gracefully shut down the server

		Closes all sockets and cleans up resources.
	*/
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

private:
	// =====================
	//  Socket Creation
	// =====================

	/*
		createListenSocket() - Create and configure a single listening socket

		This is where the magic happens! It performs:
		1. socket()     - Create the socket
		2. setsockopt() - Set SO_REUSEADDR, SO_REUSEPORT
		3. bind()       - Bind to host:port
		4. fcntl()      - Set non-blocking mode
		5. listen()     - Start accepting connections

		Parameters:
			host: IP address to bind to
			port: Port number to bind to

		Returns: Socket file descriptor, or -1 on error
	*/
	int createListenSocket(const std::string& host, int port);

	/*
		setNonBlocking() - Set a file descriptor to non-blocking mode

		Uses fcntl() to add O_NONBLOCK flag.
		Essential for our event-driven architecture.

		Returns: true on success, false on failure
	*/
	bool setNonBlocking(int fd);

	/*
		closeAllSockets() - Close all listening sockets

		Called during cleanup to release system resources.
	*/
	void closeAllSockets();

	// =====================
	//  Member Variables
	// =====================
	const Config*				_config;		// Server configuration
	std::vector<ListenSocket>	_listenSockets;	// All listening sockets
	bool						_running;		// Server state flag

	// =====================
	//  Disabled Operations
	// =====================
	// Copy operations disabled - Server manages system resources
	Server(const Server& other);
	Server& operator=(const Server& other);
};

#endif // SERVER_HPP
