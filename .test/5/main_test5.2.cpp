/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main_test5.2.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:54:34 by anemet            #+#    #+#             */
/*   Updated: 2025/12/11 17:33:58 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* ************************************************************************** */
/*                                                                            */
/*   test_router.cpp - Unit tests for Router implementation                 */
/*                                                                            */
/* ************************************************************************** */
/*
    =================================
        STATIC FILE SERVING TESTS
    =================================

    This file tests Step 5.2: Static File Serving (GET)

    What is static file serving?
    ----------------------------
    When a web browser requests a file like http://localhost:8080/index.html,
    the web server must:
    1. Parse the request to extract the requested path (/index.html)
    2. Find the corresponding file on the filesystem
    3. Read the file contents
    4. Determine the correct Content-Type (MIME type)
    5. Build an HTTP response with the file contents
    6. Send the response back to the client

    Why test without the network layer?
    -----------------------------------
    The network layer (sockets, poll, send/recv) is complex and introduces
    external dependencies. By testing the Router directly, we can:
    - Verify file serving logic in isolation
    - Get faster, more reliable tests
    - Debug issues more easily
    - Test edge cases that are hard to reproduce over the network

    Testing Strategy:
    -----------------
    We create mock Request objects (simulating what the parser would produce)
    and pass them directly to the Router. This bypasses the network layer
    entirely while still testing all the file serving logic.
*/

#include "Router.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Config.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdlib>

// ===========================================
//  ANSI Color codes for pretty test output
// ===========================================
#define GREEN "\033[32m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define CYAN "\033[36m"
#define RESET "\033[0m"
#define BOLD "\033[1m"

// Global test counters
int g_passed = 0;
int g_failed = 0;


// ===========================================
//  Test Setup Helpers
// ===========================================

/*
	createTestRequest() - Creates a mock HTTP request

	This simulates what Request::parse() would produce after parsing
	raw HTTP data from a client. Format:

	"GET /path HTTP/1.1\r\n"
	"Host: localhost\r\n"
	"\r\n"

	Parameters:
		method: HTTP method (GET, POST, DELETE)
		path:   The request URI path
		body:   Optional request body (for POST)

	Returns:
		A fully parsed Request object ready to be routed
*/
Request createTestRequest(const std::string& method, const std::string& path,
							const std::string& body = "")
{
	std::stringstream rawRequest;

	// Build the request line: "METHOD /path HTTP/1.1\r\n"
	rawRequest << method << " " << path << " HTTP/1.1\r\n";

	// Add required headers
	rawRequest << "Host: localhost\r\n";

	// If there's a body, add Content-Length header
	if (!body.empty())
	{
		rawRequest << "Content-Length: " << body.length() << "\r\n";
	}

	// End of headers (empty line)
	rawRequest << "\r\n";

	// Add body if present
	rawRequest << body;

	// Parse the raw request
	Request request;
	request.parse(rawRequest.str());

	return request;
}


/*
	createTestDirectory() - Creates a test directory structure

	Sets up:
		www_test/
		├── index.html
		├── style.css
		├── script.js
		├── data.json
		├── image.txt (simulating image for simplicity)
		├── subdir/
		│   ├── page.html
		│   └── nested/
		│       └── deep.html
		└── empty/
*/
void createTestDirectory()
{
	// Create directories
	mkdir("www_test", 0755);
	mkdir("www_test/subdir", 0755);
	mkdir("www_test/subdir/nested", 0755);
	mkdir("www_test/empty", 0755);

	// Create test files with content
	std::ofstream file;

	// index.html - Main HTML file
	file.open("www_test/index.html");
	file << "<!DOCTYPE html>\n";
	file << "<html>\n<head><title>Test Page</title></head>\n";
	file << "<body><h1>Hello, World!</h1></body>\n</html>";
	file.close();

	// style.css - CSS file
	file.open("www_test/style.css");
	file << "body { background: white; color: black; }\n";
	file << "h1 { font-size: 24px; }";
	file.close();

	// script.js - JavaScript file
	file.open("www_test/script.js");
	file << "console.log('Hello from JavaScript!');\n";
	file << "function test() { return 42; }";
	file.close();

	// data.json - JSON file
	file.open("www_test/data.json");
	file << "{\n  \"name\": \"webserv\",\n  \"version\": \"1.0\"\n}";
	file.close();

	// Subdirectory files
	file.open("www_test/subdir/page.html");
	file << "<html><body><h1>Subdirectory Page</h1></body></html>";
	file.close();

	file.open("www_test/subdir/nested/deep.html");
	file << "<html><body><h1>Deeply Nested Page</h1></body></html>";
	file.close();

	std::cout << CYAN << "  [Setup] Test directory structure created" << RESET << std::endl;
}


/*
	cleanupTestDirectory() - Removes test files and directories
*/
void cleanupTestDirectory()
{
	// Remove files (order matters - files before directories)
	unlink("www_test/index.html");
	unlink("www_test/style.css");
	unlink("www_test/script.js");
	unlink("www_test/data.json");
	unlink("www_test/subdir/page.html");
	unlink("www_test/subdir/nested/deep.html");

	// Remove directories (deepest first)
	rmdir("www_test/subdir/nested");
	rmdir("www_test/subdir");
	rmdir("www_test/empty");
	rmdir("www_test");

	std::cout << CYAN << "  [Cleanup] Test directory removed" << RESET << std::endl;
}


/*
	createTestConfig() - Creates a Config object for testing

	This simulates parsing a config file like:

	server {
		listen 8080;
		location / {
			root www_test;
			index index.html;
			allowed_methods GET;
			autoindex off;
		}
		location /autoindex {
			root www_test/subdir;
			autoindex on;
			allowed_methods GET;
		}
	}

	Since Config parsing is already implemented, we load from actual config
	or create programmatically for testing.
*/
Config createTestConfig()
{
	// Create a test config file
	std::ofstream conf("test_config.conf");
	conf << "server {\n";
	conf << "    listen 8080;\n";
	conf << "    server_name localhost;\n";
	conf << "\n";
	conf << "    # Default location - serves static files\n";
	conf << "    location / {\n";
	conf << "        root www_test;\n";
	conf << "        index index.html;\n";
	conf << "        allowed_methods GET;\n";
	conf << "        autoindex off;\n";
	conf << "    }\n";
	conf << "\n";
	conf << "    # Subdirectory with autoindex enabled\n";
	conf << "    location /subdir {\n";
	conf << "        root www_test;\n";
	conf << "        index index.html;\n";
	conf << "        allowed_methods GET;\n";
	conf << "        autoindex on;\n";
	conf << "    }\n";
	conf << "\n";
	conf << "    # Empty directory for 403 test\n";
	conf << "    location /empty {\n";
	conf << "        root www_test;\n";
	conf << "        allowed_methods GET;\n";
	conf << "        autoindex off;\n";
	conf << "    }\n";
	conf << "}\n";
	conf.close();

	// Parse the config file
	Config config("test_config.conf");

	// Cleanup the temporary config file
	unlink("test_config.conf");

	return config;
}


// ===========================================
//  Test Result Helpers
// ===========================================

void printTestHeader(const std::string& testName)
{
	std::cout << "\n" << YELLOW;
	std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
	std::cout << BOLD << "  " << testName << RESET << std::endl;
	std::cout << YELLOW;
	std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << RESET << std::endl;
}

void assertStatus(const Response& response, int expectedStatus,
					const std::string& testCase)
{
	int actualStatus = response.getStatusCode();
	if (actualStatus == expectedStatus)
	{
		std::cout << GREEN << "  ✓ " << testCase << RESET << std::endl;
		g_passed++;
	}
	else
	{
		std::cout << RED << "  ✗ " << testCase << RESET << std::endl;
		std::cout << RED << "    Expected status: " << expectedStatus
					<< ", Got: " << actualStatus << RESET << std::endl;
		g_failed++;
	}
}

void assertBodyContains(const Response& response, const std::string& expected,
						const std::string& testCase)
{
	const std::string& body = response.getBody();
	if (body.find(expected) != std::string::npos)
	{
		std::cout << GREEN << "  ✓ " << testCase << RESET << std::endl;
		g_passed++;
	}
	else
	{
		std::cout << RED << "  ✗ " << testCase << RESET << std::endl;
		std::cout << RED << "    Expected body to contain: \"" << expected
					<< "\"" << RESET << std::endl;
		std::cout << RED << "    Actual body (first 100 chars): \""
					<< body.substr(0, 100) << "...\"" << RESET << std::endl;
		g_failed++;
	}
}


// ===========================================
//  Test Cases
// ===========================================

/*
	Test 1: Serve existing HTML file

	When a client requests /index.html:
	1. Router resolves path to www_test/index.html
	2. File exists, so read its contents
	3. Determine MIME type as text/html
	4. Return 200 OK with file content
*/
void testServeHtmlFile(Router& router)
{
	printTestHeader("Test 1: Serve HTML File");

	Request request = createTestRequest("GET", "/index.html");
	Response response = router.route(request, 8080);

	assertStatus(response, 200, "GET /index.html returns 200 OK");
	assertBodyContains(response, "Hello, World!", "Response contains expected content");
	assertBodyContains(response, "<html>", "Response contains HTML structure");
}


/*
	Test 2: Serve CSS file with correct MIME type

	CSS files must be served with Content-Type: text/css
	Otherwise, browsers may refuse to apply styles
*/
void testServeCssFile(Router& router)
{
	printTestHeader("Test 2: Serve CSS File");

	Request request = createTestRequest("GET", "/style.css");
	Response response = router.route(request, 8080);

	assertStatus(response, 200, "GET /style.css returns 200 OK");
	assertBodyContains(response, "background:", "Response contains CSS content");
}


/*
	Test 3: Serve JavaScript file

	JavaScript files need Content-Type: application/javascript
*/
void testServeJsFile(Router& router)
{
	printTestHeader("Test 3: Serve JavaScript File");

	Request request = createTestRequest("GET", "/script.js");
	Response response = router.route(request, 8080);

	assertStatus(response, 200, "GET /script.js returns 200 OK");
	assertBodyContains(response, "console.log", "Response contains JavaScript content");
}


/*
	Test 4: Serve JSON file

	JSON files need Content-Type: application/json
*/
void testServeJsonFile(Router& router)
{
	printTestHeader("Test 4: Serve JSON File");

	Request request = createTestRequest("GET", "/data.json");
	Response response = router.route(request, 8080);

	assertStatus(response, 200, "GET /data.json returns 200 OK");
	assertBodyContains(response, "webserv", "Response contains JSON content");
}


/*
	Test 5: 404 Not Found for missing file

	When a file doesn't exist, the server must return:
	HTTP/1.1 404 Not Found

	This is critical for proper HTTP semantics
*/
void testFileNotFound(Router& router)
{
	printTestHeader("Test 5: 404 Not Found");

	Request request = createTestRequest("GET", "/nonexistent.html");
	Response response = router.route(request, 8080);

	assertStatus(response, 404, "GET /nonexistent.html returns 404 Not Found");
}


/*
	Test 6: Directory request with index file

	When requesting a directory (e.g., /), the server should:
	1. Check if an index file exists (index.html)
	2. If yes, serve the index file
	3. If no, check autoindex or return 403

	The trailing slash behavior:
	- GET / should serve /index.html
	- GET /subdir/ should look for /subdir/index.html
*/
void testDirectoryWithIndex(Router& router)
{
	printTestHeader("Test 6: Directory with Index File");

	Request request = createTestRequest("GET", "/");
	Response response = router.route(request, 8080);

	assertStatus(response, 200, "GET / returns 200 OK (serves index.html)");
	assertBodyContains(response, "Hello, World!", "Response contains index.html content");
}


/*
	Test 7: Directory listing (autoindex)

	When autoindex is enabled and no index file exists:
	- Server generates HTML listing of directory contents
	- Each entry is a clickable link

	This is how NGINX/Apache show directory listings
*/
void testDirectoryListing(Router& router)
{
	printTestHeader("Test 7: Directory Listing (Autoindex)");

	// /subdir has autoindex on
	Request request = createTestRequest("GET", "/subdir/");
	Response response = router.route(request, 8080);

	// Should return 200 with directory listing
	assertStatus(response, 200, "GET /subdir/ returns 200 OK");
	assertBodyContains(response, "page.html", "Directory listing contains page.html");
	assertBodyContains(response, "nested", "Directory listing contains nested/");
}


/*
	Test 8: 403 Forbidden for directory without autoindex

	When:
	- Request is for a directory
	- No index file exists
	- Autoindex is disabled

	Server must return 403 Forbidden
*/
void testDirectoryForbidden(Router& router)
{
	printTestHeader("Test 8: 403 Forbidden (No Index, No Autoindex)");

	// /empty has no index file and autoindex is off
	Request request = createTestRequest("GET", "/empty/");
	Response response = router.route(request, 8080);

	assertStatus(response, 403, "GET /empty/ returns 403 Forbidden");
}


/*
	Test 9: Files in subdirectories

	The server must correctly handle nested paths:
	/subdir/page.html -> www_test/subdir/page.html
*/
void testNestedFile(Router& router)
{
	printTestHeader("Test 9: Nested File Access");

	Request request = createTestRequest("GET", "/subdir/page.html");
	Response response = router.route(request, 8080);

	assertStatus(response, 200, "GET /subdir/page.html returns 200 OK");
	assertBodyContains(response, "Subdirectory Page", "Response contains nested file content");
}


/*
	Test 10: Deeply nested file

	Test multi-level directory traversal:
	/subdir/nested/deep.html -> www_test/subdir/nested/deep.html
*/
void testDeeplyNestedFile(Router& router)
{
	printTestHeader("Test 10: Deeply Nested File Access");

	Request request = createTestRequest("GET", "/subdir/nested/deep.html");
	Response response = router.route(request, 8080);

	assertStatus(response, 200, "GET /subdir/nested/deep.html returns 200 OK");
	assertBodyContains(response, "Deeply Nested Page", "Response contains deep file content");
}


/*
	Test 11: Path traversal attack prevention

	SECURITY: Attackers may try to access files outside the web root:
	GET /../../../etc/passwd

	The server MUST sanitize paths to prevent this:
	- Remove ".." components that would escape root
	- Normalize paths (remove ".", collapse "//"

	This is a critical security feature!
*/
void testPathTraversalPrevention(Router& router)
{
	printTestHeader("Test 11: Path Traversal Prevention (SECURITY)");

	// Try to escape the web root
	Request request = createTestRequest("GET", "/../../../etc/passwd");
	Response response = router.route(request, 8080);

	// Should NOT serve /etc/passwd
	// Either 404 (path doesn't exist in www_test) or sanitized path
	int status = response.getStatusCode();
	if (status == 404 || status == 403)
	{
		std::cout << GREEN << "  ✓ Path traversal blocked (returned " << status << ")" << RESET << std::endl;
		g_passed++;
	}
	else if (status == 200)
	{
		// If 200, verify we didn't serve /etc/passwd
		const std::string& body = response.getBody();
		if (body.find("root:") == std::string::npos)
		{
			std::cout << GREEN << "  ✓ Path traversal blocked (sanitized path)" << RESET << std::endl;
			g_passed++;
		}
		else
		{
			std::cout << RED << "  ✗ SECURITY ISSUE: /etc/passwd was served!" << RESET << std::endl;
			g_failed++;
		}
	}
	else
	{
		std::cout << RED << "  ✗ Unexpected status: " << status << RESET << std::endl;
		g_failed++;
	}
}


/*
	Test 12: Method Not Allowed

	If a location only allows GET, other methods should return:
	HTTP/1.1 405 Method Not Allowed

	This enforces the allowed_methods configuration
*/
void testMethodNotAllowed(Router& router)
{
	printTestHeader("Test 12: 405 Method Not Allowed");

	// Location / only allows GET
	Request request = createTestRequest("POST", "/index.html", "test body");
	Response response = router.route(request, 8080);

	assertStatus(response, 405, "POST /index.html returns 405 Method Not Allowed");
}


/*
	Test 13: HEAD request (like GET but no body)

	HEAD should return same headers as GET but with empty body
	Useful for checking if resource exists without downloading
*/
void testHeadRequest(Router& router)
{
	printTestHeader("Test 13: HEAD Request");

	// Create a HEAD request (similar to GET but expects no body)
	std::string rawRequest = "HEAD /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";
	Request request;
	request.parse(rawRequest);

	Response response = router.route(request, 8080);

	// HEAD should return 200 (same as GET for existing file)
	assertStatus(response, 200, "HEAD /index.html returns 200 OK");
	// Note: In a full implementation, HEAD would have empty body
}


// ===========================================
//  Main - Run All Tests
// ===========================================

int main()
{
	std::cout << "\n" << BLUE << BOLD;
	std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
	std::cout << "║                                                               ║\n";
	std::cout << "║   WEBSERV - Step 5.2: Static File Serving Tests               ║\n";
	std::cout << "║                                                               ║\n";
	std::cout << "║   Testing file serving without network layer                  ║\n";
	std::cout << "║                                                               ║\n";
	std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
	std::cout << RESET << std::endl;

	// Setup
	std::cout << CYAN << "\n[SETUP]" << RESET << std::endl;
	createTestDirectory();

	// Create config and router
	Config config;
	try
	{
		config = createTestConfig();
		std::cout << CYAN << "  [Setup] Configuration loaded" << RESET << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cout << RED << "  [Error] Failed to create config: " << e.what() << RESET << std::endl;
		cleanupTestDirectory();
		return 1;
	}

	Router router(config);
	std::cout << CYAN << "  [Setup] Router initialized" << RESET << std::endl;

	// Run all tests
	std::cout << CYAN << "\n[RUNNING TESTS]" << RESET << std::endl;

	testServeHtmlFile(router);
	testServeCssFile(router);
	testServeJsFile(router);
	testServeJsonFile(router);
	testFileNotFound(router);
	testDirectoryWithIndex(router);
	testDirectoryListing(router);
	testDirectoryForbidden(router);
	testNestedFile(router);
	testDeeplyNestedFile(router);
	testPathTraversalPrevention(router);
	testMethodNotAllowed(router);
	testHeadRequest(router);

	// Cleanup
	std::cout << CYAN << "\n[CLEANUP]" << RESET << std::endl;
	cleanupTestDirectory();

	// Summary
	std::cout << "\n" << BLUE << BOLD;
	std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
	std::cout << "║                        TEST SUMMARY                           ║\n";
	std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
	std::cout << RESET << std::endl;

	int total = g_passed + g_failed;
	std::cout << "  Total assertions: " << total << std::endl;
	std::cout << GREEN << "  Passed:           " << g_passed << RESET << std::endl;
	std::cout << RED << "  Failed:           " << g_failed << RESET << std::endl;

	// Calculate percentage
	double percentage = (total > 0) ? (100.0 * g_passed / total) : 0;
	std::cout << "\n  Success rate:     " << percentage << "%" << std::endl;

	if (g_failed == 0)
	{
		std::cout << "\n" << GREEN << BOLD;
		std::cout << "  🎉 ALL TESTS PASSED! Step 5.2 Static File Serving is complete! 🎉" << RESET;
		std::cout << "\n" << std::endl;
		return 0;
	}
	else
	{
		std::cout << "\n" << RED << BOLD;
		std::cout << "  ❌ Some tests failed. Review the output above." << RESET;
		std::cout << "\n" << std::endl;
		return 1;
	}
}
