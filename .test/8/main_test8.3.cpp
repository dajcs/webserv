/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main_test8.3.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:54:34 by anemet            #+#    #+#             */
/*   Updated: 2025/12/14 16:29:18 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


/*
    =================================================================
        CGI ERROR HANDLING TEST SUITE
    =================================================================

    This test file exercises all CGI error handling scenarios without
    requiring the network layer. It directly uses the CGI class to
    validate error codes and messages.

    Test Categories:
    ----------------
    1. Script Not Found (404)
    2. Script Not Executable (500)
    3. Interpreter Not Found (500)
    4. CGI Timeout (504)
    5. Invalid CGI Output (502)
    6. CGI Crash/Signal (500)
    7. Successful Execution (200)
*/

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

#include "CGI.hpp"
#include "Request.hpp"
#include "Config.hpp"

// Test result tracking
static int g_testsRun = 0;
static int g_testsPassed = 0;
static int g_testsFailed = 0;

// ANSI color codes for pretty output
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_RESET   "\033[0m"

/*
	Test assertion macros
*/
#define TEST_ASSERT(condition, message) \
	do { \
		g_testsRun++; \
		if (condition) { \
			g_testsPassed++; \
			std::cout << COLOR_GREEN << "[PASS] " << COLOR_RESET << message << std::endl; \
		} else { \
			g_testsFailed++; \
			std::cout << COLOR_RED << "[FAIL] " << COLOR_RESET << message << std::endl; \
			std::cout << "       Expected: " << #condition << std::endl; \
		} \
	} while (0)

#define TEST_SECTION(name) \
    std::cout << std::endl << COLOR_BLUE << "=== " << name << " ===" << COLOR_RESET << std::endl


// =========================================
//  Test Helper: Create Mock Request
// =========================================
/*
	Creates a minimal Request object for CGI testing.
	The Request needs to be "complete" for CGI to read headers/body.
*/
Request createMockRequest(const std::string& method,
							const std::string& path,
							const std::string& queryString = "",
							const std::string& body = "")
{
	Request request;

	// Build a minimal HTTP request
	std::string httpRequest = method + " " + path;
	if (!queryString.empty())
	{
		httpRequest += "?" + queryString;
	}
	httpRequest += " HTTP/1.1\r\n";
	httpRequest += "Host: localhost:8080\r\n";

	if (!body.empty())
	{
		std::ostringstream oss;
		oss << body.size();
		httpRequest += "Content-Length: " + oss.str() + "\r\n";
		httpRequest += "Content-Type: application/x-www-form-urlencoded\r\n";
	}

	httpRequest += "\r\n";
	httpRequest += body;

	request.parse(httpRequest);

	return request;
}


// =========================================
//  Test Helper: Create Mock Location
// =========================================
/*
	Creates a LocationConfig with CGI enabled.
*/
LocationConfig createCgiLocation(const std::string& extension = ".py",
									const std::string& interpreter = "/usr/bin/python3")
{
	LocationConfig location;
	location.path = "/cgi-bin";
	location.root = "www";
	location.cgi_extension = extension;
	location.cgi_path = interpreter;
	location.allowed_methods.insert("GET");
	location.allowed_methods.insert("POST");
	return location;
}


// =========================================
//  Test Helper: Create Test Scripts
// =========================================

// Directory for test CGI scripts
const std::string TEST_CGI_DIR = "www/cgi-bin";

void setupTestDirectory()
{
	// Create test directory if it doesn't exist
	mkdir("www", 0755);
	mkdir(TEST_CGI_DIR.c_str(), 0755);
}

void createTestScript(const std::string& name, const std::string& content, bool executable = true)
{
	std::string path = TEST_CGI_DIR + "/" + name;
	std::ofstream file(path.c_str());
	file << content;
	file.close();

	if (executable)
	{
		chmod(path.c_str(), 0755);  // rwxr-xr-x
	}
	else
	{
		chmod(path.c_str(), 0644);  // rw-r--r-- (not executable)
	}
}

void removeTestScript(const std::string& name)
{
	std::string path = TEST_CGI_DIR + "/" + name;
	unlink(path.c_str());
}


// =========================================
//  Test 1: Script Not Found (404)
// =========================================
void testScriptNotFound()
{
	TEST_SECTION("Test 1: Script Not Found (404)");

	Request request = createMockRequest("GET", "/cgi-bin/nonexistent.py");
	LocationConfig location = createCgiLocation();

	CGI cgi(request, location);

	// Try to set up with a script that doesn't exist
	std::string scriptPath = "www/cgi-bin/nonexistent.py";
	bool setupResult = cgi.setup(scriptPath);

	TEST_ASSERT(!setupResult, "setup() should return false for nonexistent script");
	TEST_ASSERT(cgi.getErrorCode() == 404, "Error code should be 404 Not Found");
	TEST_ASSERT(!cgi.isReady(), "CGI should not be ready");

	std::cout << "       Error message: " << cgi.getErrorMessage() << std::endl;
}


// =========================================
//  Test 2: Script Not Executable (403)
// =========================================
void testScriptNotExecutable()
{
	TEST_SECTION("Test 2: Script Not Executable (403)");
	// Create a script without execute permission
	std::string scriptContent =
		"#!/usr/bin/env python3\n"
		"print('Content-Type: text/plain\\r')\n"
		"print('\\r')\n"
		"print('Hello')\n";

	createTestScript("noexec.py", scriptContent, false);  // false = not executable

	Request request = createMockRequest("GET", "/cgi-bin/noexec.py");
	LocationConfig location = createCgiLocation();

	CGI cgi(request, location);

	std::string scriptPath = TEST_CGI_DIR + "/noexec.py";
	bool setupResult = cgi.setup(scriptPath);

	TEST_ASSERT(!setupResult, "setup() should return false for non-executable script");
	TEST_ASSERT(cgi.getErrorCode() == 403, "Error code should be 403 (Forbidden)");
	TEST_ASSERT(!cgi.isReady(), "CGI should not be ready");

	std::cout << "       Error message: " << cgi.getErrorMessage() << std::endl;

	removeTestScript("noexec.py");
}


// =========================================
//  Test 3: Interpreter Not Found (500)
// =========================================
void testInterpreterNotFound()
{
	TEST_SECTION("Test 3: Interpreter Not Found (500)");

	// Create a valid script
	std::string scriptContent =
		"#!/usr/bin/env python3\n"
		"print('Content-Type: text/plain\\r')\n"
		"print('\\r')\n"
		"print('Hello')\n";

	createTestScript("test.py", scriptContent, true);

	Request request = createMockRequest("GET", "/cgi-bin/test.py");

	// Use a non-existent interpreter
	LocationConfig location = createCgiLocation(".py", "/nonexistent/interpreter");

	CGI cgi(request, location);

	std::string scriptPath = TEST_CGI_DIR + "/test.py";
	bool setupResult = cgi.setup(scriptPath);

	TEST_ASSERT(!setupResult, "setup() should return false for nonexistent interpreter");
	TEST_ASSERT(cgi.getErrorCode() == 500, "Error code should be 500");
	TEST_ASSERT(!cgi.isReady(), "CGI should not be ready");

	std::cout << "       Error message: " << cgi.getErrorMessage() << std::endl;

	removeTestScript("test.py");
}


// =========================================
//  Test 4: CGI Timeout (504)
// =========================================
void testCgiTimeout()
{
	TEST_SECTION("Test 4: CGI Timeout (504)");

	// Create a script that sleeps for a long time
	std::string scriptContent =
		"#!/usr/bin/env python3\n"
		"import time\n"
		"time.sleep(60)  # Sleep for 60 seconds\n"
		"print('Content-Type: text/plain\\r')\n"
		"print('\\r')\n"
		"print('Done')\n";

	createTestScript("slow.py", scriptContent, true);

	Request request = createMockRequest("GET", "/cgi-bin/slow.py");
	LocationConfig location = createCgiLocation();

	CGI cgi(request, location);

	std::string scriptPath = TEST_CGI_DIR + "/slow.py";
	bool setupResult = cgi.setup(scriptPath);

	TEST_ASSERT(setupResult, "setup() should succeed for valid script");
	TEST_ASSERT(cgi.isReady(), "CGI should be ready");

	// Execute with a short timeout (2 seconds)
	std::cout << "       Executing with 2 second timeout (expect ~2s delay)..." << std::endl;
	CGI::CGIResult result = cgi.execute(2);

	TEST_ASSERT(!result.success, "execute() should fail due to timeout");
	TEST_ASSERT(result.statusCode == 504, "Status code should be 504 Gateway Timeout");

	std::cout << "       Error message: " << result.errorMessage << std::endl;

	removeTestScript("slow.py");
}


// =========================================
//  Test 5: Invalid CGI Output (502)
// =========================================
void testInvalidCgiOutput()
{
	TEST_SECTION("Test 5: Invalid CGI Output (502)");

	// Create a script that produces invalid output (no headers)
	std::string scriptContent =
		"#!/usr/bin/env python3\n"
		"# Missing the required blank line between headers and body\n"
		"print('This is not valid CGI output')\n";

	createTestScript("invalid_output.py", scriptContent, true);

	Request request = createMockRequest("GET", "/cgi-bin/invalid_output.py");
	LocationConfig location = createCgiLocation();

	CGI cgi(request, location);

	std::string scriptPath = TEST_CGI_DIR + "/invalid_output.py";
	cgi.setup(scriptPath);

	CGI::CGIResult result = cgi.execute(5);

	TEST_ASSERT(!result.success, "execute() should fail for invalid output");
	TEST_ASSERT(result.statusCode == 502, "Status code should be 502 Bad Gateway");

	std::cout << "       Error message: " << result.errorMessage << std::endl;

	removeTestScript("invalid_output.py");
}


// =========================================
//  Test 6: Successful CGI Execution (200)
// =========================================
void testSuccessfulCgi()
{
	TEST_SECTION("Test 6: Successful CGI Execution (200)");

	// Create a valid CGI script
	std::string scriptContent =
		"#!/usr/bin/env python3\n"
		"import os\n"
		"print('Content-Type: text/html\\r')\n"
		"print('\\r')\n"
		"print('<h1>CGI Works!</h1>')\n"
		"print('<p>Request Method: ' + os.environ.get('REQUEST_METHOD', 'unknown') + '</p>')\n";

	createTestScript("success.py", scriptContent, true);

	Request request = createMockRequest("GET", "/cgi-bin/success.py");
	LocationConfig location = createCgiLocation();

	CGI cgi(request, location);

	std::string scriptPath = TEST_CGI_DIR + "/success.py";
	bool setupResult = cgi.setup(scriptPath);

	TEST_ASSERT(setupResult, "setup() should succeed");
	TEST_ASSERT(cgi.isReady(), "CGI should be ready");

	CGI::CGIResult result = cgi.execute(5);

	TEST_ASSERT(result.success, "execute() should succeed");
	TEST_ASSERT(result.statusCode == 200, "Status code should be 200");
	TEST_ASSERT(!result.body.empty(), "Body should not be empty");
	TEST_ASSERT(result.headers.count("Content-Type") > 0, "Content-Type header should be present");

	std::cout << "       Body preview: " << result.body.substr(0, 50) << "..." << std::endl;

	removeTestScript("success.py");
}


// =========================================
//  Test 7: CGI with POST Data
// =========================================
void testCgiWithPost()
{
	TEST_SECTION("Test 7: CGI with POST Data");

	// Create a script that reads and echoes POST data
	std::string scriptContent =
		"#!/usr/bin/env python3\n"
		"import sys\n"
		"import os\n"
		"# Read POST body from stdin\n"
		"content_length = int(os.environ.get('CONTENT_LENGTH', 0))\n"
		"post_data = sys.stdin.read(content_length) if content_length > 0 else ''\n"
		"print('Content-Type: text/plain\\r')\n"
		"print('\\r')\n"
		"print('Received: ' + post_data)\n";

	createTestScript("echo_post.py", scriptContent, true);

	Request request = createMockRequest("POST", "/cgi-bin/echo_post.py", "", "name=test&value=123");
	LocationConfig location = createCgiLocation();

	CGI cgi(request, location);

	std::string scriptPath = TEST_CGI_DIR + "/echo_post.py";
	cgi.setup(scriptPath);

	CGI::CGIResult result = cgi.execute(5);

	TEST_ASSERT(result.success, "execute() should succeed");
	TEST_ASSERT(result.body.find("name=test") != std::string::npos,
				"Body should contain POST data");

	std::cout << "       Response body: " << result.body << std::endl;

	removeTestScript("echo_post.py");
}


// =========================================
//  Test 8: CGI with Query String
// =========================================
void testCgiWithQueryString()
{
	TEST_SECTION("Test 8: CGI with Query String");

	// Create a script that reads query string
	std::string scriptContent =
		"#!/usr/bin/env python3\n"
		"import os\n"
		"query = os.environ.get('QUERY_STRING', '')\n"
		"print('Content-Type: text/plain\\r')\n"
		"print('\\r')\n"
		"print('Query: ' + query)\n";

	createTestScript("query.py", scriptContent, true);

	Request request = createMockRequest("GET", "/cgi-bin/query.py", "foo=bar&baz=123");
	LocationConfig location = createCgiLocation();

	CGI cgi(request, location);

	std::string scriptPath = TEST_CGI_DIR + "/query.py";
	cgi.setup(scriptPath);

	CGI::CGIResult result = cgi.execute(5);

	TEST_ASSERT(result.success, "execute() should succeed");
	TEST_ASSERT(result.body.find("foo=bar") != std::string::npos,
				"Body should contain query string");

	std::cout << "       Response body: " << result.body << std::endl;

	removeTestScript("query.py");
}


// =========================================
//  Test 9: CGI Status Header
// =========================================
void testCgiStatusHeader()
{
	TEST_SECTION("Test 9: CGI Status Header");

	// Create a script that sets a custom status code
	std::string scriptContent =
		"#!/usr/bin/env python3\n"
		"print('Status: 404 Not Found\\r')\n"
		"print('Content-Type: text/plain\\r')\n"
		"print('\\r')\n"
		"print('Custom 404 page from CGI')\n";

	createTestScript("status.py", scriptContent, true);

	Request request = createMockRequest("GET", "/cgi-bin/status.py");
	LocationConfig location = createCgiLocation();

	CGI cgi(request, location);

	std::string scriptPath = TEST_CGI_DIR + "/status.py";
	cgi.setup(scriptPath);

	CGI::CGIResult result = cgi.execute(5);

	TEST_ASSERT(result.success, "execute() should succeed (Status header is valid)");
	TEST_ASSERT(result.statusCode == 404, "Status code should be 404 from CGI header");

	std::cout << "       Status from CGI: " << result.statusCode << std::endl;

	removeTestScript("status.py");
}


// =========================================
//  Test 10: CGI Redirect (Location Header)
// =========================================
void testCgiRedirect()
{
	TEST_SECTION("Test 10: CGI Redirect (Location Header)");

	// Create a script that does a redirect
	std::string scriptContent =
		"#!/usr/bin/env python3\n"
		"print('Location: /new-location\\r')\n"
		"print('\\r')\n";

	createTestScript("redirect.py", scriptContent, true);

	Request request = createMockRequest("GET", "/cgi-bin/redirect.py");
	LocationConfig location = createCgiLocation();

	CGI cgi(request, location);

	std::string scriptPath = TEST_CGI_DIR + "/redirect.py";
	cgi.setup(scriptPath);

	CGI::CGIResult result = cgi.execute(5);

	TEST_ASSERT(result.success, "execute() should succeed");
	TEST_ASSERT(result.statusCode == 302, "Status code should be 302 (redirect)");
	TEST_ASSERT(result.headers.count("Location") > 0, "Location header should be present");
	TEST_ASSERT(result.headers["Location"] == "/new-location", "Location should be /new-location");

	std::cout << "       Redirect to: " << result.headers["Location"] << std::endl;

	removeTestScript("redirect.py");
}


// =========================================
//  Main Test Runner
// =========================================
int main()
{
	std::cout << COLOR_YELLOW << "\n"
				<< "╔═══════════════════════════════════════════════════════════════╗\n"
				<< "║           WEBSERV CGI ERROR HANDLING TEST SUITE               ║\n"
				<< "║                     Step 8.3 Tests                            ║\n"
				<< "╚═══════════════════════════════════════════════════════════════╝\n"
				<< COLOR_RESET << std::endl;

	// Check for Python3
	if (system("which python3 > /dev/null 2>&1") != 0)
	{
		std::cerr << COLOR_RED << "ERROR: python3 not found. Cannot run CGI tests."
					<< COLOR_RESET << std::endl;
		return 1;
	}

	// Setup test directory
	setupTestDirectory();

	// Run all tests
	testScriptNotFound();       // 404
	testScriptNotExecutable();  // 500
	testInterpreterNotFound();  // 500
	testCgiTimeout();           // 504
	testInvalidCgiOutput();     // 502
	testSuccessfulCgi();        // 200
	testCgiWithPost();          // POST handling
	testCgiWithQueryString();   // Query string
	testCgiStatusHeader();      // Custom status
	testCgiRedirect();          // Location redirect

	// Print summary
	std::cout << "\n" << COLOR_YELLOW
				<< "═══════════════════════════════════════════════════════════════\n"
				<< COLOR_RESET;

	std::cout << "Tests Run:    " << g_testsRun << std::endl;
	std::cout << COLOR_GREEN << "Tests Passed: " << g_testsPassed << COLOR_RESET << std::endl;

	if (g_testsFailed > 0)
	{
		std::cout << COLOR_RED << "Tests Failed: " << g_testsFailed << COLOR_RESET << std::endl;
	}

	std::cout << COLOR_YELLOW
				<< "═══════════════════════════════════════════════════════════════\n"
				<< COLOR_RESET << std::endl;

	return (g_testsFailed > 0) ? 1 : 0;
}
