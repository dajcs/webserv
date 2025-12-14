/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main_test8.1.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:54:34 by anemet            #+#    #+#             */
/*   Updated: 2025/12/14 16:15:26 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/* ************************************************************************** */
/*                                                                            */
/*   Test file for CGI Detection & Setup (Step 8.1)                          */
/*                                                                            */
/* ************************************************************************** */

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <cassert>
#include "CGI.hpp"
#include "Request.hpp"
#include "Config.hpp"

/*
	Test CGI Detection and Setup

	This test verifies:
	1. CGI detection based on file extension
	2. Environment variable generation
	3. Script and interpreter validation
*/

// Helper to print test results
void printTest(const std::string& name, bool passed)
{
	std::cout << (passed ? "[PASS] " : "[FAIL] ") << name << std::endl;
}

// Test 1: CGI detection by extension
void testCgiDetection()
{
	std::cout << "\n=== Test CGI Detection ===" << std::endl;

	LocationConfig loc;

	// Test without CGI configured
	loc.cgi_extension = "";
	loc.cgi_path = "";
	printTest("No CGI config -> not CGI",
			  !CGI::isCgiRequest("/test.py", loc));

	// Test with CGI configured
	loc.cgi_extension = ".py";
	loc.cgi_path = "/usr/bin/python3";
	printTest("Python script detected",
			  CGI::isCgiRequest("/cgi-bin/test.py", loc));
	printTest("HTML not detected as CGI",
			  !CGI::isCgiRequest("/index.html", loc));
	printTest("PHP not detected (wrong extension)",
			  !CGI::isCgiRequest("/test.php", loc));

	// Test PHP configuration
	loc.cgi_extension = ".php";
	loc.cgi_path = "/usr/bin/php-cgi";
	printTest("PHP script detected",
			  CGI::isCgiRequest("/info.php", loc));
	printTest("Python not detected (wrong extension)",
			  !CGI::isCgiRequest("/test.py", loc));
}

// Test 2: Environment variable building
void testEnvironmentBuilding()
{
	std::cout << "\n=== Test Environment Building ===" << std::endl;

	// Create a mock request
	Request request;
	std::string rawRequest =
		"GET /cgi-bin/test.py?name=World&count=5 HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"User-Agent: TestClient/1.0\r\n"
		"Accept: text/html\r\n"
		"Accept-Language: en-US\r\n"
		"\r\n";
	request.parse(rawRequest);

	// Create location config
	LocationConfig loc;
	loc.path = "/cgi-bin";
	loc.root = "/var/www";
	loc.cgi_extension = ".py";
	loc.cgi_path = "/usr/bin/python3";

	// Create CGI and build environment (without validation)
	CGI cgi(request, loc);

	// We can't call setup() without a real script, but we can test
	// the static method and verify the CGI was initialized
	printTest("CGI object created", !cgi.isReady());
	printTest("CGI detection works",
			  CGI::isCgiRequest("/cgi-bin/test.py", loc));

	// Display what the environment would contain
	std::cout << "\nExpected environment variables for this request:" << std::endl;
	std::cout << "  REQUEST_METHOD=GET" << std::endl;
	std::cout << "  QUERY_STRING=name=World&count=5" << std::endl;
	std::cout << "  SERVER_PROTOCOL=HTTP/1.1" << std::endl;
	std::cout << "  HTTP_HOST=localhost:8080" << std::endl;
	std::cout << "  HTTP_USER_AGENT=TestClient/1.0" << std::endl;
	std::cout << "  HTTP_ACCEPT=text/html" << std::endl;
	std::cout << "  HTTP_ACCEPT_LANGUAGE=en-US" << std::endl;
}

// Test 3: POST request environment
void testPostEnvironment()
{
	std::cout << "\n=== Test POST Request Environment ===" << std::endl;

	// Create a POST request
	Request request;
	std::string rawRequest =
		"POST /cgi-bin/upload.py HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"Content-Type: application/x-www-form-urlencoded\r\n"
		"Content-Length: 21\r\n"
		"\r\n"
		"username=john&age=30";
	request.parse(rawRequest);

	std::cout << "POST request parsed:" << std::endl;
	std::cout << "  Method: " << request.getMethod() << std::endl;
	std::cout << "  Path: " << request.getPath() << std::endl;
	std::cout << "  Content-Type: " << request.getHeader("Content-Type") << std::endl;
	std::cout << "  Content-Length: " << request.getHeader("Content-Length") << std::endl;
	std::cout << "  Body: " << request.getBody() << std::endl;

	printTest("POST method detected", request.getMethod() == "POST");
	printTest("Content-Type parsed",
			  request.getHeader("Content-Type") == "application/x-www-form-urlencoded");
	printTest("Content-Length parsed", request.getHeader("Content-Length") == "21");
	printTest("Body received", request.getBody() == "username=john&age=30");
}

// Test 4: PATH_INFO extraction
void testPathInfo()
{
	std::cout << "\n=== Test PATH_INFO Extraction ===" << std::endl;

	// Request with extra path info
	Request request;
	std::string rawRequest =
		"GET /cgi-bin/api.py/users/123/profile HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n";
	request.parse(rawRequest);

	std::cout << "Request path: " << request.getPath() << std::endl;
	std::cout << "Expected SCRIPT_NAME: /cgi-bin/api.py" << std::endl;
	std::cout << "Expected PATH_INFO: /users/123/profile" << std::endl;

	printTest("Full path preserved",
			  request.getPath() == "/cgi-bin/api.py/users/123/profile");
}

// Test 5: Full CGI setup with real script (if available)
void testFullSetup()
{
	std::cout << "\n=== Test Full CGI Setup ===" << std::endl;

	// First, check if we have Python available
	if (access("/usr/bin/python3", X_OK) != 0)
	{
		std::cout << "Skipping: /usr/bin/python3 not available" << std::endl;
		return;
	}

	// Create a simple test script
	const char* testScript = "/tmp/webserv_test.py";
	std::ofstream script(testScript);
	if (!script)
	{
		std::cout << "Skipping: Cannot create test script" << std::endl;
		return;
	}

	script << "#!/usr/bin/env python3\n";
	script << "import os\n";
	script << "print('Content-Type: text/plain\\r')\n";
	script << "print('\\r')\n";
	script << "print('CGI Test Success!')\n";
	script.close();

	// Make it executable
	if (chmod(testScript, 0755) != 0)
	{
		std::cout << "Skipping: Cannot make script executable" << std::endl;
		return;
	}

	// Create request and location
	Request request;
	std::string rawRequest =
		"GET /test.py?message=hello HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"\r\n";
	request.parse(rawRequest);

	LocationConfig loc;
	loc.path = "/";
	loc.root = "/tmp";
	loc.cgi_extension = ".py";
	loc.cgi_path = "/usr/bin/python3";

	// Setup CGI
	CGI cgi(request, loc);
	bool setupResult = cgi.setup(testScript);

	printTest("CGI setup successful", setupResult);
	printTest("CGI is ready", cgi.isReady());
	printTest("No error code", cgi.getErrorCode() == 0);

	if (setupResult)
	{
		std::cout << "\nGenerated environment variables:" << std::endl;
		const std::map<std::string, std::string>& env = cgi.getEnvMap();
		for (std::map<std::string, std::string>::const_iterator it = env.begin();
			 it != env.end(); ++it)
		{
			std::cout << "  " << it->first << "=" << it->second << std::endl;
		}

		// Test getEnvArray
		char** envArray = cgi.getEnvArray();
		printTest("Environment array created", envArray != NULL);
		if (envArray)
		{
			int count = 0;
			while (envArray[count] != NULL)
			{
				count++;
			}
			printTest("Environment count matches",
					  count == static_cast<int>(env.size()));
			CGI::freeEnvArray(envArray);
		}

		// Test getArgv
		char** argv = cgi.getArgv();
		printTest("Argv array created", argv != NULL);
		if (argv)
		{
			printTest("argv[0] is interpreter",
					  std::string(argv[0]) == "/usr/bin/python3");
			printTest("argv[1] is script",
					  std::string(argv[1]) == "webserv_test.py");
			printTest("argv[2] is NULL", argv[2] == NULL);
			CGI::freeArgv(argv);
		}
	}
	else
	{
		std::cout << "Error: " << cgi.getErrorMessage() << std::endl;
	}

	// Cleanup
	unlink(testScript);
}

// Test 6: Error handling
void testErrorHandling()
{
	std::cout << "\n=== Test Error Handling ===" << std::endl;

	Request request;
	std::string rawRequest =
		"GET /test.py HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n";
	request.parse(rawRequest);

	LocationConfig loc;
	loc.path = "/";
	loc.root = "/tmp";
	loc.cgi_extension = ".py";
	loc.cgi_path = "/usr/bin/python3";

	// Test: Script not found
	{
		CGI cgi(request, loc);
		bool result = cgi.setup("/nonexistent/script.py");
		printTest("Nonexistent script fails", !result);
		printTest("Error code is 404", cgi.getErrorCode() == 404);
	}

	// Test: Script not executable (create non-executable file)
	{
		const char* nonExecScript = "/tmp/webserv_noexec.py";
		std::ofstream script(nonExecScript);
		script << "print('test')\n";
		script.close();
		chmod(nonExecScript, 0644);  // Read-only

		CGI cgi(request, loc);
		bool result = cgi.setup(nonExecScript);
		printTest("Non-executable script fails", !result);
		printTest("Error code is 403", cgi.getErrorCode() == 403);

		unlink(nonExecScript);
	}

	// Test: Invalid interpreter
	{
		LocationConfig badLoc = loc;
		badLoc.cgi_path = "/nonexistent/python";

		const char* testScript = "/tmp/webserv_test2.py";
		std::ofstream script(testScript);
		script << "print('test')\n";
		script.close();
		chmod(testScript, 0755);

		CGI cgi(request, badLoc);
		bool result = cgi.setup(testScript);
		printTest("Invalid interpreter fails", !result);
		printTest("Error code is 500", cgi.getErrorCode() == 500);

		unlink(testScript);
	}
}

int main()
{
	std::cout << "========================================" << std::endl;
	std::cout << "   CGI Detection & Setup Tests" << std::endl;
	std::cout << "   Step 8.1 Implementation" << std::endl;
	std::cout << "========================================" << std::endl;

	testCgiDetection();
	testEnvironmentBuilding();
	testPostEnvironment();
	testPathInfo();
	testFullSetup();
	testErrorHandling();

	std::cout << "\n========================================" << std::endl;
	std::cout << "   Tests Complete" << std::endl;
}
