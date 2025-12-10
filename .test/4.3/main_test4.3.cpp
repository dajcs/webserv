/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main_test4.3.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:54:34 by anemet            #+#    #+#             */
/*   Updated: 2025/12/10 17:28:32 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


/* ************************************************************************** */
/*   test_request_step4_3.cpp - Test for HTTP Body Parsing                  */
/* ************************************************************************** */

#include "Request.hpp"
#include <iostream>
#include <string>
#include <sstream>

// Color codes
#define GREEN "\033[32m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define RESET "\033[0m"

int g_passed = 0;
int g_failed = 0;

void testRequest(const std::string& testName,
				 const std::string& requestData,
				 bool shouldSucceed,
				 const std::string& expectedBody = "",
				 int expectedError = 0)
{
	std::cout << "\n" << YELLOW << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << RESET << std::endl;
	std::cout << BLUE << "Test: " << testName << RESET << std::endl;

	Request req;
	bool complete = req.parse(requestData);

	bool testPassed = false;

	if (shouldSucceed)
	{
		if (complete && !req.hasError())
		{
			if (req.getBody() == expectedBody)
			{
				testPassed = true;
				std::cout << GREEN << "✓ PASS" << RESET << std::endl;
				std::cout << "  Method:       " << req.getMethod() << std::endl;
				std::cout << "  URI:          " << req.getUri() << std::endl;
				std::cout << "  Body Length:  " << req.getBody().length() << " bytes" << std::endl;
				if (!expectedBody.empty() && expectedBody.length() < 100)
				{
					std::cout << "  Body Content: \"" << req.getBody() << "\"" << std::endl;
				}
			}
			else
			{
				std::cout << RED << "✗ FAIL - Body mismatch" << RESET << std::endl;
				std::cout << "  Expected: \"" << expectedBody << "\"" << std::endl;
				std::cout << "  Got:      \"" << req.getBody() << "\"" << std::endl;
			}
		}
		else if (!complete)
		{
			std::cout << RED << "✗ FAIL - Parsing incomplete" << RESET << std::endl;
			std::cout << "  State: " << req.getState() << std::endl;
		}
		else
		{
			std::cout << RED << "✗ FAIL - Got error " << req.getErrorCode() << RESET << std::endl;
		}
	}
	else
	{
		if (req.hasError() && req.getErrorCode() == expectedError)
		{
			testPassed = true;
			std::cout << GREEN << "✓ PASS - Correctly rejected with error "
					 << expectedError << RESET << std::endl;
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

	if (testPassed)
		g_passed++;
	else
		g_failed++;
}

void testIncrementalBody()
{
	std::cout << "\n" << YELLOW << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << RESET << std::endl;
	std::cout << BLUE << "Test: Incremental Body Parsing (Multiple recv() calls)" << RESET << std::endl;

	Request req;

	// Send headers first
	std::string chunk1 = "POST /upload HTTP/1.1\r\n"
						 "Host: localhost\r\n"
						 "Content-Length: 27\r\n"
						 "\r\n";

	bool complete = req.parse(chunk1);
	if (complete)
	{
		std::cout << RED << "  ✗ Should not complete with only headers" << RESET << std::endl;
		g_failed++;
		return;
	}

	// Send body in chunks
	std::string chunk2 = "Hello, ";
	complete = req.parse(chunk2);
	if (complete)
	{
		std::cout << RED << "  ✗ Should not complete with partial body" << RESET << std::endl;
		g_failed++;
		return;
	}

	std::string chunk3 = "this is ";
	complete = req.parse(chunk3);
	if (complete)
	{
		std::cout << RED << "  ✗ Should not complete with partial body" << RESET << std::endl;
		g_failed++;
		return;
	}

	std::string chunk4 = "a test body!";
	complete = req.parse(chunk4);

	if (complete && !req.hasError() && req.getBody() == "Hello, this is a test body!")
	{
		std::cout << GREEN << "✓ PASS - Incremental parsing works" << RESET << std::endl;
		std::cout << "  Final body: \"" << req.getBody() << "\"" << std::endl;
		g_passed++;
	}
	else
	{
		std::cout << RED << "✗ FAIL - Incremental parsing failed" << RESET << std::endl;
		std::cout << "  Complete: " << complete << std::endl;
		std::cout << "  Body: \"" << req.getBody() << "\"" << std::endl;
		g_failed++;
	}
}

int main()
{
	std::cout << "\n" << BLUE;
	std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
	std::cout << "║                                                           ║\n";
	std::cout << "║   WEBSERV - Step 4.3: Request Body Handling Tests         ║\n";
	std::cout << "║                                                           ║\n";
	std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
	std::cout << RESET << std::endl;

	// ========================================================================
	// SECTION 1: Content-Length Body Parsing
	// ========================================================================

	std::cout << "\n" << YELLOW << "══════════════════════════════════════════════════════" << RESET << std::endl;
	std::cout << YELLOW << "  SECTION 1: Content-Length Body Parsing" << RESET << std::endl;
	std::cout << YELLOW << "══════════════════════════════════════════════════════" << RESET << std::endl;

	testRequest("Simple POST with body",
				"POST /upload HTTP/1.1\r\n"
				"Host: localhost\r\n"
				"Content-Length: 13\r\n"
				"\r\n"
				"Hello, World!",
				true, "Hello, World!");

	testRequest("POST with JSON body",
				"POST /api/data HTTP/1.1\r\n"
				"Host: localhost\r\n"
				"Content-Type: application/json\r\n"
				"Content-Length: 26\r\n"
				"\r\n"
				"{\"name\":\"test\",\"value\":42}",
				true, "{\"name\":\"test\",\"value\":42}");

	testRequest("POST with empty body (Content-Length: 0)",
				"POST /api/ping HTTP/1.1\r\n"
				"Host: localhost\r\n"
				"Content-Length: 0\r\n"
				"\r\n",
				true, "");

	testRequest("POST with multiline body",
				"POST /form HTTP/1.1\r\n"
				"Host: localhost\r\n"
				"Content-Length: 33\r\n"
				"\r\n"
				"name=John\nage=30\nemail=j@test.com",
				true, "name=John\nage=30\nemail=j@test.com");

	// ========================================================================
	// SECTION 2: Chunked Transfer Encoding
	// ========================================================================

	std::cout << "\n" << YELLOW << "══════════════════════════════════════════════════════" << RESET << std::endl;
	std::cout << YELLOW << "  SECTION 2: Chunked Transfer Encoding" << RESET << std::endl;
	std::cout << YELLOW << "══════════════════════════════════════════════════════" << RESET << std::endl;

	testRequest("Simple chunked body",
				"POST /upload HTTP/1.1\r\n"
				"Host: localhost\r\n"
				"Transfer-Encoding: chunked\r\n"
				"\r\n"
				"5\r\n"
				"Hello\r\n"
				"0\r\n"
				"\r\n",
				true, "Hello");

	testRequest("Multiple chunks",
				"POST /upload HTTP/1.1\r\n"
				"Host: localhost\r\n"
				"Transfer-Encoding: chunked\r\n"
				"\r\n"
				"5\r\n"
				"Hello\r\n"
				"7\r\n"
				" World!\r\n"
				"0\r\n"
				"\r\n",
				true, "Hello World!");

	testRequest("Chunked with hex uppercase",
				"POST /upload HTTP/1.1\r\n"
				"Host: localhost\r\n"
				"Transfer-Encoding: chunked\r\n"
				"\r\n"
				"A\r\n"
				"0123456789\r\n"
				"0\r\n"
				"\r\n",
				true, "0123456789");

	testRequest("Single chunk with data",
				"POST /upload HTTP/1.1\r\n"
				"Host: localhost\r\n"
				"Transfer-Encoding: chunked\r\n"
				"\r\n"
				"D\r\n"
				"Test message!\r\n"
				"0\r\n"
				"\r\n",
				true, "Test message!");

	// ========================================================================
	// SECTION 3: Error Cases
	// ========================================================================

	std::cout << "\n" << YELLOW << "══════════════════════════════════════════════════════" << RESET << std::endl;
	std::cout << YELLOW << "  SECTION 3: Error Handling" << RESET << std::endl;
	std::cout << YELLOW << "══════════════════════════════════════════════════════" << RESET << std::endl;

	testRequest("Body exceeds limit (413)",
				"POST /upload HTTP/1.1\r\n"
				"Host: localhost\r\n"
				"Content-Length: 99999999\r\n"
				"\r\n",
				false, "", 413);

	testRequest("Invalid chunk size (not hex)",
				"POST /upload HTTP/1.1\r\n"
				"Host: localhost\r\n"
				"Transfer-Encoding: chunked\r\n"
				"\r\n"
				"XYZ\r\n"
				"Hello\r\n"
				"0\r\n"
				"\r\n",
				false, "", 400);
/*
				testRequest("Chunked missing final CRLF",
				"POST /upload HTTP/1.1\r\n"
				"Host: localhost\r\n"
				"Transfer-Encoding: chunked\r\n"
				"\r\n"
				"5\r\n"
				"Hello\r\n"
				"0\r\n",
				false, "", 400);


The test expects hasError() to be true,
but parse() returned false (incomplete),
and since it's waiting for more data, there's no error set.
*/

	// ========================================================================
	// SECTION 4: Special Tests
	// ========================================================================

	std::cout << "\n" << YELLOW << "══════════════════════════════════════════════════════" << RESET << std::endl;
	std::cout << YELLOW << "  SECTION 4: Special Cases" << RESET << std::endl;
	std::cout << YELLOW << "══════════════════════════════════════════════════════" << RESET << std::endl;

	testIncrementalBody();

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

	if (g_failed == 0)
	{
		std::cout << "\n" << GREEN << "🎉 ALL TESTS PASSED! Step 4.3 is complete! 🎉" << RESET << "\n" << std::endl;
		return 0;
	}
	else
	{
		std::cout << "\n" << RED << "❌ Some tests failed. Review the output above." << RESET << "\n" << std::endl;
		return 1;
	}
}
