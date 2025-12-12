/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main_test7.2.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:54:34 by anemet            #+#    #+#             */
/*   Updated: 2025/12/12 17:07:28 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*
	=================================
		POST METHOD TEST SUITE
	=================================

	This file tests the POST method and file upload functionality
	at the Router/Request/Response layer, without requiring network I/O.

	Testing Strategy:
	-----------------
	We simulate HTTP requests by:
	1. Creating Request objects manually
	2. Feeding them raw HTTP data (as would come from network)
	3. Passing to Router::route()
	4. Verifying the Response

	This approach tests:
	- Multipart parsing
	- File saving
	- Error handling
	- URL-encoded form parsing

	Compile with:
		g++ -std=c++98 -Wall -Wextra -Werror -I inc \
			src/test_post.cpp src/Request.cpp src/Response.cpp \
			src/Router.cpp src/Config.cpp src/Utils.cpp \
			-o test_post

	Run:
		./test_post
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>

#include "Request.hpp"
#include "Response.hpp"
#include "Router.hpp"
#include "Config.hpp"
#include "Utils.hpp"

// ANSI color codes for test output
#define GREEN   "\033[32m"
#define RED     "\033[31m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"
#define RESET   "\033[0m"
#define BOLD    "\033[1m"

// Test counters
static int g_testsPassed = 0;
static int g_testsFailed = 0;


/*
	Helper: Print test result
*/
void printResult(const std::string& testName, bool passed, const std::string& details = "")
{
	if (passed)
	{
		std::cout << GREEN << "[PASS] " << RESET << testName;
		g_testsPassed++;
	}
	else
	{
		std::cout << RED << "[FAIL] " << RESET << testName;
		g_testsFailed++;
	}

	if (!details.empty())
	{
		std::cout << " - " << details;
	}
	std::cout << std::endl;
}


/*
	Helper: Create a test Request from raw HTTP data
*/
Request createRequest(const std::string& rawHttp)
{
	Request request;
	request.parse(rawHttp);
	return request;
}


/*
	Helper: Check if file exists
*/
bool fileExists(const std::string& path)
{
	struct stat st;
	return stat(path.c_str(), &st) == 0;
}


/*
	Helper: Read file content
*/
std::string readFile(const std::string& path)
{
	std::ifstream file(path.c_str(), std::ios::binary);
	if (!file)
		return "";

	std::stringstream ss;
	ss << file.rdbuf();
	return ss.str();
}


/*
	Helper: Clean up test files
*/
void cleanup(const std::string& path)
{
	unlink(path.c_str());
}


// =============================================
//  Test: Utils::extractBoundary
// =============================================
void testExtractBoundary()
{
	std::cout << "\n" << YELLOW << "=== Testing extractBoundary ===" << RESET << std::endl;

	// Test 1: Standard boundary
	{
		std::string contentType = "multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxk";
		std::string boundary = Utils::extractBoundary(contentType);
		printResult("Standard boundary",
					boundary == "----WebKitFormBoundary7MA4YWxk",
					"Got: " + boundary);
	}

	// Test 2: Quoted boundary
	{
		std::string contentType = "multipart/form-data; boundary=\"my-boundary-123\"";
		std::string boundary = Utils::extractBoundary(contentType);
		printResult("Quoted boundary",
					boundary == "my-boundary-123",
					"Got: " + boundary);
	}

	// Test 3: No boundary
	{
		std::string contentType = "multipart/form-data";
		std::string boundary = Utils::extractBoundary(contentType);
		printResult("Missing boundary",
					boundary.empty(),
					"Got: " + boundary);
	}

	// Test 4: Boundary with spaces
	{
		std::string contentType = "multipart/form-data; boundary = simple";
		std::string boundary = Utils::extractBoundary(contentType);
		// Note: The space after = might be included depending on implementation
		printResult("Boundary with spaces",
					!boundary.empty(),
					"Got: '" + boundary + "'");
	}
}


// =============================================
//  Test: Utils::parseContentDisposition
// =============================================
void testParseContentDisposition()
{
	std::cout << "\n" << YELLOW << "=== Testing parseContentDisposition ===" << RESET << std::endl;

	// Test 1: Field without filename
	{
		std::string header = "form-data; name=\"description\"";
		std::string name, filename;
		Utils::parseContentDisposition(header, name, filename);
		printResult("Field without filename",
					name == "description" && filename.empty(),
					"name=" + name + ", filename=" + filename);
	}

	// Test 2: File with filename
	{
		std::string header = "form-data; name=\"upload\"; filename=\"photo.jpg\"";
		std::string name, filename;
		Utils::parseContentDisposition(header, name, filename);
		printResult("File with filename",
					name == "upload" && filename == "photo.jpg",
					"name=" + name + ", filename=" + filename);
	}

	// Test 3: Complex filename
	{
		std::string header = "form-data; name=\"file\"; filename=\"my document (1).pdf\"";
		std::string name, filename;
		Utils::parseContentDisposition(header, name, filename);
		printResult("Complex filename",
					name == "file" && filename == "my document (1).pdf",
					"name=" + name + ", filename=" + filename);
	}
}


// =============================================
//  Test: Utils::parseMultipart
// =============================================
void testParseMultipart()
{
	std::cout << "\n" << YELLOW << "=== Testing parseMultipart ===" << RESET << std::endl;

	// Test 1: Simple multipart with one file
	{
		std::string boundary = "----TestBoundary";
		std::string body =
			"------TestBoundary\r\n"
			"Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
			"Content-Type: text/plain\r\n"
			"\r\n"
			"Hello, World!\r\n"
			"------TestBoundary--\r\n";

		std::vector<MultipartPart> parts = Utils::parseMultipart(body, boundary);

		bool success = (parts.size() == 1 &&
						parts[0].name == "file" &&
						parts[0].filename == "test.txt" &&
						parts[0].data == "Hello, World!");

		printResult("Simple multipart with one file",
					success,
					"parts=" + (parts.empty() ? "0" : parts[0].name));
	}

	// Test 2: Multiple parts
	{
		std::string boundary = "----TestBoundary";
		std::string body =
			"------TestBoundary\r\n"
			"Content-Disposition: form-data; name=\"description\"\r\n"
			"\r\n"
			"My file description\r\n"
			"------TestBoundary\r\n"
			"Content-Disposition: form-data; name=\"file\"; filename=\"data.bin\"\r\n"
			"Content-Type: application/octet-stream\r\n"
			"\r\n"
			"BINARY_DATA_HERE\r\n"
			"------TestBoundary--\r\n";

		std::vector<MultipartPart> parts = Utils::parseMultipart(body, boundary);

		bool success = (parts.size() == 2 &&
						parts[0].name == "description" &&
						parts[0].filename.empty() &&
						parts[1].name == "file" &&
						parts[1].filename == "data.bin");

		std::stringstream details;
		details << "parts=" << parts.size();
		printResult("Multiple parts (field + file)",
					success,
					details.str());
	}

	// Test 3: Binary data
	{
		std::string boundary = "----TestBoundary";
		// Create body with binary content (including null bytes)
		std::string binaryData;
		binaryData += '\x00';
		binaryData += '\x01';
		binaryData += '\x02';
		binaryData += "text";
		binaryData += '\x00';

		std::string body =
			"------TestBoundary\r\n"
			"Content-Disposition: form-data; name=\"bin\"; filename=\"data.bin\"\r\n"
			"Content-Type: application/octet-stream\r\n"
			"\r\n" +
			binaryData +
			"\r\n"
			"------TestBoundary--\r\n";

		std::vector<MultipartPart> parts = Utils::parseMultipart(body, boundary);

		bool success = (parts.size() == 1 && parts[0].data == binaryData);

		printResult("Binary data preservation",
					success,
					"data length=" + (parts.empty() ? "0" :
						static_cast<std::ostringstream&>(std::ostringstream() << parts[0].data.length()).str()));
	}
}


// =============================================
//  Test: Utils::sanitizeFilename
// =============================================
void testSanitizeFilename()
{
	std::cout << "\n" << YELLOW << "=== Testing sanitizeFilename ===" << RESET << std::endl;

	// Test 1: Normal filename
	{
		std::string result = Utils::sanitizeFilename("photo.jpg");
		printResult("Normal filename",
					result == "photo.jpg",
					"Got: " + result);
	}

	// Test 2: Path traversal attempt
	{
		std::string result = Utils::sanitizeFilename("../../../etc/passwd");
		bool safe = (result.find("..") == std::string::npos &&
					 result.find("/") == std::string::npos);
		printResult("Path traversal blocked",
					safe,
					"Got: " + result);
	}

	// Test 3: Absolute path
	{
		std::string result = Utils::sanitizeFilename("/etc/passwd");
		bool safe = (result.find("/") == std::string::npos);
		printResult("Absolute path stripped",
					safe,
					"Got: " + result);
	}

	// Test 4: Special characters
	{
		std::string result = Utils::sanitizeFilename("file; rm -rf /.txt");
		bool safe = (result.find(";") == std::string::npos &&
					 result.find(" ") == std::string::npos);
		printResult("Special characters removed",
					safe,
					"Got: " + result);
	}

	// Test 5: Hidden file (starts with .)
	{
		std::string result = Utils::sanitizeFilename(".htaccess");
		bool safe = (result[0] != '.');
		printResult("Leading dot removed",
					safe,
					"Got: " + result);
	}
}


// =============================================
//  Test: Utils::urlDecode
// =============================================
void testUrlDecode()
{
	std::cout << "\n" << YELLOW << "=== Testing urlDecode ===" << RESET << std::endl;

	// Test 1: Simple encoding
	{
		std::string result = Utils::urlDecode("Hello%20World");
		printResult("Space decoding",
					result == "Hello World",
					"Got: " + result);
	}

	// Test 2: Plus as space
	{
		std::string result = Utils::urlDecode("Hello+World");
		printResult("Plus as space",
					result == "Hello World",
					"Got: " + result);
	}

	// Test 3: Special characters
	{
		std::string result = Utils::urlDecode("a%26b%3Dc");
		printResult("Special characters",
					result == "a&b=c",
					"Got: " + result);
	}
}


// =============================================
//  Test: Full POST Request Flow
// =============================================
void testPostRequest()
{
	std::cout << "\n" << YELLOW << "=== Testing Full POST Request ===" << RESET << std::endl;

	// Create a config for testing
	Config config("config/default.conf");
	Router router(config);

	// Test 1: Simple multipart file upload
	{
		std::string boundary = "----TestBoundary12345";
		std::string body =
			"------TestBoundary12345\r\n"
			"Content-Disposition: form-data; name=\"file\"; filename=\"testfile.txt\"\r\n"
			"Content-Type: text/plain\r\n"
			"\r\n"
			"This is test content for the uploaded file.\r\n"
			"------TestBoundary12345--\r\n";

		std::string rawHttp =
			"POST /upload HTTP/1.1\r\n"
			"Host: localhost:8080\r\n"
			"Content-Type: multipart/form-data; boundary=----TestBoundary12345\r\n"
			"Content-Length: " +
			static_cast<std::ostringstream&>(std::ostringstream() << body.length()).str() + "\r\n"
			"\r\n" +
			body;

		Request request;
		request.parse(rawHttp);

		Response response = router.route(request, 8080);

		bool success = (response.getStatusCode() == 201 ||
						response.getStatusCode() == 200);

		printResult("Multipart file upload",
					success,
					"Status: " +
					static_cast<std::ostringstream&>(std::ostringstream() << response.getStatusCode()).str());

		// Output response for debugging
		if (!success)
		{
			std::cout << "  Response body: " << response.getBody() << std::endl;
		}
	}

	// Test 2: URL-encoded form data
	{
		std::string body = "username=testuser&email=test%40example.com";

		std::string rawHttp =
			"POST /upload HTTP/1.1\r\n"
			"Host: localhost:8080\r\n"
			"Content-Type: application/x-www-form-urlencoded\r\n"
			"Content-Length: " +
			static_cast<std::ostringstream&>(std::ostringstream() << body.length()).str() + "\r\n"
			"\r\n" +
			body;

		Request request;
		request.parse(rawHttp);

		Response response = router.route(request, 8080);

		bool success = (response.getStatusCode() == 200 ||
						response.getStatusCode() == 201);

		printResult("URL-encoded form data",
					success,
					"Status: " +
					static_cast<std::ostringstream&>(std::ostringstream() << response.getStatusCode()).str());
	}

	// Test 3: POST to location without upload_path (should fail)
	{
		std::string body = "test data";

		std::string rawHttp =
			"POST / HTTP/1.1\r\n"
			"Host: localhost:8080\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Length: " +
			static_cast<std::ostringstream&>(std::ostringstream() << body.length()).str() + "\r\n"
			"\r\n" +
			body;

		Request request;
		request.parse(rawHttp);

		Response response = router.route(request, 8080);

		// Should get 403 or 405 (forbidden or method not allowed)
		bool success = (response.getStatusCode() == 403 ||
						response.getStatusCode() == 405);

		printResult("POST to location without upload (should fail)",
					success,
					"Status: " +
					static_cast<std::ostringstream&>(std::ostringstream() << response.getStatusCode()).str());
	}
}


// =============================================
//  Main
// =============================================
int main()
{
	std::cout << "\n" << YELLOW << "========================================" << RESET << std::endl;
	std::cout << YELLOW << "  POST Method & File Upload Test Suite  " << RESET << std::endl;
	std::cout << YELLOW << "========================================" << RESET << std::endl;

	// Run all tests
	testExtractBoundary();
	testParseContentDisposition();
	testParseMultipart();
	testSanitizeFilename();
	testUrlDecode();
	testPostRequest();

	// Print summary
	std::cout << std::endl;
	std::cout << BOLD << "═══════════════════════════════════════════════════════════" << RESET << std::endl;
	std::cout << BOLD << "                    TEST SUMMARY" << RESET << std::endl;
	std::cout << BOLD << "═══════════════════════════════════════════════════════════" << RESET << std::endl;
	std::cout << GREEN << "  Passed: " << g_testsPassed << RESET << std::endl;
	std::cout << RED   << "  Failed: " << g_testsFailed << RESET << std::endl;
	std::cout << BOLD << "  Total:  " << (g_testsPassed + g_testsFailed) << RESET << std::endl;
	std::cout << BOLD << "═══════════════════════════════════════════════════════════" << RESET << std::endl;

	if (g_testsFailed == 0)
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
