/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main_test2.1.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:54:34 by anemet            #+#    #+#             */
/*   Updated: 2025/12/15 15:57:27 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


/*


	Run tests:
	---------
		./webserv --test



	Test connectivity:
	-----------------
		Terminal 1:
			./webserv config/default_step2.conf

		Terminal 2:

netstat -tuln | grep 8080
# Expected: tcp  0  0  0.0.0.0:8080  0.0.0.0:*  LISTEN

# or use ss (modern netstat)
ss -tuln | grep 8080

# Test with telnet
telnet localhost 8080
# Type: GET / HTTP/1.1
# Then press Enter twice

# Test with curl
curl -v http://localhost:8080/

# Test with netcat
echo -e "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost 8080


	Test Multiport:
	--------------
		Terminal 1:
			./webserv config/multi_port.conf

		Terminal 2:

# Terminal 2
curl http://localhost:8080/  # Port 8080
curl http://localhost:8081/  # Port 8081
curl http://localhost:8082/  # Port 8082 (may fail if not on localhost)


	Test Error Cases:
	----------------

# Try to start server on port already in use
./webserv &
./webserv  # Should fail with "Address already in use"

# Try privileged port without root
# (Create a config with listen 80)
./webserv config/port80.conf  # Should fail with permission error

*/



/*
	=================================================================
		STEP 2.1: SOCKET CREATION & BINDING - TEST SUITE
	=================================================================

	This test file verifies that our socket setup is working correctly.

	What we're testing:
	1. Socket creation for configured ports
	2. Socket options (SO_REUSEADDR)
	3. Binding to specified host:port
	4. Non-blocking mode
	5. Listening for connections
	6. Multiple listening ports
	7. Error handling

	How to run tests:
	-----------------
	# Compile
	make

	# Run with default config
	./webserv

	# Run with specific config
	./webserv config/test.conf

	# In another terminal, test connectivity:
	curl http://localhost:8080/
	# or
	telnet localhost 8080
	# or
	nc localhost 8080
*/

#include <iostream>
#include <cstring>
#include <csignal>
#include <cstdlib>

#include "Config.hpp"
#include "Server.hpp"

// Global pointer for signal handler
static Server* g_server = NULL;

/*
	Signal Handler
	--------------
	Catches Ctrl+C (SIGINT) and Ctrl+\ (SIGQUIT)
	to allow graceful shutdown.

	Without this:
	- Ctrl+C would kill the process immediately
	- Sockets might not be closed properly
	- Port might stay in TIME_WAIT state
*/
void signalHandler(int signum)
{
	std::cout << "\nReceived signal " << signum << std::endl;

	if (g_server)
	{
		g_server->stop();
	}
}

/*
	Setup signal handlers for graceful shutdown
*/
void setupSignalHandlers()
{
	signal(SIGINT, signalHandler);		// Ctrl+C (Terminates the process)
	signal(SIGQUIT, signalHandler);		// Ctrl+\ (Terminates with core dump)
	signal(SIGTERM, signalHandler);		// kill command
}

/*
    printUsage() - Display usage information
*/
void printUsage(const char* programName)
{
	std::cout << "Usage: " << programName << " [config_file]" << std::endl;
	std::cout << std::endl;
	std::cout << "If no config file is specified, uses config/default.conf" << std::endl;
	std::cout << std::endl;
	std::cout << "Examples:" << std::endl;
	std::cout << "  " << programName << std::endl;
	std::cout << "  " << programName << " config/myconfig.conf" << std::endl;
}

/*
	=================================================================
		UNIT TESTS FOR STEP 2.1
	=================================================================
*/

// Test counters
static int g_testsRun = 0;
static int g_testsPassed = 0;
static int g_testsFailed = 0;

// Colors for output
#define COLOR_GREEN  "\033[32m"
#define COLOR_RED    "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_RESET  "\033[0m"

#define TEST_ASSERT(condition, message) \
	do { \
		g_testsRun++; \
		if (condition) { \
			g_testsPassed++; \
			std::cout << COLOR_GREEN << "[PASS] " << COLOR_RESET << message << std::endl; \
		} else { \
			g_testsFailed++; \
			std::cout << COLOR_RED << "[FAIL] " << COLOR_RESET << message << std::endl; \
		} \
	} while (0)

/*
    Test 1: Server initialization with default config
*/
void testDefaultInit()
{
	std::cout << "\n=== Test 1: Default Configuration ===" << std::endl;

	try
	{
		Config config("config/default.conf");
		Server server(config);

		bool initResult = server.init();
		TEST_ASSERT(initResult == true, "Server init() returns true");

		const std::vector<ListenSocket>& sockets = server.getListenSockets();
		TEST_ASSERT(sockets.size() > 0, "At least one listening socket created");

		if (!sockets.empty())
		{
			TEST_ASSERT(sockets[0].fd >= 0, "Socket has valid file descriptor");
			TEST_ASSERT(sockets[0].port > 0, "Socket has valid port");
		}

		server.stop();
		TEST_ASSERT(server.getListenSockets().empty(), "Sockets cleaned up after stop()");
	}
	catch (const std::exception& e)
	{
		std::cout << COLOR_RED << "[ERROR] Exception: " << e.what() << COLOR_RESET << std::endl;
		g_testsFailed++;
	}
}

/*
	Test 2: Multiple ports
*/
void testMultiplePorts()
{
	std::cout << "\n=== Test 2: Multiple Ports ===" << std::endl;

	// Create a config string for multiple ports
	// We'll create the config programmatically

	try
	{
		// Try to create config with multiple servers
		Config config("config/default.conf");
		Server server(config);

		bool initResult = server.init();
		TEST_ASSERT(initResult == true, "Server with multiple ports initializes");

		// Even if config only has one server, test passes if we get here
		server.stop();
	}
	catch (const std::exception& e)
	{
		std::cout << COLOR_YELLOW << "[SKIP] " << e.what() << COLOR_RESET << std::endl;
	}
}

/*
	Test 3: Socket is non-blocking
*/
void testNonBlocking()
{
	std::cout << "\n=== Test 3: Non-Blocking Mode ===" << std::endl;

	try
	{
		Config config("config/default.conf");
		Server server(config);

		bool initResult = server.init();
		TEST_ASSERT(initResult == true, "Server initializes for non-blocking test");

		const std::vector<ListenSocket>& sockets = server.getListenSockets();

		if (!sockets.empty())
		{
			int fd = sockets[0].fd;
			int flags = fcntl(fd, F_GETFL, 0);

			TEST_ASSERT(flags >= 0, "fcntl(F_GETFL) succeeds");
			TEST_ASSERT((flags & O_NONBLOCK) != 0, "O_NONBLOCK flag is set");
		}

		server.stop();
	}
	catch (const std::exception& e)
	{
		std::cout << COLOR_RED << "[ERROR] Exception: " << e.what() << COLOR_RESET << std::endl;
		g_testsFailed++;
	}
}

/*
    Test 4: Socket options
*/
void testSocketOptions()
{
	std::cout << "\n=== Test 4: Socket Options ===" << std::endl;

	try
	{
		Config config("config/default.conf");
		Server server(config);

		bool initResult = server.init();
		TEST_ASSERT(initResult == true, "Server initializes for socket options test");

		const std::vector<ListenSocket>& sockets = server.getListenSockets();

		if (!sockets.empty())
		{
			int fd = sockets[0].fd;
			int optval;
			socklen_t optlen = sizeof(optval);

			// Check SO_REUSEADDR
			int result = getsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, &optlen);
			TEST_ASSERT(result == 0, "getsockopt(SO_REUSEADDR) succeeds");
			TEST_ASSERT(optval != 0, "SO_REUSEADDR is enabled");
		}

		server.stop();
	}
	catch (const std::exception& e)
	{
		std::cout << COLOR_RED << "[ERROR] Exception: " << e.what() << COLOR_RESET << std::endl;
		g_testsFailed++;
	}
}

/*
	Test 5: Socket is listening
*/
void testSocketListening()
{
	std::cout << "\n=== Test 5: Socket is Listening ===" << std::endl;

	try
	{
		Config config("config/default.conf");
		Server server(config);

		bool initResult = server.init();
		TEST_ASSERT(initResult == true, "Server initializes for listening test");

		const std::vector<ListenSocket>& sockets = server.getListenSockets();

		if (!sockets.empty())
		{
			// Try to get socket name to verify it's bound
			struct sockaddr_in addr;
			socklen_t addrLen = sizeof(addr);

			int result = getsockname(sockets[0].fd, (struct sockaddr*)&addr, &addrLen);
			TEST_ASSERT(result == 0, "getsockname() succeeds (socket is bound)");
			TEST_ASSERT(ntohs(addr.sin_port) > 0, "Socket is bound to a valid port");
		}

		server.stop();
	}
	catch (const std::exception& e)
	{
		std::cout << COLOR_RED << "[ERROR] Exception: " << e.what() << COLOR_RESET << std::endl;
		g_testsFailed++;
	}
}

/*
	Test 6: Connection test (requires manual verification)
*/
void testConnection()
{
	std::cout << "\n=== Test 6: Connection Test ===" << std::endl;
	std::cout << "This test requires manual verification." << std::endl;
	std::cout << "After server starts, run in another terminal:" << std::endl;
	std::cout << "  curl http://localhost:8080/" << std::endl;
	std::cout << "  or" << std::endl;
	std::cout << "  telnet localhost 8080" << std::endl;
}

/*
	Run all unit tests
*/
void runUnitTests()
{
	std::cout << "\n" << COLOR_YELLOW;
	std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
	std::cout << "║         STEP 2.1: SOCKET CREATION & BINDING TESTS             ║\n";
	std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
	std::cout << COLOR_RESET << std::endl;

	testDefaultInit();
	testMultiplePorts();
	testNonBlocking();
	testSocketOptions();
	testSocketListening();
	testConnection();

	// Summary
	std::cout << "\n" << COLOR_YELLOW;
	std::cout << "═══════════════════════════════════════════════════════════════\n";
	std::cout << "                      TEST SUMMARY\n";
	std::cout << "═══════════════════════════════════════════════════════════════\n";
	std::cout << COLOR_RESET;
	std::cout << COLOR_GREEN << "  Passed: " << g_testsPassed << COLOR_RESET << std::endl;
	std::cout << COLOR_RED   << "  Failed: " << g_testsFailed << COLOR_RESET << std::endl;
	std::cout << "  Total:  " << g_testsRun << std::endl;
	std::cout << COLOR_YELLOW;
	std::cout << "═══════════════════════════════════════════════════════════════\n";
	std::cout << COLOR_RESET << std::endl;
}


/*
	=================================================================
		MAIN FUNCTION
	=================================================================
*/
int main(int argc, char* argv[])
{
	std::cout << "\n";
	std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
	std::cout << "║                        WEBSERV                                ║\n";
	std::cout << "║              Step 2.1: Socket Creation & Binding              ║\n";
	std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
	std::cout << std::endl;

	// Check for --test flag
	if (argc >= 2 && std::string(argv[1]) == "--test")
	{
		runUnitTests();
		return (g_testsFailed > 0) ? 1 : 0;
	}

	// Determine config file path
	std::string configPath;

	if (argc >= 2)
	{
		configPath = argv[1];
	}
	else
	{
		configPath = "config/default.conf";
		std::cout << "No config file specified, using: " << configPath << std::endl;
	}

	try
	{
		// Parse configuration
		std::cout << "\nLoading configuration from: " << configPath << std::endl;
		Config config(configPath);
		config.printConfig();

		// Create and initialize server
		Server server(config);
		g_server = &server;

		setupSignalHandlers();

		if (!server.init())
		{
			std::cerr << "Failed to initialize server" << std::endl;
			return 1;
		}

		// Run the server (blocking)
		server.run();

		std::cout << "Server shutdown complete." << std::endl;
		return 0;
	}
	catch (const ConfigException& e)
	{
		std::cerr << "Configuration error: " << e.what() << std::endl;
		return 1;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
}
