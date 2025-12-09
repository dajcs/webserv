/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main_test4.2.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:54:34 by anemet            #+#    #+#             */
/*   Updated: 2025/12/09 20:49:13 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


/* ************************************************************************** */
/*   test_request_step4_2.cpp - Test for HTTP Headers Parser                */
/* ************************************************************************** */

#include "Request.hpp"
#include <iostream>
#include <string>

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
			testPassed = true;
			std::cout << GREEN << "✓ PASS" << RESET << std::endl;
			std::cout << "  Method:  " << req.getMethod() << std::endl;
			std::cout << "  URI:     " << req.getUri() << std::endl;
			std::cout << "  Version: " << req.getHttpVersion() << std::endl;
			std::cout << "  Headers:" << std::endl;

			const std::map<std::string, std::string>& headers = req.getHeaders();
			for (std::map<std::string, std::string>::const_iterator it = headers.begin();
				 it != headers.end(); ++it)
			{
				std::cout << "    " << it->first << ": " << it->second << std::endl;
			}
		}
		else if (!complete)
		{
			std::cout << RED << "✗ FAIL - Parsing incomplete" << RESET << std::endl;
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

void testHeaderAccess()
{
	std::cout << "\n" << YELLOW << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << RESET << std::endl;
	std::cout << BLUE << "Test: Case-Insensitive Header Access" << RESET << std::endl;

	Request req;
	std::string request =
		"GET / HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: application/json\r\n"
		"\r\n";

	req.parse(request);

	bool testPassed = true;

	// Test different case variations
	if (req.getHeader("host") != "localhost")
	{
		std::cout << RED << "  ✗ getHeader(\"host\") failed" << RESET << std::endl;
		testPassed = false;
	}
	if (req.getHeader("Host") != "localhost")
	{
		std::cout << RED << "  ✗ getHeader(\"Host\") failed" << RESET << std::endl;
		testPassed = false;
	}
	if (req.getHeader("HOST") != "localhost")
	{
		std::cout << RED << "  ✗ getHeader(\"HOST\") failed" << RESET << std::endl;
		testPassed = false;
	}
	if (req.getHeader("content-type") != "application/json")
	{
		std::cout << RED << "  ✗ getHeader(\"content-type\") failed" << RESET << std::endl;
		testPassed = false;
	}
	if (req.getHeader("Content-Type") != "application/json")
	{
		std::cout << RED << "  ✗ getHeader(\"Content-Type\") failed" << RESET << std::endl;
		testPassed = false;
	}

	// Test non-existent header
	if (!req.getHeader("NonExistent").empty())
	{
		std::cout << RED << "  ✗ Non-existent header should return empty string" << RESET << std::endl;
		testPassed = false;
	}

	if (testPassed)
	{
		std::cout << GREEN << "✓ PASS - All case variations work correctly" << RESET << std::endl;
		g_passed++;
	}
	else
	{
		std::cout << RED << "✗ FAIL - Case-insensitive access broken" << RESET << std::endl;
		g_failed++;
	}
}

int main()
{
	std::cout << "\n" << BLUE;
	std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
	std::cout << "║                                                           ║\n";
	std::cout << "║   WEBSERV - Step 4.2: HTTP Headers Parser Tests           ║\n";
	std::cout << "║                                                           ║\n";
	std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
	std::cout << RESET << std::endl;

	// ========================================================================
	// VALID REQUESTS WITH HEADERS
	// ========================================================================

	std::cout << "\n" << YELLOW << "══════════════════════════════════════════════════════" << RESET << std::endl;
	std::cout << YELLOW << "  SECTION 1: Valid HTTP Requests with Headers" << RESET << std::endl;
	std::cout << YELLOW << "══════════════════════════════════════════════════════" << RESET << std::endl;

	testRequest("Basic GET with Host header",
				"GET / HTTP/1.1\r\n"
				"Host: localhost\r\n"
				"\r\n",
				true);

	testRequest("GET with multiple headers",
				"GET /index.html HTTP/1.1\r\n"
				"Host: localhost:8080\r\n"
				"User-Agent: TestClient/1.0\r\n"
				"Accept: text/html\r\n"
				"Accept-Language: en-US\r\n"
				"\r\n",
				true);

	testRequest("POST with Content-Length",
				"POST /api/data HTTP/1.1\r\n"
				"Host: localhost\r\n"
				"Content-Type: application/json\r\n"
				"Content-Length: 27\r\n"
				"\r\n",
				true);

	testRequest("Headers with extra whitespace",
				"GET / HTTP/1.1\r\n"
				"Host:    localhost   \r\n"
				"User-Agent:   Mozilla/5.0   \r\n"
				"\r\n",
				true);

	testRequest("Header with colon in value",
				"GET / HTTP/1.1\r\n"
				"Host: localhost\r\n"
				"Cookie: session=abc:def:123\r\n"
				"\r\n",
				true);

	testRequest("Mixed case headers",
				"GET / HTTP/1.1\r\n"
				"HoSt: localhost\r\n"
				"CoNtEnT-TyPe: text/html\r\n"
				"\r\n",
				true);

	testRequest("Connection header",
				"GET / HTTP/1.1\r\n"
				"Host: localhost\r\n"
				"Connection: keep-alive\r\n"
				"\r\n",
				true);

	testRequest("HTTP/1.0 without Host (allowed)",
				"GET / HTTP/1.0\r\n"
				"User-Agent: OldBrowser/1.0\r\n"
				"\r\n",
				true);

	// ========================================================================
	// INVALID REQUESTS
	// ========================================================================

	std::cout << "\n" << YELLOW << "══════════════════════════════════════════════════════" << RESET << std::endl;
	std::cout << YELLOW << "  SECTION 2: Invalid Headers (Error Handling)" << RESET << std::endl;
	std::cout << YELLOW << "══════════════════════════════════════════════════════" << RESET << std::endl;

	testRequest("HTTP/1.1 missing Host header",
				"GET / HTTP/1.1\r\n"
				"User-Agent: TestClient/1.0\r\n"
				"\r\n",
				false, 400);

	testRequest("Header without colon",
				"GET / HTTP/1.1\r\n"
				"Host localhost\r\n"
				"\r\n",
				false, 400);

	testRequest("Header with empty name",
				"GET / HTTP/1.1\r\n"
				"Host: localhost\r\n"
				": value\r\n"
				"\r\n",
				false, 400);

	testRequest("Header with space in name",
				"GET / HTTP/1.1\r\n"
				"Host Name: localhost\r\n"
				"\r\n",
				false, 400);

	testRequest("Content-Length too large",
				"POST /upload HTTP/1.1\r\n"
				"Host: localhost\r\n"
				"Content-Length: 99999999999\r\n"
				"\r\n",
				false, 413);

	// ========================================================================
	// SPECIAL TESTS
	// ========================================================================

	std::cout << "\n" << YELLOW << "══════════════════════════════════════════════════════" << RESET << std::endl;
	std::cout << YELLOW << "  SECTION 3: Special Cases" << RESET << std::endl;
	std::cout << YELLOW << "══════════════════════════════════════════════════════" << RESET << std::endl;

	testHeaderAccess();

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
	std::cout << GREEN << "  Passed:	   " << g_passed << RESET << std::endl;
	std::cout << RED << "  Failed:	   " << g_failed << RESET << std::endl;

	if (g_failed == 0)
	{
		std::cout << "\n" << GREEN << "🎉 ALL TESTS PASSED! Step 4.2 is complete! 🎉" << RESET << "\n" << std::endl;
		return 0;
	}
	else
	{
		std::cout << "\n" << RED << "❌ Some tests failed. Review the output above." << RESET << "\n" << std::endl;
		return 1;
	}
}
