/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:54:34 by anemet            #+#    #+#             */
/*   Updated: 2025/12/16 13:58:12 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Config.hpp"

#include <iostream>
#include <csignal>
#include <cstdlib>

/*

# Test basic GET request
curl -v http://localhost:8080/

# Test index page
curl http://localhost:8080/index.html

# Test 404 error
curl http://localhost:8080/nonexistent.html

# Test CGI (Python script)
curl http://localhost:8080/cgi-bin/hello.py

# Test file upload (POST)
curl -X POST -F "file=@testfile.txt" http://localhost:8080/upload

# Test DELETE (if configured)
curl -X DELETE http://localhost:8080/uploads/testfile.txt

*/

/*
    =================================================================
        WEBSERV - A Minimal HTTP/1.1 Web Server
    =================================================================

    Usage: ./webserv [config_file]

    If no config file is specified, uses "config/default.conf"

    The program:
    1. Parses command line arguments
    2. Loads and validates configuration
    3. Initializes the server (creates listening sockets)
    4. Runs the main event loop (epoll-based)
    5. Handles graceful shutdown on SIGINT/SIGTERM
*/


// =================================================================
//  GLOBAL SIGNAL HANDLING
// =================================================================

/*
    Global pointer to server for signal handling.

    Why global? Signal handlers in C/C++ can only be simple functions
    with specific signatures. They can't be member functions or have
    custom parameters. So we need a global way to access our server.

    This is a common pattern in server implementations.
*/
static Server* g_server = NULL;

/*
    signalHandler - Handle SIGINT (Ctrl+C) and SIGTERM

    When user presses Ctrl+C or sends kill signal, we want to:
    1. Stop the server gracefully
    2. Close all connections properly
    3. Free all resources
    4. Exit cleanly

    This prevents resource leaks and ensures clients get proper
    disconnect notifications.
*/
static void signalHandler(int signum)
{
    if (signum == SIGINT)
        std::cout << "\n[INFO] Received SIGINT (Ctrl+C), shutting down..." << std::endl;
    else if (signum == SIGTERM)
        std::cout << "\n[INFO] Received SIGTERM, shutting down..." << std::endl;

    if (g_server != NULL)
    {
        g_server->stop();
    }
}

/*
    setupSignalHandlers - Register signal handlers

    We handle:
    - SIGINT:  Ctrl+C from terminal
    - SIGTERM: Standard termination signal
    - SIGPIPE: Broken pipe (client disconnected while we're writing)

    SIGPIPE is particularly important: by default, writing to a closed
    socket sends SIGPIPE and terminates the program. We ignore it so
    we can handle the error gracefully (send() will return -1 with EPIPE).
*/
static void setupSignalHandlers()
{
    // Use sigaction for more reliable signal handling
    struct sigaction sa;

    // Handler for SIGINT and SIGTERM
    sa.sa_handler = signalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        std::cerr << "[ERROR] Failed to set SIGINT handler" << std::endl;
    }

    if (sigaction(SIGTERM, &sa, NULL) == -1)
    {
        std::cerr << "[ERROR] Failed to set SIGTERM handler" << std::endl;
    }

    // Ignore SIGPIPE - we'll handle broken pipes via send() return value
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) == -1)
    {
        std::cerr << "[ERROR] Failed to ignore SIGPIPE" << std::endl;
    }
}


// =================================================================
//  HELPER FUNCTIONS
// =================================================================

/*
    printUsage - Display usage information

    Called when user provides invalid arguments.
*/
static void printUsage(const char* programName)
{
    std::cout << "Usage: " << programName << " [config_file]" << std::endl;
    std::cout << std::endl;
    std::cout << "Arguments:" << std::endl;
    std::cout << "  config_file  Path to configuration file (optional)" << std::endl;
    std::cout << "               Default: config/default.conf" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << std::endl;
    std::cout << "  " << programName << " config/default.conf" << std::endl;
    std::cout << "  " << programName << " /path/to/custom.conf" << std::endl;
}

/*
    printBanner - Display startup banner

    Shows server info when starting up.
*/
static void printBanner()
{
    std::cout << std::endl;
    std::cout << "╔══════════════════════════════════════════╗" << std::endl;
    std::cout << "║            WEBSERV v1.0                  ║" << std::endl;
    std::cout << "║     A minimal HTTP/1.1 Web Server        ║" << std::endl;
    std::cout << "║            42 School Project             ║" << std::endl;
    std::cout << "╚══════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
}


// =================================================================
//  MAIN FUNCTION
// =================================================================

int main(int argc, char** argv)
{
    // =====================
    //  1. Parse Arguments
    // =====================

    std::string configPath;

    if (argc > 2)
    {
        std::cerr << "[ERROR] Too many arguments" << std::endl;
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }
    else if (argc == 2)
    {
        // Check for help flags
        std::string arg = argv[1];
        if (arg == "-h" || arg == "--help")
        {
            printUsage(argv[0]);
            return EXIT_SUCCESS;
        }
        configPath = argv[1];
    }
    else
    {
        // No arguments: use default config
        configPath = "config/default.conf";
    }

    // Print startup banner
    printBanner();
    std::cout << "[INFO] Starting webserv..." << std::endl;
    std::cout << "[INFO] Config file: " << configPath << std::endl;

    // =====================
    //  2. Setup Signal Handlers
    // =====================

    setupSignalHandlers();

    // =====================
    //  3. Parse Configuration
    // =====================

    Config config;

    try
    {
        std::cout << "[INFO] Parsing configuration..." << std::endl;
        config.parseFile(configPath);

        // Optional: Print parsed config for debugging
        // config.printConfig();

        std::cout << "[INFO] Configuration loaded successfully" << std::endl;
        std::cout << "[INFO] Configured servers: " << config.getServers().size() << std::endl;

        // Print listening ports
        const std::vector<ServerConfig>& servers = config.getServers();
        for (size_t i = 0; i < servers.size(); ++i)
        {
            std::cout << "[INFO]   - " << servers[i].host << ":" << servers[i].port << std::endl;
        }
    }
    catch (const ConfigException& e)
    {
        std::cerr << "[ERROR] Configuration error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[ERROR] Unexpected error loading config: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    // =====================
    //  4. Initialize Server
    // =====================

    Server server(config);

    // Set global pointer for signal handler
    g_server = &server;

    try
    {
        std::cout << "[INFO] Initializing server..." << std::endl;

        if (!server.init())
        {
            std::cerr << "[ERROR] Failed to initialize server" << std::endl;
            return EXIT_FAILURE;
        }

        std::cout << "[INFO] Server initialized successfully" << std::endl;
        std::cout << "[INFO] Listening sockets created: " << server.getListenSockets().size() << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[ERROR] Server initialization failed: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    // =====================
    //  5. Run Server
    // =====================

    std::cout << std::endl;
    std::cout << "═══════════════════════════════════════════" << std::endl;
    std::cout << "  Server is running! Press Ctrl+C to stop" << std::endl;
    std::cout << "═══════════════════════════════════════════" << std::endl;
    std::cout << std::endl;

    try
    {
        // This blocks until server.stop() is called (via signal handler)
        server.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "[ERROR] Server error: " << e.what() << std::endl;
        g_server = NULL;
        return EXIT_FAILURE;
    }

    // =====================
    //  6. Cleanup
    // =====================

    g_server = NULL;
    std::cout << "[INFO] Server stopped gracefully" << std::endl;
    std::cout << "[INFO] Goodbye!" << std::endl;

    return EXIT_SUCCESS;
}
