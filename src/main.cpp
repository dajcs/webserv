/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:54:34 by anemet            #+#    #+#             */
/*   Updated: 2025/12/09 13:59:07 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* ************************************************************************** */
/*   test_request_step4_1.cpp - Standalone test for Request Line Parser     */
/*   No network code required - tests parsing logic directly                */
/* ************************************************************************** */

#include "Request.hpp"
#include <iostream>
#include <string>
#include <vector>

// Color codes for terminal output
#define GREEN "\033[32m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define RESET "\033[0m"

// Test result tracking
int g_passed = 0;
int g_failed = 0;



/**
 * Test helper function
 * Simulates what the network layer will do: feed raw HTTP data to parser
 */
void testRequest(const std::string& testName,
				 const std::string& requestData,
				 bool shouldSucceed,
				 const std::string& expectedMethod = "",
				 const std::string& expectedPath = "",
				 const std::string& expectedQuery = "",
				 const std::string& expectedVersion = "",
				 int expectedError = 0)
{
	std::cout << "\n" << YELLOW << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << RESET << std::endl;
	std::cout << BLUE << "Test: " << testName << RESET << std::endl;
	std::cout << "Request data: \"";

	// Print request with visible \r\n
	for (size_t i = 0; i < requestData.length(); ++i) {
		if (requestData[i] == '\r')
			std::cout << "\\r";
		else if (requestData[i] == '\n')
			std::cout << "\\n";
		else
			std::cout << requestData[i];
	}
	std::cout << "\"" << std::endl;

	// Create a fresh request object
	Request req;

	// Parse the data (simulates recv() from socket)
	bool complete = req.parse(requestData);

	// Check results
	bool testPassed = false;

	if (shouldSucceed)
	{
		// Test should succeed - verify no errors and correct parsing
		if (complete && !req.hasError()) {
			bool methodMatch = (expectedMethod.empty() || req.getMethod() == expectedMethod);
			bool pathMatch = (expectedPath.empty() || req.getPath() == expectedPath);
			bool queryMatch = (expectedQuery.empty() || req.getQueryString() == expectedQuery);
			bool versionMatch = (expectedVersion.empty() || req.getHttpVersion() == expectedVersion);

			if (methodMatch && pathMatch && queryMatch && versionMatch)
			{
				testPassed = true;
				std::cout << GREEN << "✓ PASS" << RESET << std::endl;
				std::cout << "  Method:       " << req.getMethod() << std::endl;
				std::cout << "  URI:          " << req.getUri() << std::endl;
				std::cout << "  Path:         " << req.getPath() << std::endl;
				std::cout << "  Query String: " << (req.getQueryString().empty() ? "(none)" : req.getQueryString()) << std::endl;
				std::cout << "  HTTP Version: " << req.getHttpVersion() << std::endl;
			}
			else
			{
				std::cout << RED << "✗ FAIL - Parsing succeeded but values don't match" << RESET << std::endl;
				std::cout << "  Expected: " << expectedMethod << " " << expectedPath
						 << (expectedQuery.empty() ? "" : "?") << expectedQuery
						 << " " << expectedVersion << std::endl;
				std::cout << "  Got:      " << req.getMethod() << " " << req.getPath()
						 << (req.getQueryString().empty() ? "" : "?") << req.getQueryString()
						 << " " << req.getHttpVersion() << std::endl;
			}
		}
		else if (!complete)
		{
			std::cout << RED << "✗ FAIL - Parsing incomplete (waiting for more data?)" << RESET << std::endl;
			std::cout << "  State: " << req.getState() << std::endl;
		}
		else
		{
			std::cout << RED << "✗ FAIL - Got error " << req.getErrorCode() << RESET << std::endl;
		}
	}
	else
	{
		// Test should fail - verify correct error code
		if (req.hasError() && req.getErrorCode() == expectedError)
		{
			testPassed = true;
			std::cout << GREEN << "✓ PASS - Correctly rejected with error " << expectedError << RESET << std::endl;
		}
		else if (!req.hasError())
		{
			std::cout << RED << "✗ FAIL - Should have failed but succeeded" << RESET << std::endl;
		}
		else
		{
			std::cout << RED << "✗ FAIL - Expected error " << expectedError
					 << ", got " << req.getErrorCode() << RESET << std::endl;
		}
	}

	// Update statistics
	if (testPassed)
		g_passed++;
	else
		g_failed++;
}




/**
 * Test incremental parsing (simulates multiple recv() calls)
 * This is critical for non-blocking I/O
 */
void testIncrementalParsing()
{
	std::cout << "\n" << YELLOW << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << RESET << std::endl;
	std::cout << BLUE << "Test: Incremental Parsing (Multiple recv() chunks)" << RESET << std::endl;
	std::cout << "Simulating data arriving in small pieces over network\n" << std::endl;

	Request req;
	bool testPassed = true;

	// Simulate first recv() - only part of request line
	std::cout << "  Chunk 1: \"GET / HT\"" << std::endl;
	bool complete = req.parse("GET / HT");
	if (complete || req.hasError()) {
		std::cout << RED << "  ✗ Should wait for more data" << RESET << std::endl;
		testPassed = false;
	}
	else
	{
		std::cout << GREEN << "  ✓ Buffering, waiting for more..." << RESET << std::endl;
	}

	// Simulate second recv() - more data but still incomplete
	std::cout << "  Chunk 2: \"TP/1.\"" << std::endl;
	complete = req.parse("TP/1.");
	if (complete || req.hasError())
	{
		std::cout << RED << "  ✗ Should still wait for more data" << RESET << std::endl;
		testPassed = false;
	}
	else
	{
		std::cout << GREEN << "  ✓ Still buffering..." << RESET << std::endl;
	}

	// Simulate third recv() - final piece with CRLF
	std::cout << "  Chunk 3: \"1\\r\\n\"" << std::endl;
	complete = req.parse("1\r\n");
	if (complete && !req.hasError())
	{
		std::cout << GREEN << "  ✓ Request line complete!" << RESET << std::endl;
		std::cout << "    Parsed: " << req.getMethod() << " " << req.getUri()
				 << " " << req.getHttpVersion() << std::endl;
	}
	else
	{
		std::cout << RED << "  ✗ Failed to complete parsing" << RESET << std::endl;
		testPassed = false;
	}

	if (testPassed)
	{
		std::cout << GREEN << "\n✓ PASS - Incremental parsing works correctly" << RESET << std::endl;
		g_passed++;
	}
	else
	{
		std::cout << RED << "\n✗ FAIL - Incremental parsing broken" << RESET << std::endl;
		g_failed++;
	}
}

/**
 * Test buffer overflow protection
 */
void testBufferOverflow()
{
	std::cout << "\n" << YELLOW << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << RESET << std::endl;
	std::cout << BLUE << "Test: Buffer Overflow Protection" << RESET << std::endl;
	std::cout << "Sending very long request line without CRLF\n" << std::endl;

	Request req;

	// Create a very long request line (over 8KB limit)
	std::string malicious = "GET /";
	for (int i = 0; i < 9000; ++i)
		malicious += "A";
	malicious += " HTTP/1.1";  // No \r\n

	std::cout << "  Sending " << malicious.length() << " bytes without line ending..." << std::endl;

	req.parse(malicious);

	if (req.hasError() && req.getErrorCode() == 414) {
		std::cout << GREEN << "✓ PASS - Correctly rejected with 414 URI Too Long" << RESET << std::endl;
		g_passed++;
	} else {
		std::cout << RED << "✗ FAIL - Should have rejected oversized request" << RESET << std::endl;
		g_failed++;
	}
}

/**
 * Test keep-alive connection reset
 */
void testReset()
{
	std::cout << "\n" << YELLOW << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << RESET << std::endl;
	std::cout << BLUE << "Test: Request Reset (Keep-Alive Simulation)" << RESET << std::endl;
	std::cout << "Simulating multiple requests on same connection\n" << std::endl;

	Request req;

	// First request
	std::cout << "  Request 1: GET /first HTTP/1.1\\r\\n" << std::endl;
	req.parse("GET /first HTTP/1.1\r\n");
	if (req.getPath() == "/first") {
		std::cout << GREEN << "  ✓ First request parsed" << RESET << std::endl;
	}
	else
	{
		std::cout << RED << "  ✗ First request failed" << RESET << std::endl;
		g_failed++;
		return;
	}

	// Reset for keep-alive
	std::cout << "\n  Resetting connection for next request..." << std::endl;
	req.reset();

	// Second request
	std::cout << "  Request 2: POST /second HTTP/1.1\\r\\n" << std::endl;
	req.parse("POST /second HTTP/1.1\r\n");
	if (req.getMethod() == "POST" && req.getPath() == "/second") {
		std::cout << GREEN << "  ✓ Second request parsed correctly" << RESET << std::endl;
		std::cout << GREEN << "\n✓ PASS - Reset works for keep-alive" << RESET << std::endl;
		g_passed++;
	}
	else
	{
		std::cout << RED << "  ✗ Second request failed" << RESET << std::endl;
		std::cout << RED << "\n✗ FAIL - Reset didn't clear state" << RESET << std::endl;
		g_failed++;
	}
}

int main()
{
	std::cout << "\n" << BLUE;
	std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
	std::cout << "║                                                           ║\n";
	std::cout << "║   WEBSERV - Step 4.1: Request Line Parser Tests           ║\n";
	std::cout << "║   (Standalone - No Network Code Required)                 ║\n";
	std::cout << "║                                                           ║\n";
	std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
	std::cout << RESET << std::endl;

	// ========================================================================
	// VALID REQUESTS - Should succeed
	// ========================================================================

	std::cout << "\n" << YELLOW << "══════════════════════════════════════════════════════" << RESET << std::endl;
	std::cout << YELLOW << "  SECTION 1: Valid HTTP Requests" << RESET << std::endl;
	std::cout << YELLOW << "══════════════════════════════════════════════════════" << RESET << std::endl;

	testRequest("Basic GET request",
				"GET / HTTP/1.1\r\n",
				true, "GET", "/", "", "HTTP/1.1");

	testRequest("GET with file path",
				"GET /index.html HTTP/1.1\r\n",
				true, "GET", "/index.html", "", "HTTP/1.1");

	testRequest("GET with subdirectory",
				"GET /static/css/style.css HTTP/1.1\r\n",
				true, "GET", "/static/css/style.css", "", "HTTP/1.1");

	testRequest("GET with query string",
				"GET /search?q=webserv HTTP/1.1\r\n",
				true, "GET", "/search", "q=webserv", "HTTP/1.1");

	testRequest("GET with multiple query parameters",
				"GET /api?id=123&format=json&limit=10 HTTP/1.1\r\n",
				true, "GET", "/api", "id=123&format=json&limit=10", "HTTP/1.1");

	testRequest("POST request",
				"POST /api/upload HTTP/1.1\r\n",
				true, "POST", "/api/upload", "", "HTTP/1.1");

	testRequest("DELETE request",
				"DELETE /files/document.txt HTTP/1.1\r\n",
				true, "DELETE", "/files/document.txt", "", "HTTP/1.1");

	testRequest("HTTP/1.0 request",
				"GET /legacy.html HTTP/1.0\r\n",
				true, "GET", "/legacy.html", "", "HTTP/1.0");

	testRequest("HEAD request",
				"HEAD /status HTTP/1.1\r\n",
				true, "HEAD", "/status", "", "HTTP/1.1");

	testRequest("PUT request",
				"PUT /resource/123 HTTP/1.1\r\n",
				true, "PUT", "/resource/123", "", "HTTP/1.1");

	// ========================================================================
	// INVALID REQUESTS - Should fail with specific error codes
	// ========================================================================

	std::cout << "\n" << YELLOW << "══════════════════════════════════════════════════════" << RESET << std::endl;
	std::cout << YELLOW << "  SECTION 2: Invalid HTTP Requests (Error Handling)" << RESET << std::endl;
	std::cout << YELLOW << "══════════════════════════════════════════════════════" << RESET << std::endl;

	testRequest("Missing URI and version",
				"GET\r\n",
				false, "", "", "", "", 400);

	testRequest("Missing version",
				"GET /index.html\r\n",
				false, "", "", "", "", 400);

	testRequest("Missing method",
				"/index.html HTTP/1.1\r\n",
				false, "", "", "", "", 400);

	testRequest("Extra tokens after version",
				"GET / HTTP/1.1 EXTRA_STUFF\r\n",
				false, "", "", "", "", 400);

	testRequest("URI without leading slash",
				"GET index.html HTTP/1.1\r\n",
				false, "", "", "", "", 400);

	testRequest("Invalid HTTP method",
				"INVALID /page.html HTTP/1.1\r\n",
				false, "", "", "", "", 501);

	testRequest("Unsupported HTTP method (TRACE)",
				"TRACE / HTTP/1.1\r\n",
				false, "", "", "", "", 501);

	testRequest("HTTP/2.0 not supported",
				"GET / HTTP/2.0\r\n",
				false, "", "", "", "", 505);

	testRequest("HTTP/0.9 too old",
				"GET / HTTP/0.9\r\n",
				false, "", "", "", "", 505);

	testRequest("Lowercase HTTP (case-sensitive)",
				"GET / http/1.1\r\n",
				false, "", "", "", "", 505);

	testRequest("Malformed version",
				"GET / HTTP\r\n",
				false, "", "", "", "", 505);

	testRequest("Empty request line",
				"\r\n",
				false, "", "", "", "", 400);

	// ========================================================================
	// SPECIAL TESTS
	// ========================================================================

	std::cout << "\n" << YELLOW << "══════════════════════════════════════════════════════" << RESET << std::endl;
	std::cout << YELLOW << "  SECTION 3: Special Cases & Edge Cases" << RESET << std::endl;
	std::cout << YELLOW << "══════════════════════════════════════════════════════" << RESET << std::endl;

	testIncrementalParsing();
	testBufferOverflow();
	testReset();

	// ========================================================================
	// SUMMARY
	// ========================================================================

	std::cout << "\n" << BLUE;
	std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
	std::cout << "║                      TEST SUMMARY                         ║\n";
	std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
	std::cout << RESET << std::endl;

	int total = g_passed + g_failed;
	std::cout << "  Total tests:  " << total << std::endl;
	std::cout << GREEN << "  Passed:       " << g_passed << RESET << std::endl;
	std::cout << RED << "  Failed:       " << g_failed << RESET << std::endl;

	if (g_failed == 0) {
		std::cout << "\n" << GREEN << "🎉 ALL TESTS PASSED! Step 4.1 is complete! 🎉" << RESET << "\n" << std::endl;
		return 0;
	}
	else
	{
		std::cout << "\n" << RED << "❌ Some tests failed. Review the output above." << RESET << "\n" << std::endl;
		return 1;
	}
}
