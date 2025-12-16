/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:57:03 by anemet            #+#    #+#             */
/*   Updated: 2025/12/16 10:19:17 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef CONNECTION_HPP
# define CONNECTION_HPP

/*
	=================================================================
		CONNECTION CLASS - Client Connection Management
	=================================================================

	┌─────────────────────────────────────────────────────────────────┐
	│                    CONNECTION LIFECYCLE                         │
	├─────────────────────────────────────────────────────────────────┤
	│                                                                 │
	│   1. ACCEPT                                                     │
	│      ↓                                                          │
	│      Client connects, we accept() and create Connection         │
	│      State: CONN_READING                                        │
	│                                                                 │
	│   2. READ REQUEST                                               │
	│      ↓                                                          │
	│      Read data from socket, buffer it, parse HTTP request       │
	│      May take multiple recv() calls (data arrives in chunks)    │
	│      State: CONN_READING                                        │
	│                                                                 │
	│   3. PROCESS REQUEST                                            │
	│      ↓                                                          │
	│      Route request, generate response                           │
	│      State: CONN_WRITING                                        │
	│                                                                 │
	│   4. SEND RESPONSE                                              │
	│      ↓                                                          │
	│      Write response to socket (may take multiple send() calls)  │
	│      State: CONN_WRITING                                        │
	│                                                                 │
	│   5. COMPLETE                                                   │
	│      ↓                                                          │
	│      Either:                                                    │
	│      - Close connection (HTTP/1.0 or Connection: close)         │
	│      - Reset for next request (keep-alive)                      │
	│                                                                 │
	│   TIMEOUT                                                       │
	│      ↓                                                          │
	│      If client is idle too long, close connection               │
	│      Prevents resource exhaustion from abandoned connections    │
	│                                                                 │
	└─────────────────────────────────────────────────────────────────┘

	Why Connection State Matters:
	-----------------------------
	The server uses epoll to know when sockets are ready for I/O.
	But epoll doesn't know what PHASE we're in:

	- CONN_READING: Monitor for EPOLLIN (data to read)
	- CONN_WRITING: Monitor for EPOLLOUT (ready to write)

	We track state to:
	1. Know which epoll events to watch for
	2. Know what action to take when an event occurs
	3. Handle partial reads/writes correctly

	Buffer Management:
	------------------
	HTTP data arrives/sends in unpredictable chunks:

	┌─────────────────────────────────────────────────────────────────┐
	│  recv() call 1:  "GET / HTTP"                                   │
	│  recv() call 2:  "/1.1\r\nHost: local"                          │
	│  recv() call 3:  "host\r\n\r\n"                                 │
	│                                                                 │
	│  Complete request: "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"  │
	└─────────────────────────────────────────────────────────────────┘

	We buffer data until we have a complete request.
	Similarly for responses - send() might not send everything at once.

	Keep-Alive Connections:
	-----------------------
	HTTP/1.1 defaults to keep-alive: after one request/response,
	the connection stays open for more requests.

	Request 1: GET /index.html → Response 1
	Request 2: GET /style.css  → Response 2  (same TCP connection!)
	Request 3: GET /script.js  → Response 3

	This is more efficient than opening a new TCP connection each time.
	We reset the connection state after each complete response.
*/


#include <string>
#include <ctime>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>	// epoll_create1(), epoll_ctl(), epoll_wait()

// Forward declarations
class Request;
class Response;
class Config;
struct ServerConfig;

// Connection State:
//		the current phase of the connection lifecycle
enum ConnectionState
{
	CONN_READING,	// Reading request from client
	CONN_WRITING,	// Writing response to client
	CONN_CLOSED,	// Connection closed normally
	CONN_ERROR		// Error occurred
};


/*
	=================================================================
		CONNECTION CLASS
	=================================================================

	Manages a single client connection from accept() to close().

	Responsibilities:
	1. Store socket file descriptor and client info
	2. Buffer incoming request data
	3. Buffer outgoing response data
	4. Track connection state and timestamps
	5. Provide interface for reading and writing
	6. Handle keep-alive connection reuse

	Thread Safety:
	--------------
	This class is NOT thread-safe. In webserv, we use a single-threaded
	event loop, so this is fine. Each connection is handled by one thread.
*/
class Connection
{
public:
	// ===========================
	//  Constructors & Destructor
	// ===========================

	/*
		Default Constructor
		Creates an invalid connection (fd = -1).
		Used for container initialization.
	*/
	Connection();

	/*
		Parameterized Constructor
		Creates a connection for an accepted client socket.

		Parameters:
			fd:         Client socket file descriptor (from accept())
			clientAddr: Client's IP address structure
			serverPort: Which port the client connected to
	*/
	Connection(int fd, const struct sockaddr_in& clientAddr, int serverPort);

	/*
		Destructor
		Does NOT close the socket - the Server class manages socket lifecycle.
		This allows connections to be copied/moved without accidentally
		closing sockets.
	*/
	~Connection();

	/*
		Copy Constructor & Assignment
		Connections can be copied for storage in containers.
		The socket is NOT duplicated - both copies reference the same fd.
	*/
	Connection(const Connection& other);
	Connection& operator=(const Connection& other);


	// ===========================
	//  Core I/O Operations
	// ===========================

	/*
		readData() - Read available data from client socket

		Called when epoll indicates EPOLLIN (data available).

		Non-blocking behavior:
		- Reads whatever data is currently available
		- Returns immediately, never blocks
		- May need multiple calls to get complete request

		Algorithm:
		1. Call recv() to get available data
		2. Append to read buffer
		3. Try to parse as HTTP request
		4. If request complete, change state to CONN_WRITING

		Returns:
			true:  Read successful (may need more data)
			false: Error or client disconnected (close connection)
	*/
	bool readData();

	/*
		writeData() - Write buffered response data to client

		Called when epoll indicates EPOLLOUT (ready to write).

		Non-blocking behavior:
		- Writes as much as the socket will accept
		- Returns immediately, never blocks
		- May need multiple calls to send complete response

		Algorithm:
		1. Call send() with remaining response data
		2. Update write position
		3. If all data sent:
		   - If keep-alive: reset for next request
		   - If not: mark as CONN_CLOSED

		Returns:
			true:  Write successful (may have more to write)
			false: Error (close connection)
	*/
	bool writeData();

/*
	handleWriteComplete() - Handle successful completion of response

	Called when all response data has been sent.

	Decision logic:
	- If keep-alive: Reset connection for next request
	- If not keep-alive: Close the connection

	Returns:
		true if connection should stay open (keep-alive)
		false if connection should close
*/
bool handleWriteComplete();


	// ===========================
	//  Response Management
	// ===========================

	/*
		setResponse() - Queue a response for sending

		Called after the Router generates a response for the request.

		Parameters:
			response: The HTTP response to send

		Side effects:
		- Stores the response data in write buffer
		- Changes state to CONN_WRITING
		- Updates keep-alive flag from response
	*/
	void setResponse(const Response& response);

	/*
		hasCompleteRequest() - Check if we've received a full HTTP request

		HTTP requests end with "\r\n\r\n" (blank line after headers).
		For POST with body, we also need the complete body.

		Returns:
			true if request is complete and ready for processing
	*/
	bool hasCompleteRequest() const;


	// ===========================
	//  State Management
	// ===========================

	/*
		getState() - Get current connection state

		Used by Server to determine:
		- Which epoll events to monitor (EPOLLIN vs EPOLLOUT)
		- Whether to close the connection
	*/
	ConnectionState getState() const;

	/*
		setState() - Manually set connection state

		Usually state changes happen automatically, but sometimes
		we need to force a state (e.g., on error).
	*/
	void setState(ConnectionState state);

	/*
		reset() - Reset connection for keep-alive reuse

		After a complete request/response cycle on a keep-alive connection:
		1. Clear request/response buffers
		2. Reset state to CONN_READING
		3. Keep socket open for next request
		4. Update activity timestamp
	*/
	void reset();


	// ===========================
	//  Timeout Management
	// ===========================

	/*
		updateActivity() - Update last activity timestamp

		Called whenever we successfully read or write data.
		Used for timeout detection.
	*/
	void updateActivity();

	/*
		isTimedOut() - Check if connection has been idle too long

		Parameters:
			timeoutSeconds: Maximum idle time allowed

		Returns:
			true if connection should be closed due to inactivity
	*/
	bool isTimedOut(int timeoutSeconds) const;

	/*
		getLastActivity() - Get timestamp of last activity

		Returns:
			Unix timestamp of last read/write operation
	*/
	time_t getLastActivity() const;

	/*
		getConnectTime() - Get timestamp when connection was established

		Returns:
			Unix timestamp of when accept() was called
	*/
	time_t getConnectTime() const;


	// ===========================
	//  Getters
	// ===========================

	/*
		getFd() - Get the socket file descriptor

		Used by Server for epoll operations and socket I/O.
	*/
	int getFd() const;

	/*
		getServerPort() - Get the port client connected to

		Used for routing to the correct server configuration.
		If we listen on multiple ports (8080, 8081), we need to
		know which port this client connected to.
	*/
	int getServerPort() const;

	/*
		getClientIP() - Get client's IP address as string

		Used for logging and access control.
		Example: "192.168.1.100"
	*/
	const std::string& getClientIP() const;

	/*
		getClientPort() - Get client's port number

		The client's ephemeral port (not our server port).
		Used for logging.
	*/
	int getClientPort() const;

	/*
		getRequest() - Get the parsed HTTP request

		Returns pointer to Request object after successful parsing.
		Returns NULL if request not yet complete or parsing failed.
	*/
	Request* getRequest();
	const Request* getRequest() const;

	/*
		shouldKeepAlive() - Check if connection should stay open

		Returns:
			true if HTTP/1.1 with keep-alive (or HTTP/1.0 with explicit keep-alive)
			false if connection should close after response
	*/
	bool shouldKeepAlive() const;


	// ===========================
	//  Epoll Event Helpers
	// ===========================

	/*
		getNeededEvents() - Get epoll events needed for current state

		Returns:
			EPOLLIN  if state == CONN_READING
			EPOLLOUT if state == CONN_WRITING
			0        if connection should be closed

		Used by Server to update epoll monitoring.
	*/
	uint32_t getNeededEvents() const;

	/*
		hasDataToWrite() - Check if there's pending response data

		Returns:
			true if write buffer has unsent data
	*/
	bool hasDataToWrite() const;



private:
	// ===========================
	//  Private Members
	// ===========================

	// Socket and client information
	int					_fd;            // Client socket file descriptor
	std::string			_clientIP;      // Client IP address (for logging)
	int					_clientPort;    // Client source port
	int					_serverPort;    // Server port client connected to

	// Timestamps for timeout management
	time_t				_connectTime;   // When connection was established
	time_t				_lastActivity;  // Last successful I/O operation

	// Connection state
	ConnectionState		_state;         // Current state in lifecycle
	bool				_keepAlive;     // Keep connection open after response?

	// Buffers for HTTP data
	std::string			_readBuffer;    // Incoming data from client
	std::string			_writeBuffer;   // Outgoing data to client
	size_t				_writeOffset;   // How much of writeBuffer has been sent

	// HTTP Request object
	Request*			_request;       // Parsed HTTP request (owned by Connection)


	// ===========================
	//  Private Methods
	// ===========================

	/*
		parseRequest() - Attempt to parse buffered data as HTTP request

		Called after each read to check if we have a complete request.

		Returns:
			true if request successfully parsed
			false if need more data or parse error
	*/
	bool parseRequest();

	/*
		determineKeepAlive() - Check if connection should persist

		Examines HTTP version and Connection header to determine
		if we should keep the connection open after the response.

		HTTP/1.1: Keep-alive by default (unless Connection: close)
		HTTP/1.0: Close by default (unless Connection: keep-alive)
	*/
	void determineKeepAlive();

};




























#endif
