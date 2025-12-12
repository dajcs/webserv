/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main_test6.1.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:54:34 by anemet            #+#    #+#             */
/*   Updated: 2025/12/12 09:12:35 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* ************************************************************************** */
/*                                                                            */
/*   test_response.cpp - Unit tests for Response Generation (Step 6.1)       */
/*                                                                            */
/* ************************************************************************** */

/*
	=================================
		TESTING WITHOUT NETWORK
	=================================

	Since the network layer isn't implemented yet, we test the Response class
	in isolation. This approach:

	1. Tests all response building logic directly
	2. Verifies HTTP format compliance
	3. Checks all status codes and headers
	4. Runs fast without socket overhead
	5. Is deterministic (no network timing issues)

	Testing Strategy:
	-----------------
	- Create Response objects using various methods
	- Build the response string
	- Parse/verify the built string contains expected components
	- Check HTTP format compliance (CRLF, headers, etc.)

	What We're Testing (Step 6.1 Requirements):
	------------------------------------------
	- Status line: "HTTP/1.1 STATUS_CODE REASON_PHRASE"
	- Standard headers: Content-Type, Content-Length, Date, Server, Connection
	- Response format: status line + headers + blank line + body
	- Default error pages for all error codes
*/

#include "Response.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>

// ===========================================
//  ANSI Color codes for pretty test output
// ===========================================
#define GREEN   "\033[32m"
#define RED     "\033[31m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"
#define RESET   "\033[0m"
#define BOLD    "\033[1m"

// Global test counters
static int g_passed = 0;
static int g_failed = 0;


// ===========================================
//  Test Helper Functions
// ===========================================

/*
	printTestHeader() - Display test section header
*/
void printTestHeader(const std::string& testName)
{
	std::cout << std::endl;
	std::cout << CYAN << BOLD << "═══════════════════════════════════════════" << RESET << std::endl;
	std::cout << CYAN << BOLD << "  " << testName << RESET << std::endl;
	std::cout << CYAN << BOLD << "═══════════════════════════════════════════" << RESET << std::endl;
}

/*
	assertTrue() - Assert a condition is true
*/
void assertTrue(bool condition, const std::string& testCase)
{
	if (condition)
	{
		std::cout << GREEN << "  ✓ PASS: " << testCase << RESET << std::endl;
		g_passed++;
	}
	else
	{
		std::cout << RED << "  ✗ FAIL: " << testCase << RESET << std::endl;
		g_failed++;
	}
}

/*
	assertEqual() - Assert two values are equal
*/
template<typename T>
void assertEqual(const T& actual, const T& expected, const std::string& testCase)
{
	if (actual == expected)
	{
		std::cout << GREEN << "  ✓ PASS: " << testCase << RESET << std::endl;
		g_passed++;
	}
	else
	{
		std::cout << RED << "  ✗ FAIL: " << testCase << RESET << std::endl;
		std::cout << RED << "    Expected: " << expected << RESET << std::endl;
		std::cout << RED << "    Actual:   " << actual << RESET << std::endl;
		g_failed++;
	}
}

/*
	assertContains() - Assert string contains substring
*/
void assertContains(const std::string& haystack, const std::string& needle,
					const std::string& testCase)
{
	if (haystack.find(needle) != std::string::npos)
	{
		std::cout << GREEN << "  ✓ PASS: " << testCase << RESET << std::endl;
		g_passed++;
	}
	else
	{
		std::cout << RED << "  ✗ FAIL: " << testCase << RESET << std::endl;
		std::cout << RED << "    String does not contain: \"" << needle << "\"" << RESET << std::endl;
		g_failed++;
	}
}

/*
	assertNotContains() - Assert string does NOT contain substring
*/
void assertNotContains(const std::string& haystack, const std::string& needle,
						const std::string& testCase)
{
	if (haystack.find(needle) == std::string::npos)
	{
		std::cout << GREEN << "  ✓ PASS: " << testCase << RESET << std::endl;
		g_passed++;
	}
	else
	{
		std::cout << RED << "  ✗ FAIL: " << testCase << RESET << std::endl;
		std::cout << RED << "    String should NOT contain: \"" << needle << "\"" << RESET << std::endl;
		g_failed++;
	}
}


// ===========================================
//  Test Cases: Status Line
// ===========================================

/*
	Test 1: Status Line Format

	HTTP responses MUST start with a status line:
	"HTTP/1.1 STATUS_CODE REASON_PHRASE\r\n"

	Examples:
	- "HTTP/1.1 200 OK\r\n"
	- "HTTP/1.1 404 Not Found\r\n"
*/
void testStatusLine()
{
	printTestHeader("STATUS LINE TESTS");

	// Test 200 OK
	{
		Response response;
		response.setStatus(200);
		std::string built = response.build();
		assertContains(built, "HTTP/1.1 200 OK\r\n", "200 OK status line");
	}

	// Test 404 Not Found
	{
		Response response;
		response.setStatus(404);
		std::string built = response.build();
		assertContains(built, "HTTP/1.1 404 Not Found\r\n", "404 Not Found status line");
	}

	// Test 500 Internal Server Error
	{
		Response response;
		response.setStatus(500);
		std::string built = response.build();
		assertContains(built, "HTTP/1.1 500 Internal Server Error\r\n", "500 status line");
	}

	// Test custom reason phrase
	{
		Response response;
		response.setStatus(418, "I'm a teapot");
		std::string built = response.build();
		assertContains(built, "HTTP/1.1 418 I'm a teapot\r\n", "Custom reason phrase");
	}
}


// ===========================================
//  Test Cases: Standard Headers
// ===========================================

/*
	Test 2: Content-Type Header

	Content-Type tells the browser how to interpret the response body.
	Without it, browsers may guess wrong (security risk!).
*/
void testContentTypeHeader()
{
	printTestHeader("CONTENT-TYPE HEADER TESTS");

	// Test HTML content type
	{
		Response response;
		response.setContentType("text/html");
		std::string built = response.build();
		assertContains(built, "Content-Type: text/html\r\n", "HTML content type");
	}

	// Test JSON content type
	{
		Response response;
		response.setContentType("application/json");
		std::string built = response.build();
		assertContains(built, "Content-Type: application/json\r\n", "JSON content type");
	}

	// Test via factory method
	{
		Response response = Response::ok("<html></html>", "text/html");
		std::string built = response.build();
		assertContains(built, "Content-Type: text/html\r\n", "Content-Type from ok() factory");
	}
}

/*
	Test 3: Content-Length Header

	Content-Length indicates the size of the body in bytes.
	Essential for HTTP/1.1 persistent connections.
*/
void testContentLengthHeader()
{
	printTestHeader("CONTENT-LENGTH HEADER TESTS");

	// Test auto-calculated Content-Length
	{
		Response response;
		response.setBody("Hello World");  // 11 bytes
		std::string built = response.build();
		assertContains(built, "Content-Length: 11\r\n", "Auto-calculated Content-Length");
	}

	// Test empty body
	{
		Response response;
		response.setBody("");
		std::string built = response.build();
		assertContains(built, "Content-Length: 0\r\n", "Content-Length for empty body");
	}

	// Test manual Content-Length
	{
		Response response;
		response.setContentLength(100);
		std::string built = response.build();
		assertContains(built, "Content-Length: 100\r\n", "Manual Content-Length");
	}

	// Test 204 No Content (should NOT have Content-Length)
	{
		Response response = Response::noContent();
		std::string built = response.build();
		assertNotContains(built, "Content-Length:", "204 No Content omits Content-Length");
	}
}

/*
	Test 4: Date Header

	Date header indicates when the response was generated.
	Format: "Date: Wed, 11 Dec 2025 12:00:00 GMT"
*/
void testDateHeader()
{
	printTestHeader("DATE HEADER TESTS");

	// Test Date header is added
	{
		Response response;
		response.addDateHeader();
		std::string built = response.build();
		assertContains(built, "Date: ", "Date header present");
		assertContains(built, " GMT\r\n", "Date header ends with GMT");
	}

	// Test addStandardHeaders includes Date
	{
		Response response;
		response.addStandardHeaders();
		std::string built = response.build();
		assertContains(built, "Date: ", "addStandardHeaders includes Date");
	}

	// Test formatHttpDate format
	{
		// January 1, 2025 00:00:00 GMT
		time_t testTime = 1735689600;
		std::string formatted = Response::formatHttpDate(testTime);
		// Should be "Wed, 01 Jan 2025 00:00:00 GMT"
		assertContains(formatted, "Jan 2025", "formatHttpDate contains month and year");
		assertContains(formatted, "GMT", "formatHttpDate contains GMT");
	}
}

/*
	Test 5: Server Header

	Server header identifies the server software.
	Example: "Server: webserv/1.0"
*/
void testServerHeader()
{
	printTestHeader("SERVER HEADER TESTS");

	// Test Server header is added
	{
		Response response;
		response.addServerHeader();
		std::string built = response.build();
		assertContains(built, "Server: webserv/1.0\r\n", "Server header present");
	}

	// Test addStandardHeaders includes Server
	{
		Response response;
		response.addStandardHeaders();
		std::string built = response.build();
		assertContains(built, "Server: webserv/1.0\r\n", "addStandardHeaders includes Server");
	}
}

/*
	Test 6: Connection Header

	Connection header controls whether the TCP connection stays open.
	- "Connection: keep-alive" - reuse connection for more requests
	- "Connection: close" - close connection after response
*/
void testConnectionHeader()
{
	printTestHeader("CONNECTION HEADER TESTS");

	// Test keep-alive (default)
	{
		Response response;
		response.setConnection(true);
		std::string built = response.build();
		assertContains(built, "Connection: keep-alive\r\n", "Connection keep-alive");
		assertTrue(response.shouldKeepAlive(), "shouldKeepAlive() returns true");
	}

	// Test close
	{
		Response response;
		response.setConnection(false);
		std::string built = response.build();
		assertContains(built, "Connection: close\r\n", "Connection close");
		assertTrue(!response.shouldKeepAlive(), "shouldKeepAlive() returns false");
	}
}


// ===========================================
//  Test Cases: Response Format
// ===========================================

/*
	Test 7: Complete Response Format

	HTTP response format:
	STATUS_LINE\r\n
	HEADER1: VALUE1\r\n
	HEADER2: VALUE2\r\n
	\r\n
	BODY
*/
void testResponseFormat()
{
	printTestHeader("RESPONSE FORMAT TESTS");

	// Test complete response structure
	{
		Response response;
		response.setStatus(200);
		response.setContentType("text/html");
		response.setBody("<html><body>Hello</body></html>");
		response.addStandardHeaders();
		std::string built = response.build();

		// Check status line comes first
		assertTrue(built.find("HTTP/1.1 200 OK\r\n") == 0, "Status line is first");

		// Check headers before body
		size_t headerEnd = built.find("\r\n\r\n");
		assertTrue(headerEnd != std::string::npos, "Headers end with blank line");

		// Check body comes after headers
		std::string body = built.substr(headerEnd + 4);
		assertContains(body, "<html><body>Hello</body></html>", "Body is after headers");
	}

	// Test CRLF line endings (not just \n)
	{
		Response response;
		response.setStatus(200);
		std::string built = response.build();
		assertContains(built, "\r\n", "Response uses CRLF line endings");
		assertNotContains(built.substr(0, 20), "\r\r", "No double CR");
	}

	// Test blank line separates headers from body
	{
		Response response;
		response.setBody("test body");
		std::string built = response.build();
		assertContains(built, "\r\n\r\ntest body", "Blank line before body");
	}
}


// ===========================================
//  Test Cases: Default Error Pages
// ===========================================

/*
	Test 8: Default Error Pages

	The server must have default error pages for all error codes.
	These are used when no custom error page is configured.
*/
void testDefaultErrorPages()
{
	printTestHeader("DEFAULT ERROR PAGES TESTS");

	// Common error codes to test
	int errorCodes[] = {400, 403, 404, 405, 413, 500, 501, 502, 504};
	int numCodes = sizeof(errorCodes) / sizeof(errorCodes[0]);

	for (int i = 0; i < numCodes; i++)
	{
		int code = errorCodes[i];
		Response response = Response::error(code);
		std::string built = response.build();

		std::ostringstream testName;
		testName << "Error " << code << " has valid status";
		assertEqual(response.getStatusCode(), code, testName.str());

		testName.str("");
		testName << "Error " << code << " has HTML body";
		assertContains(built, "<!DOCTYPE html>", testName.str());

		testName.str("");
		testName << "Error " << code << " shows error code in body";
		std::ostringstream codeStr;
		codeStr << code;
		assertContains(response.getBody(), codeStr.str(), testName.str());
	}

	// Test error page content
	{
		Response response = Response::error(404);
		std::string body = response.getBody();
		assertContains(body, "404", "404 error page shows code");
		assertContains(body, "Not Found", "404 error page shows reason");
		assertContains(body, "webserv", "Error page shows server name");
	}
}


// ===========================================
//  Test Cases: Factory Methods
// ===========================================

/*
	Test 9: Factory Methods

	Static factory methods for common response types:
	- Response::ok() for 200 OK
	- Response::error() for error responses
	- Response::redirect() for redirects
	- Response::noContent() for 204 No Content
*/
void testFactoryMethods()
{
	printTestHeader("FACTORY METHOD TESTS");

	// Test ok()
	{
		Response response = Response::ok("<p>Success</p>", "text/html");
		assertEqual(response.getStatusCode(), 200, "ok() returns 200 status");
		assertContains(response.getBody(), "Success", "ok() sets body");
		assertContains(response.build(), "Content-Type: text/html", "ok() sets content type");
	}

	// Test error()
	{
		Response response = Response::error(500);
		assertEqual(response.getStatusCode(), 500, "error() returns correct status");
		assertContains(response.build(), "Content-Type:", "error() sets content type");
		assertTrue(!response.getBody().empty(), "error() has body");
	}

	// Test redirect()
	{
		Response response = Response::redirect(301, "https://example.com/new");
		assertEqual(response.getStatusCode(), 301, "redirect() returns 301 status");
		assertContains(response.build(), "Location: https://example.com/new\r\n",
					   "redirect() sets Location header");
		assertContains(response.getBody(), "example.com/new", "redirect() body has link");
	}

	// Test noContent()
	{
		Response response = Response::noContent();
		assertEqual(response.getStatusCode(), 204, "noContent() returns 204 status");
		assertTrue(response.getBody().empty(), "noContent() has no body");
	}
}


// ===========================================
//  Test Cases: MIME Types
// ===========================================

/*
	Test 10: MIME Type Detection

	getMimeType() should return correct Content-Type for file extensions.
*/
void testMimeTypes()
{
	printTestHeader("MIME TYPE TESTS");

	// HTML
	assertEqual(Response::getMimeType(".html"), std::string("text/html; charset=UTF-8"),
				"MIME type for .html");
	assertEqual(Response::getMimeType(".htm"), std::string("text/html; charset=UTF-8"),
				"MIME type for .htm");

	// CSS and JS
	assertEqual(Response::getMimeType(".css"), std::string("text/css; charset=UTF-8"),
				"MIME type for .css");
	assertEqual(Response::getMimeType(".js"), std::string("application/javascript; charset=UTF-8"),
				"MIME type for .js");

	// JSON
	assertEqual(Response::getMimeType(".json"), std::string("application/json; charset=UTF-8"),
				"MIME type for .json");

	// Images
	assertEqual(Response::getMimeType(".png"), std::string("image/png"), "MIME type for .png");
	assertEqual(Response::getMimeType(".jpg"), std::string("image/jpeg"), "MIME type for .jpg");
	assertEqual(Response::getMimeType(".gif"), std::string("image/gif"), "MIME type for .gif");

	// Unknown extension
	assertEqual(Response::getMimeType(".xyz"), std::string("application/octet-stream"),
				"MIME type for unknown extension");
}


// ===========================================
//  Test Cases: Reason Phrases
// ===========================================

/*
	Test 11: Reason Phrases

	Each status code should have a standard reason phrase.
*/
void testReasonPhrases()
{
	printTestHeader("REASON PHRASE TESTS");

	// 2xx Success
	assertEqual(Response::getReasonPhrase(200), std::string("OK"), "Reason for 200");
	assertEqual(Response::getReasonPhrase(201), std::string("Created"), "Reason for 201");
	assertEqual(Response::getReasonPhrase(204), std::string("No Content"), "Reason for 204");

	// 3xx Redirection
	assertEqual(Response::getReasonPhrase(301), std::string("Moved Permanently"), "Reason for 301");
	assertEqual(Response::getReasonPhrase(302), std::string("Found"), "Reason for 302");
	assertEqual(Response::getReasonPhrase(304), std::string("Not Modified"), "Reason for 304");

	// 4xx Client Errors
	assertEqual(Response::getReasonPhrase(400), std::string("Bad Request"), "Reason for 400");
	assertEqual(Response::getReasonPhrase(403), std::string("Forbidden"), "Reason for 403");
	assertEqual(Response::getReasonPhrase(404), std::string("Not Found"), "Reason for 404");
	assertEqual(Response::getReasonPhrase(405), std::string("Method Not Allowed"), "Reason for 405");
	assertEqual(Response::getReasonPhrase(413), std::string("Payload Too Large"), "Reason for 413");

	// 5xx Server Errors
	assertEqual(Response::getReasonPhrase(500), std::string("Internal Server Error"), "Reason for 500");
	assertEqual(Response::getReasonPhrase(502), std::string("Bad Gateway"), "Reason for 502");
	assertEqual(Response::getReasonPhrase(504), std::string("Gateway Timeout"), "Reason for 504");

	// Unknown
	assertEqual(Response::getReasonPhrase(999), std::string("Unknown"), "Reason for unknown code");
}


// ===========================================
//  Test Cases: Getters
// ===========================================

/*
	Test 12: Getter Methods
*/
void testGetters()
{
	printTestHeader("GETTER TESTS");

	Response response;
	response.setStatus(404);
	response.setHeader("X-Custom", "value123");
	response.setBody("test body content");
	response.setConnection(true);

	assertEqual(response.getStatusCode(), 404, "getStatusCode()");
	assertEqual(response.getBody(), std::string("test body content"), "getBody()");
	assertEqual(response.getHeader("X-Custom"), std::string("value123"), "getHeader() existing");
	assertEqual(response.getHeader("NonExistent"), std::string(""), "getHeader() non-existing");
	assertTrue(response.hasHeader("X-Custom"), "hasHeader() existing");
	assertTrue(!response.hasHeader("NonExistent"), "hasHeader() non-existing");
	assertTrue(response.shouldKeepAlive(), "shouldKeepAlive()");
}


// ===========================================
//  Test Cases: Copy Semantics
// ===========================================

/*
	Test 13: Copy Constructor and Assignment

	Responses are often returned by value from functions,
	so copy semantics must work correctly.
*/
void testCopySemantics()
{
	printTestHeader("COPY SEMANTICS TESTS");

	// Copy constructor
	{
		Response original;
		original.setStatus(201);
		original.setBody("original body");
		original.setHeader("X-Test", "test");

		Response copy(original);
		assertEqual(copy.getStatusCode(), 201, "Copy constructor copies status");
		assertEqual(copy.getBody(), std::string("original body"), "Copy constructor copies body");
		assertEqual(copy.getHeader("X-Test"), std::string("test"), "Copy constructor copies headers");

		// Modify original, copy should not change
		original.setStatus(500);
		assertEqual(copy.getStatusCode(), 201, "Copy is independent of original");
	}

	// Copy assignment
	{
		Response original;
		original.setStatus(202);
		original.setBody("assigned body");

		Response copy;
		copy = original;
		assertEqual(copy.getStatusCode(), 202, "Assignment copies status");
		assertEqual(copy.getBody(), std::string("assigned body"), "Assignment copies body");
	}

	// Self-assignment
	{
		Response response;
		response.setStatus(200);
		response.setBody("test");
		// response = response;  // Self-assignment  -Werror doesn't like
		// workaround for self-assignment:
		Response& alias = response;
		response = alias;
		assertEqual(response.getStatusCode(), 200, "Self-assignment preserves status");
		assertEqual(response.getBody(), std::string("test"), "Self-assignment preserves body");
	}
}


// ===========================================
//  Test Cases: Binary Data
// ===========================================

/*
	Test 14: Binary Data Handling

	Response body can contain binary data (images, files).
	The Response class must handle null bytes correctly.
*/
void testBinaryData()
{
	printTestHeader("BINARY DATA TESTS");

	// Test body with null bytes
	{
		char binaryData[] = "hello\0world";  // 11 bytes including null
		Response response;
		response.setBody(binaryData, 11);

		assertEqual(response.getBody().size(), (size_t)11, "Binary body preserves size");
		assertContains(response.build(), "Content-Length: 11\r\n", "Content-Length for binary");
	}

	// Test setBody with char* and length
	{
		const char* data = "\x00\x01\x02\x03";
		Response response;
		response.setBody(data, 4);
		assertEqual(response.getBody().size(), (size_t)4, "setBody(char*, size) works");
	}
}


// ===========================================
//  Test Cases: Dirty Flag (Lazy Building)
// ===========================================

/*
	Test 15: Lazy Building / Dirty Flag

	The Response uses a "dirty" flag to avoid rebuilding
	the HTTP string unnecessarily.
*/
void testLazyBuilding()
{
	printTestHeader("LAZY BUILDING TESTS");

	Response response;
	response.setStatus(200);
	response.setBody("initial");

	// First build
	std::string first = response.build();
	assertContains(first, "initial", "First build has initial body");

	// Modify and rebuild
	response.setBody("modified");
	std::string second = response.build();
	assertContains(second, "modified", "Second build has modified body");
	assertNotContains(second, "initial", "Second build doesn't have old body");

	// Multiple builds without modification should return same result
	std::string third = response.build();
	assertEqual(second, third, "Unmodified builds return same result");
}


// ===========================================
//  Main Test Runner
// ===========================================

int main()
{
	std::cout << std::endl;
	std::cout << BOLD << "╔═══════════════════════════════════════════════════════════╗" << RESET << std::endl;
	std::cout << BOLD << "║     WEBSERV RESPONSE GENERATION TESTS (Step 6.1)          ║" << RESET << std::endl;
	std::cout << BOLD << "╚═══════════════════════════════════════════════════════════╝" << RESET << std::endl;

	// Run all test suites
	testStatusLine();
	testContentTypeHeader();
	testContentLengthHeader();
	testDateHeader();
	testServerHeader();
	testConnectionHeader();
	testResponseFormat();
	testDefaultErrorPages();
	testFactoryMethods();
	testMimeTypes();
	testReasonPhrases();
	testGetters();
	testCopySemantics();
	testBinaryData();
	testLazyBuilding();

	// Print summary
	std::cout << std::endl;
	std::cout << BOLD << "═══════════════════════════════════════════════════════════" << RESET << std::endl;
	std::cout << BOLD << "                    TEST SUMMARY" << RESET << std::endl;
	std::cout << BOLD << "═══════════════════════════════════════════════════════════" << RESET << std::endl;
	std::cout << GREEN << "  Passed: " << g_passed << RESET << std::endl;
	std::cout << RED   << "  Failed: " << g_failed << RESET << std::endl;
	std::cout << BOLD << "  Total:  " << (g_passed + g_failed) << RESET << std::endl;
	std::cout << BOLD << "═══════════════════════════════════════════════════════════" << RESET << std::endl;

	if (g_failed == 0)
	{
		std::cout << std::endl;
		std::cout << GREEN << BOLD << "  ✓ ALL TESTS PASSED!" << RESET << std::endl;
		std::cout << std::endl;
		return 0;
	}
	else
	{
		std::cout << std::endl;
		std::cout << RED << BOLD << "  ✗ SOME TESTS FAILED" << RESET << std::endl;
		std::cout << "\n" << std::endl;
		return 1;
	}
}

