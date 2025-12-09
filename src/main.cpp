/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:54:34 by anemet            #+#    #+#             */
/*   Updated: 2025/12/09 11:47:13 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*   test_request_parser.cpp - Test program for Step 4.1                    */
/* ************************************************************************** */

#include "Request.hpp"
#include <iostream>
#include <string>

// Color codes for pretty output
#define GREEN "\033[32m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define RESET "\033[0m"

void testRequest(const std::string& testName, const std::string& requestData,
                 bool shouldSucceed, int expectedError = 0)
{
    std::cout << "\n" << YELLOW << "Test: " << testName << RESET << std::endl;
    std::cout << "Request: \"" << requestData << "\"" << std::endl;

    Request req;
    req.parse(requestData);

    if (shouldSucceed) {
        if (!req.hasError()) {
            std::cout << GREEN << "✓ PASS" << RESET << std::endl;
            std::cout << "  Method: " << req.getMethod() << std::endl;
            std::cout << "  URI: " << req.getUri() << std::endl;
            std::cout << "  Path: " << req.getPath() << std::endl;
            std::cout << "  Query: " << req.getQueryString() << std::endl;
            std::cout << "  Version: " << req.getHttpVersion() << std::endl;
        } else {
            std::cout << RED << "✗ FAIL - Got error " << req.getErrorCode() << RESET << std::endl;
        }
    } else {
        if (req.hasError() && req.getErrorCode() == expectedError) {
            std::cout << GREEN << "✓ PASS - Correctly rejected with " << expectedError << RESET << std::endl;
        } else {
            std::cout << RED << "✗ FAIL - Expected error " << expectedError
                     << ", got " << req.getErrorCode() << RESET << std::endl;
        }
    }
}

int main()
{
    std::cout << "========================================" << std::endl;
    std::cout << "  Request Line Parser Tests (Step 4.1)" << std::endl;
    std::cout << "========================================" << std::endl;

    // Valid requests
    testRequest("Simple GET", "GET / HTTP/1.1\r\n", true);
    testRequest("GET with path", "GET /index.html HTTP/1.1\r\n", true);
    testRequest("GET with query", "GET /search?q=test&lang=en HTTP/1.1\r\n", true);
    testRequest("POST request", "POST /api/upload HTTP/1.1\r\n", true);
    testRequest("DELETE request", "DELETE /files/test.txt HTTP/1.1\r\n", true);
    testRequest("HTTP/1.0", "GET /old.html HTTP/1.0\r\n", true);

    // Invalid requests
    testRequest("Missing URI", "GET\r\n", false, 400);
    testRequest("Missing version", "GET /index.html\r\n", false, 400);
    testRequest("Invalid method", "INVALID /index.html HTTP/1.1\r\n", false, 501);
    testRequest("Wrong version", "GET / HTTP/2.0\r\n", false, 505);
    testRequest("Missing slash", "GET index.html HTTP/1.1\r\n", false, 400);
    testRequest("Extra tokens", "GET / HTTP/1.1 EXTRA\r\n", false, 400);

    // Partial request (should wait for more data)
    std::cout << "\n" << YELLOW << "Test: Partial request" << RESET << std::endl;
    Request partial;
    bool complete = partial.parse("GET / HT");  // Incomplete
    if (!complete && !partial.hasError()) {
        std::cout << GREEN << "✓ PASS - Waiting for more data" << RESET << std::endl;

        // Send rest of data
        complete = partial.parse("TP/1.1\r\n");
        if (complete && !partial.hasError()) {
            std::cout << GREEN << "✓ PASS - Completed after second chunk" << RESET << std::endl;
            std::cout << "  Full request: " << partial.getMethod() << " "
                     << partial.getUri() << " " << partial.getHttpVersion() << std::endl;
        } else {
            std::cout << RED << "✗ FAIL" << RESET << std::endl;
        }
    } else {
        std::cout << RED << "✗ FAIL" << RESET << std::endl;
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "  Tests Complete!" << std::endl;
    std::cout << "========================================\n" << std::endl;

    return 0;
}
