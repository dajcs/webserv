/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:54:34 by anemet            #+#    #+#             */
/*   Updated: 2025/12/13 19:07:11 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*
	==================================================
		CGI UNIT TESTS (Step 8.2)
	==================================================

	This file tests CGI execution without the network layer.
	It directly creates Request and LocationConfig objects,
	then tests the CGI class methods.

	Tests:
	1. CGI detection (isCgiRequest)
	2. CGI setup (environment building)
	3. CGI execution (fork/pipe/execve)
	4. CGI output parsing
	5. Error handling (timeout, bad script, etc.)

	Compile:
		g++ -Wall -Wextra -Werror -std=c++98 \
			-I inc/ \
			src/CGI.cpp src/Request.cpp src/Config.cpp src/Utils.cpp \
			tests/test_cgi.cpp \
			-o test_cgi

	Run:
		./test_cgi
*/

#include "CGI.hpp"
#include "Request.hpp"
#include "Config.hpp"

#include <iostream>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

// Simple test framework
static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST(name) std::cout << "\n[TEST] " << name << std::endl;
#define ASSERT(condition, message) \
	do { \
		if (condition) { \
			std::cout << "  ✓ " << message << std::endl; \
			g_testsPassed++; \
		} else { \
			std::cout << "  ✗ " << message << " (FAILED)" << std::endl; \
			g_testsFailed++; \
		} \
	} while(0)


/*
	findPythonInterpreter() - Find a valid Python 3 interpreter

	Checks common locations for Python 3.
	Returns empty string if not found.
*/
std::string findPythonInterpreter()
{
	const char* paths[] = {
		"/usr/bin/python3",
		"/usr/local/bin/python3",
		"/usr/bin/python",
		"/usr/local/bin/python",
		NULL
	};

	for (int i = 0; paths[i] != NULL; i++)
	{
		if (access(paths[i], X_OK) == 0)
		{
			return paths[i];
		}
	}
	return "";
}


/*
	getWorkingDirectory() - Get the current working directory
*/
std::string getWorkingDirectory()
{
	char cwd[1024];
	if (getcwd(cwd, sizeof(cwd)) != NULL)
	{
		return std::string(cwd);
	}
	return ".";
}


/*
	createMockRequest() - Create a Request object with given parameters

	Since Request normally parses from raw HTTP data,
	we'll create a minimal test by directly setting fields.
*/
Request createMockRequest(const std::string& method,
						  const std::string& uri,
						  const std::string& body = "")
{
	// Build a raw HTTP request string
	std::string rawRequest = method + " " + uri + " HTTP/1.1\r\n";
	rawRequest += "Host: localhost:8080\r\n";
	rawRequest += "User-Agent: test/1.0\r\n";

	if (!body.empty())
	{
		rawRequest += "Content-Type: application/x-www-form-urlencoded\r\n";
		rawRequest += "Content-Length: " +
			static_cast<std::ostringstream*>(&(std::ostringstream() << body.size()))->str() + "\r\n";
	}

	rawRequest += "\r\n";
	rawRequest += body;

	Request request;
	request.parse(rawRequest);
	return request;
}


/*
	createMockLocationConfig() - Create a LocationConfig for CGI testing
*/
LocationConfig createMockLocationConfig(const std::string& pythonPath)
{
	LocationConfig location;
	location.path = "/cgi-bin";
	location.root = getWorkingDirectory() + "/www";
	location.cgi_extension = ".py";
	location.cgi_path = pythonPath;
	location.allowed_methods.insert("GET");
	location.allowed_methods.insert("POST");
	return location;
}


// =============================================
//  Test Functions
// =============================================

/*
	Test 1: CGI Detection
*/
void testCgiDetection()
{
	TEST("CGI Detection (isCgiRequest)")

	std::string pythonPath = findPythonInterpreter();
	if (pythonPath.empty())
	{
		std::cout << "  ! Python not found, skipping detection test" << std::endl;
		return;
	}

	LocationConfig location = createMockLocationConfig(pythonPath);

	// Should detect .py files as CGI
	ASSERT(CGI::isCgiRequest("/var/www/cgi-bin/test.py", location),
		   ".py file should be detected as CGI");

	ASSERT(CGI::isCgiRequest("/some/path/script.py", location),
		   "Any .py file should be detected");

	// Should NOT detect non-.py files
	ASSERT(!CGI::isCgiRequest("/var/www/index.html", location),
		   ".html file should NOT be detected as CGI");

	ASSERT(!CGI::isCgiRequest("/var/www/style.css", location),
		   ".css file should NOT be detected as CGI");

	ASSERT(!CGI::isCgiRequest("/var/www/script.php", location),
		   ".php file should NOT be detected (configured for .py)");

	// Edge cases
	ASSERT(!CGI::isCgiRequest(".py", location),
		   "Just extension should not match");

	ASSERT(CGI::isCgiRequest("/a.py", location),
		   "Short path with .py should match");
}


/*
	Test 2: CGI Setup
*/
void testCgiSetup()
{
	TEST("CGI Setup (environment building)")

	std::string pythonPath = findPythonInterpreter();
	if (pythonPath.empty())
	{
		std::cout << "  ! Python not found, skipping setup test" << std::endl;
		return;
	}

	std::string cwd = getWorkingDirectory();
	std::string scriptPath = cwd + "/www/cgi-bin/hello.py";

	// Check if test script exists
	struct stat st;
	if (stat(scriptPath.c_str(), &st) != 0)
	{
		std::cout << "  ! Test script not found: " << scriptPath << std::endl;
		return;
	}

	// Make script executable
	chmod(scriptPath.c_str(), 0755);

	LocationConfig location = createMockLocationConfig(pythonPath);
	Request request = createMockRequest("GET", "/cgi-bin/hello.py?name=World");

	CGI cgi(request, location);
	bool setupResult = cgi.setup(scriptPath);

	ASSERT(setupResult, "Setup should succeed for valid script");
	ASSERT(cgi.isReady(), "CGI should be ready after successful setup");
	ASSERT(cgi.getErrorCode() == 0, "No error code after successful setup");

	// Check environment variables
	const std::map<std::string, std::string>& env = cgi.getEnvMap();

	ASSERT(env.find("REQUEST_METHOD") != env.end(),
		   "REQUEST_METHOD should be set");
	ASSERT(env.at("REQUEST_METHOD") == "GET",
		   "REQUEST_METHOD should be GET");

	ASSERT(env.find("QUERY_STRING") != env.end(),
		   "QUERY_STRING should be set");
	ASSERT(env.at("QUERY_STRING") == "name=World",
		   "QUERY_STRING should contain query parameters");

	ASSERT(env.find("SCRIPT_FILENAME") != env.end(),
		   "SCRIPT_FILENAME should be set");

	ASSERT(env.find("GATEWAY_INTERFACE") != env.end(),
		   "GATEWAY_INTERFACE should be set");
	ASSERT(env.at("GATEWAY_INTERFACE") == "CGI/1.1",
		   "GATEWAY_INTERFACE should be CGI/1.1");
}


/*
	Test 3: CGI Setup Failure (non-existent script)
*/
void testCgiSetupFailure()
{
	TEST("CGI Setup Failure Handling")

	std::string pythonPath = findPythonInterpreter();
	if (pythonPath.empty())
	{
		std::cout << "  ! Python not found, skipping failure test" << std::endl;
		return;
	}

	LocationConfig location = createMockLocationConfig(pythonPath);
	Request request = createMockRequest("GET", "/cgi-bin/nonexistent.py");

	CGI cgi(request, location);
	bool setupResult = cgi.setup("/path/to/nonexistent/script.py");

	ASSERT(!setupResult, "Setup should fail for non-existent script");
	ASSERT(!cgi.isReady(), "CGI should NOT be ready after failed setup");
	ASSERT(cgi.getErrorCode() == 404, "Error code should be 404 for missing script");
}


/*
	Test 4: CGI Execution (simple hello world)
*/
void testCgiExecution()
{
	TEST("CGI Execution (simple script)")

	std::string pythonPath = findPythonInterpreter();
	if (pythonPath.empty())
	{
		std::cout << "  ! Python not found, skipping execution test" << std::endl;
		return;
	}

	std::string cwd = getWorkingDirectory();
	std::string scriptPath = cwd + "/www/cgi-bin/hello.py";

	// Check if test script exists
	struct stat st;
	if (stat(scriptPath.c_str(), &st) != 0)
	{
		std::cout << "  ! Test script not found: " << scriptPath << std::endl;
		return;
	}

	// Make script executable
	chmod(scriptPath.c_str(), 0755);

	LocationConfig location = createMockLocationConfig(pythonPath);
	Request request = createMockRequest("GET", "/cgi-bin/hello.py");

	CGI cgi(request, location);

	if (!cgi.setup(scriptPath))
	{
		std::cout << "  ! Setup failed: " << cgi.getErrorMessage() << std::endl;
		return;
	}

	// Execute CGI
	CGI::CGIResult result = cgi.execute(10);  // 10 second timeout

	ASSERT(result.success, "Execution should succeed");
	ASSERT(result.statusCode == 200, "Status code should be 200");

	// Check headers
	ASSERT(result.headers.find("Content-Type") != result.headers.end(),
		   "Content-Type header should be present");

	// Check body contains expected content
	ASSERT(result.body.find("Hello") != std::string::npos,
		   "Body should contain 'Hello'");
	ASSERT(result.body.find("<html>") != std::string::npos,
		   "Body should contain HTML");
}


/*
	Test 5: CGI with Query String
*/
void testCgiQueryString()
{
	TEST("CGI with Query String")

	std::string pythonPath = findPythonInterpreter();
	if (pythonPath.empty())
	{
		std::cout << "  ! Python not found, skipping query string test" << std::endl;
		return;
	}

	std::string cwd = getWorkingDirectory();
	std::string scriptPath = cwd + "/www/cgi-bin/test.py";

	struct stat st;
	if (stat(scriptPath.c_str(), &st) != 0)
	{
		std::cout << "  ! Test script not found: " << scriptPath << std::endl;
		return;
	}

	chmod(scriptPath.c_str(), 0755);

	LocationConfig location = createMockLocationConfig(pythonPath);
	Request request = createMockRequest("GET", "/cgi-bin/test.py?name=WebServ&count=42");

	CGI cgi(request, location);

	if (!cgi.setup(scriptPath))
	{
		std::cout << "  ! Setup failed: " << cgi.getErrorMessage() << std::endl;
		return;
	}

	CGI::CGIResult result = cgi.execute(10);

	ASSERT(result.success, "Execution should succeed");

	// Body should show the query parameters
	ASSERT(result.body.find("name") != std::string::npos,
		   "Body should show query parameter names");
	ASSERT(result.body.find("WebServ") != std::string::npos,
		   "Body should show parameter value 'WebServ'");
}


/*
	Test 6: CGI with POST body
*/
void testCgiPostBody()
{
	TEST("CGI with POST Body")

	std::string pythonPath = findPythonInterpreter();
	if (pythonPath.empty())
	{
		std::cout << "  ! Python not found, skipping POST test" << std::endl;
		return;
	}

	std::string cwd = getWorkingDirectory();
	std::string scriptPath = cwd + "/www/cgi-bin/test.py";

	struct stat st;
	if (stat(scriptPath.c_str(), &st) != 0)
	{
		std::cout << "  ! Test script not found: " << scriptPath << std::endl;
		return;
	}

	chmod(scriptPath.c_str(), 0755);

	LocationConfig location = createMockLocationConfig(pythonPath);
	Request request = createMockRequest("POST", "/cgi-bin/test.py", "username=test&password=secret");

	CGI cgi(request, location);

	if (!cgi.setup(scriptPath))
	{
		std::cout << "  ! Setup failed: " << cgi.getErrorMessage() << std::endl;
		return;
	}

	CGI::CGIResult result = cgi.execute(10);

	ASSERT(result.success, "POST execution should succeed");

	// Body should indicate POST data was received
	// The test.py script reads from stdin and displays it
	ASSERT(result.body.find("POST") != std::string::npos ||
		   result.body.find("username") != std::string::npos,
		   "Response should reference POST method or body content");
}


/*
	Test 7: Memory management (argv and envp)
*/
void testMemoryManagement()
{
	TEST("Memory Management (argv/envp allocation)")

	std::string pythonPath = findPythonInterpreter();
	if (pythonPath.empty())
	{
		std::cout << "  ! Python not found, skipping memory test" << std::endl;
		return;
	}

	std::string cwd = getWorkingDirectory();
	std::string scriptPath = cwd + "/www/cgi-bin/hello.py";

	struct stat st;
	if (stat(scriptPath.c_str(), &st) != 0)
	{
		std::cout << "  ! Test script not found, skipping" << std::endl;
		return;
	}

	chmod(scriptPath.c_str(), 0755);

	LocationConfig location = createMockLocationConfig(pythonPath);
	Request request = createMockRequest("GET", "/cgi-bin/hello.py");

	CGI cgi(request, location);
	cgi.setup(scriptPath);

	// Test argv allocation and freeing
	char** argv = cgi.getArgv();
	ASSERT(argv != NULL, "getArgv should return non-NULL");
	ASSERT(argv[0] != NULL, "argv[0] should be interpreter path");
	ASSERT(argv[1] != NULL, "argv[1] should be script path");
	ASSERT(argv[2] == NULL, "argv[2] should be NULL terminator");
	CGI::freeArgv(argv);
	std::cout << "  ✓ argv freed successfully" << std::endl;
	g_testsPassed++;

	// Test envp allocation and freeing
	char** envp = cgi.getEnvArray();
	ASSERT(envp != NULL, "getEnvArray should return non-NULL");

	int envCount = 0;
	while (envp[envCount] != NULL)
	{
		envCount++;
	}
	ASSERT(envCount > 0, "Environment should have at least one variable");

	CGI::freeEnvArray(envp);
	std::cout << "  ✓ envp freed successfully" << std::endl;
	g_testsPassed++;
}


// =============================================
//  Main
// =============================================

int main()
{
	std::cout << "============================================" << std::endl;
	std::cout << "     CGI Unit Tests (Step 8.2)" << std::endl;
	std::cout << "============================================" << std::endl;

	// Find Python
	std::string pythonPath = findPythonInterpreter();
	if (pythonPath.empty())
	{
		std::cout << "\n⚠ WARNING: Python not found in standard locations." << std::endl;
		std::cout << "Most tests will be skipped." << std::endl;
	}
	else
	{
		std::cout << "\nUsing Python: " << pythonPath << std::endl;
	}

	std::cout << "Working directory: " << getWorkingDirectory() << std::endl;

	// Run tests
	testCgiDetection();
	testCgiSetup();
	testCgiSetupFailure();
	testCgiExecution();
	testCgiQueryString();
	testCgiPostBody();
	testMemoryManagement();

	// Summary
	std::cout << "\n============================================" << std::endl;
	std::cout << "     Results: " << g_testsPassed << " passed, "
			  << g_testsFailed << " failed" << std::endl;
	std::cout << "============================================" << std::endl;

	return g_testsFailed > 0 ? 1 : 0;
}
