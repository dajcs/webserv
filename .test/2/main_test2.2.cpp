/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main_test2.2.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:54:34 by anemet            #+#    #+#             */
/*   Updated: 2025/12/15 19:21:50 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Config.hpp"

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>

// Test result tracking
static int g_testsRun = 0;
static int g_testsPassed = 0;
static int g_testsFailed = 0;

// Color codes for output
#define COLOR_GREEN  "\033[32m"
#define COLOR_RED    "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_RESET  "\033[0m"

// Test macros
#define TEST_ASSERT(condition, message) \
	do { \
		g_testsRun++; \
		if (condition) { \
			std::cout << COLOR_GREEN << "[PASS] " << COLOR_RESET << message << std::endl; \
			g_testsPassed++; \
		} else { \
			std::cout << COLOR_RED << "[FAIL] " << COLOR_RESET << message << std::endl; \
			g_testsFailed++; \
		} \
	} while(0)

#define TEST_SECTION(name) \
	std::cout << "\n" << COLOR_YELLOW << "=== " << name << " ===" << COLOR_RESET << std::endl

// Helper: Create a client socket and connect to server
int createClientSocket(const char* host, int port)
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		return -1;

	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);

	if (inet_pton(AF_INET, host, &serverAddr.sin_addr) <= 0)
	{
		close(sockfd);
		return -1;
	}

	// Set non-blocking for connect
	int flags = fcntl(sockfd, F_GETFL, 0);
	fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

	int result = connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
	if (result < 0 && errno != EINPROGRESS)
	{
		close(sockfd);
		return -1;
	}

	return sockfd;
}

// Helper: Wait for socket to be connected
bool waitForConnect(int sockfd, int timeoutMs)
{
	fd_set writefds;
	FD_ZERO(&writefds);
	FD_SET(sockfd, &writefds);

	struct timeval tv;
	tv.tv_sec = timeoutMs / 1000;
	tv.tv_usec = (timeoutMs % 1000) * 1000;

	int result = select(sockfd + 1, NULL, &writefds, NULL, &tv);
	if (result > 0 && FD_ISSET(sockfd, &writefds))
	{
		int error = 0;
		socklen_t len = sizeof(error);
		getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len);
		return error == 0;
	}
	return false;
}

/*
 * Test 1: Epoll Initialization
 * Verifies that epoll is properly created after server init
 */
void testEpollInitialization(const Config& config)
{
	TEST_SECTION("Test 1: Epoll Initialization");

	Server server(config);

	// Before init, epoll should not be valid
	TEST_ASSERT(server.getEpollFd() < 0, "Epoll fd invalid before init()");

	// Initialize server
	bool initResult = server.init();
	TEST_ASSERT(initResult == true, "Server init() succeeds");

	// After init, epoll should be valid
	TEST_ASSERT(server.getEpollFd() >= 0, "Epoll fd valid after init()");

	// Listening sockets should be created
	TEST_ASSERT(server.getListenSockets().size() > 0, "Listening sockets created");

	std::cout << "  Info: Epoll fd = " << server.getEpollFd() << std::endl;
	std::cout << "  Info: Listen sockets = " << server.getListenSockets().size() << std::endl;
}

/*
 * Test 2: Single Connection Acceptance
 * Verifies that a single client can connect
 */
void testSingleConnection(const Config& config)
{
	TEST_SECTION("Test 2: Single Connection Acceptance");

	Server server(config);
	bool initResult = server.init();
	TEST_ASSERT(initResult == true, "Server initialized");

	if (!initResult) return;

	// Get the first listening port
	const std::vector<ListenSocket>& sockets = server.getListenSockets();
	if (sockets.empty())
	{
		TEST_ASSERT(false, "No listening sockets available");
		return;
	}

	int port = sockets[0].port;
	std::cout << "  Info: Connecting to port " << port << std::endl;

	// Create client socket
	int clientFd = createClientSocket("127.0.0.1", port);
	TEST_ASSERT(clientFd >= 0, "Client socket created");

	if (clientFd < 0) return;

	// Wait for connection
	bool connected = waitForConnect(clientFd, 1000);
	TEST_ASSERT(connected, "Client connected to server");

	// Clean up
	close(clientFd);
}

/*
 * Test 3: Multiple Simultaneous Connections
 * Verifies non-blocking acceptance of multiple clients
 */
void testMultipleConnections(const Config& config)
{
	TEST_SECTION("Test 3: Multiple Simultaneous Connections");

	Server server(config);
	bool initResult = server.init();
	TEST_ASSERT(initResult == true, "Server initialized");

	if (!initResult) return;

	const std::vector<ListenSocket>& sockets = server.getListenSockets();
	if (sockets.empty())
	{
		TEST_ASSERT(false, "No listening sockets available");
		return;
	}

	int port = sockets[0].port;
	const int NUM_CLIENTS = 10;
	int clientFds[NUM_CLIENTS];
	int connectedCount = 0;

	std::cout << "  Info: Creating " << NUM_CLIENTS << " client connections..." << std::endl;

	// Create multiple client connections rapidly
	for (int i = 0; i < NUM_CLIENTS; i++)
	{
		clientFds[i] = createClientSocket("127.0.0.1", port);
		if (clientFds[i] >= 0)
		{
			connectedCount++;
		}
	}

	TEST_ASSERT(connectedCount == NUM_CLIENTS, "All client sockets created");

	// Wait for connections to complete
	int successfulConnections = 0;
	for (int i = 0; i < NUM_CLIENTS; i++)
	{
		if (clientFds[i] >= 0 && waitForConnect(clientFds[i], 500))
		{
			successfulConnections++;
		}
	}

	std::cout << "  Info: " << successfulConnections << "/" << NUM_CLIENTS
				<< " connections successful" << std::endl;
	TEST_ASSERT(successfulConnections == NUM_CLIENTS, "All clients connected");

	// Clean up
	for (int i = 0; i < NUM_CLIENTS; i++)
	{
		if (clientFds[i] >= 0)
			close(clientFds[i]);
	}
}

/*
 * Test 4: Multiple Ports
 * Verifies server can listen on multiple ports simultaneously
 */
void testMultiplePorts(const Config& multiPortConfig)
{
	TEST_SECTION("Test 4: Multiple Ports");

	Server server(multiPortConfig);
	bool initResult = server.init();
	TEST_ASSERT(initResult == true, "Multi-port server initialized");

	if (!initResult) return;

	const std::vector<ListenSocket>& sockets = server.getListenSockets();
	size_t numPorts = sockets.size();

	std::cout << "  Info: Server listening on " << numPorts << " port(s)" << std::endl;
	TEST_ASSERT(numPorts >= 2, "Multiple listening ports created");

	// Try to connect to each port
	int successfulPorts = 0;
	for (size_t i = 0; i < sockets.size(); i++)
	{
		int port = sockets[i].port;
		std::string host = sockets[i].host;

		// Use 127.0.0.1 if host is 0.0.0.0
		const char* connectHost = (host == "0.0.0.0") ? "127.0.0.1" : host.c_str();

		std::cout << "  Info: Testing connection to " << connectHost << ":" << port << std::endl;

		int clientFd = createClientSocket(connectHost, port);
		if (clientFd >= 0)
		{
			if (waitForConnect(clientFd, 1000))
			{
				successfulPorts++;
				std::cout << "    -> Connection successful" << std::endl;
			}
			else
			{
				std::cout << "    -> Connection failed (timeout)" << std::endl;
			}
			close(clientFd);
		}
		else
		{
			std::cout << "    -> Socket creation failed" << std::endl;
		}
	}

	std::stringstream ss;
	ss << "Connected to " << successfulPorts << "/" << numPorts << " ports";
	TEST_ASSERT(successfulPorts == (int)numPorts, ss.str());
}

/*
 * Test 5: Listening Socket Identification
 * Verifies isListenSocket() correctly identifies socket types
 */
void testListenSocketIdentification(const Config& config)
{
	TEST_SECTION("Test 5: Listen Socket Identification");

	Server server(config);
	bool initResult = server.init();
	TEST_ASSERT(initResult == true, "Server initialized");

	if (!initResult) return;

	const std::vector<ListenSocket>& sockets = server.getListenSockets();

	// All listen sockets should be identified correctly
	bool allIdentified = true;
	for (size_t i = 0; i < sockets.size(); i++)
	{
		// Note: We can't directly test isListenSocket() since it's private
		// Instead, we verify the sockets are in the list
		if (sockets[i].fd < 0)
		{
			allIdentified = false;
			break;
		}
	}

	TEST_ASSERT(allIdentified, "All listening socket FDs are valid");

	// Verify epoll has been set up
	TEST_ASSERT(server.getEpollFd() >= 0, "Epoll instance is valid");
}

/*
 * Test 6: Connection Data Sending
 * Verifies that connections can send/receive basic data
 */
void testConnectionData(const Config& config)
{
	TEST_SECTION("Test 6: Connection Data Exchange");

	Server server(config);
	bool initResult = server.init();
	TEST_ASSERT(initResult == true, "Server initialized");

	if (!initResult) return;

	const std::vector<ListenSocket>& sockets = server.getListenSockets();
	if (sockets.empty())
	{
		TEST_ASSERT(false, "No listening sockets");
		return;
	}

	int port = sockets[0].port;
	int clientFd = createClientSocket("127.0.0.1", port);
	TEST_ASSERT(clientFd >= 0, "Client socket created");

	if (clientFd < 0) return;

	bool connected = waitForConnect(clientFd, 1000);
	TEST_ASSERT(connected, "Client connected");

	if (connected)
	{
		// Send a simple HTTP request
		const char* request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
		ssize_t bytesSent = send(clientFd, request, strlen(request), 0);
		TEST_ASSERT(bytesSent > 0, "Data sent successfully");

		std::cout << "  Info: Sent " << bytesSent << " bytes" << std::endl;
	}

	close(clientFd);
}

/*
 * Test 7: Server Stop and Cleanup
 * Verifies server can be stopped and resources are cleaned up
 */
void testServerCleanup(const Config& config)
{
	TEST_SECTION("Test 7: Server Stop and Cleanup");

	{
		Server server(config);
		bool initResult = server.init();
		TEST_ASSERT(initResult == true, "Server initialized");

		// Check server is ready
		TEST_ASSERT(server.getEpollFd() >= 0, "Epoll fd valid");
		TEST_ASSERT(server.getListenSockets().size() > 0, "Has listening sockets");

		// Stop the server
		server.stop();
		TEST_ASSERT(server.isRunning() == false, "Server stopped");
	}

	// Server destructor called - if we get here without crash, cleanup worked
	TEST_ASSERT(true, "Server destructor completed without crash");
}

/*
 * Print test summary
 */
void printSummary()
{
	std::cout << "\n" << COLOR_YELLOW << "========================================" << COLOR_RESET << std::endl;
	std::cout << COLOR_YELLOW << "           TEST SUMMARY" << COLOR_RESET << std::endl;
	std::cout << COLOR_YELLOW << "========================================" << COLOR_RESET << std::endl;
	std::cout << "Total tests run:    " << g_testsRun << std::endl;
	std::cout << COLOR_GREEN << "Tests passed:       " << g_testsPassed << COLOR_RESET << std::endl;
	std::cout << COLOR_RED << "Tests failed:       " << g_testsFailed << COLOR_RESET << std::endl;
	std::cout << COLOR_YELLOW << "========================================" << COLOR_RESET << std::endl;

	if (g_testsFailed == 0)
	{
		std::cout << COLOR_GREEN << "\n✓ ALL TESTS PASSED!" << COLOR_RESET << std::endl;
	}
	else
	{
		std::cout << COLOR_RED << "\n✗ SOME TESTS FAILED!" << COLOR_RESET << std::endl;
	}
}

/*
 * Main test runner
 */
int main(int argc, char** argv)
{
	// Ignore SIGPIPE to prevent crashes on broken connections
	signal(SIGPIPE, SIG_IGN);

	std::cout << COLOR_YELLOW << "========================================" << COLOR_RESET << std::endl;
	std::cout << COLOR_YELLOW << "   Step 2.2: Epoll Implementation Tests" << COLOR_RESET << std::endl;
	std::cout << COLOR_YELLOW << "========================================" << COLOR_RESET << std::endl;

	// Load default config
	std::string configPath = "config/default.conf";
	std::string multiPortConfigPath = "config/multi_port.conf";

	if (argc > 1)
	{
		configPath = argv[1];
	}

	std::cout << "\nLoading config: " << configPath << std::endl;

	try
	{
		// Load single-port config
		Config config(configPath);
		config.parseFile(configPath);

		// Load multi-port config
		Config multiPortConfig(multiPortConfigPath);
		multiPortConfig.parseFile(multiPortConfigPath);

		// Run tests
		testEpollInitialization(config);
		testSingleConnection(config);
		testMultipleConnections(config);
		testMultiplePorts(multiPortConfig);
		testListenSocketIdentification(config);
		testConnectionData(config);
		testServerCleanup(config);

		printSummary();

	}
	catch (const std::exception& e)
	{
		std::cerr << COLOR_RED << "\nError: " << e.what() << COLOR_RESET << std::endl;
		return 1;
	}

	return (g_testsFailed > 0) ? 1 : 0;
}
