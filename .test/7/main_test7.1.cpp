/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main_test7.1.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:54:34 by anemet            #+#    #+#             */
/*   Updated: 2025/12/12 10:39:06 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*
    ./webserv config/delete_test.conf
*/

/*
    ============================================================================
    DELETE Method Unit Tests
    ============================================================================

    These tests validate the DELETE method implementation at the Router level,
    without requiring network connectivity.

    Test Strategy:
    --------------
    We test the Router directly by:
    1. Creating a Config object (from file or programmatically)
    2. Creating mock Request objects that simulate HTTP requests
    3. Calling Router::route() and checking the Response

    This approach tests:
    - Path resolution and security
    - File existence checking
    - Permission handling
    - Response generation

    Running the tests:
    ------------------
    Compile: g++ -Wall -Wextra -Werror -std=c++98 -I../inc \
             test_delete.cpp Config.cpp Request.cpp Response.cpp Router.cpp \
             Utils.cpp -o test_delete

    Run: ./test_delete

    Before running, create test files:
      mkdir -p www/uploads
      echo "delete me" > www/uploads/test.txt
      echo "protected" > www/uploads/protected.txt
      chmod 444 www/uploads/protected.txt  # read-only
*/

#include "Config.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Router.hpp"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

/*
    Test utility: Create a test file with given content
*/
bool createTestFile(const std::string& path, const std::string& content)
{
    std::ofstream file(path.c_str());
    if (!file)
    {
        std::cerr << "Failed to create test file: " << path << std::endl;
        return false;
    }
    file << content;
    file.close();
    return true;
}

/*
    Test utility: Check if a file exists
*/
bool fileExists(const std::string& path)
{
    struct stat st;
    return (stat(path.c_str(), &st) == 0);
}

/*
    Test utility: Create a directory if it doesn't exist
*/
bool createDirectory(const std::string& path)
{
    struct stat st;
    if (stat(path.c_str(), &st) == 0)
    {
        return S_ISDIR(st.st_mode);
    }
    // Use mkdir with rwxr-xr-x permissions
    return (mkdir(path.c_str(), 0755) == 0);
}

/*
    Helper: Create a Request object for DELETE method

    In a real scenario, the Request would be parsed from raw HTTP data.
    For testing, we manually construct the request with the needed fields.
*/
Request createDeleteRequest(const std::string& path)
{
    /*
        Simulating an HTTP DELETE request:

        DELETE /uploads/test.txt HTTP/1.1\r\n
        Host: localhost:8080\r\n
        \r\n
    */
    Request req;

    // Build raw HTTP request string and parse it
    std::string rawRequest = "DELETE " + path + " HTTP/1.1\r\n";
    rawRequest += "Host: localhost:8080\r\n";
    rawRequest += "\r\n";

    req.parse(rawRequest);
    return req;
}

/*
    Helper: Print test result with colored output (ANSI codes)
*/
void printResult(const std::string& testName, bool passed)
{
    if (passed)
    {
        std::cout << "\033[32m[PASS]\033[0m " << testName << std::endl;
    }
    else
    {
        std::cout << "\033[31m[FAIL]\033[0m " << testName << std::endl;
    }
}

/*
    ============================================================================
    Test Cases
    ============================================================================
*/

/*
    Test 1: Successful file deletion

    Expected: 204 No Content

    Scenario:
    1. Create a file in the upload directory
    2. Send DELETE request for that file
    3. Verify response is 204
    4. Verify file no longer exists
*/
bool testSuccessfulDelete(Router& router)
{
    std::string testFile = "www/uploads/delete_test.txt";

    // Setup: Create test file
    if (!createTestFile(testFile, "This file will be deleted"))
    {
        std::cerr << "Setup failed: couldn't create test file" << std::endl;
        return false;
    }

    // Verify file exists before deletion
    if (!fileExists(testFile))
    {
        std::cerr << "Setup failed: test file doesn't exist" << std::endl;
        return false;
    }

    // Create DELETE request
    Request request = createDeleteRequest("/uploads/delete_test.txt");

    // Route the request (port 8080 matches our config)
    Response response = router.route(request, 8080);

    // Check response status
    bool statusOk = (response.getStatusCode() == 204);

    // Check file is deleted
    bool fileDeleted = !fileExists(testFile);

    // Both conditions must be true
    return (statusOk && fileDeleted);
}

/*
    Test 2: Delete non-existent file

    Expected: 404 Not Found

    Scenario:
    1. Ensure file doesn't exist
    2. Send DELETE request for that file
    3. Verify response is 404
*/
bool testDeleteNonExistent(Router& router)
{
    std::string testFile = "www/uploads/nonexistent_file.txt";

    // Ensure file doesn't exist
    unlink(testFile.c_str());

    // Create DELETE request
    Request request = createDeleteRequest("/uploads/nonexistent_file.txt");

    // Route the request
    Response response = router.route(request, 8080);

    // Check response status
    return (response.getStatusCode() == 404);
}

/*
    Test 3: Delete a directory (should fail)

    Expected: 409 Conflict

    Scenario:
    1. Create a directory
    2. Send DELETE request for that directory
    3. Verify response is 409 (conflict - can't delete directories)
*/
bool testDeleteDirectory(Router& router)
{
    std::string testDir = "www/uploads/test_directory";

    // Setup: Create test directory
    createDirectory(testDir);

    // Create DELETE request for the directory
    Request request = createDeleteRequest("/uploads/test_directory");

    // Route the request
    Response response = router.route(request, 8080);

    // Cleanup
    rmdir(testDir.c_str());

    // Check response status
    return (response.getStatusCode() == 409);
}

/*
    Test 4: Delete with directory traversal attempt (security test)

    Expected: 403 Forbidden or 404 Not Found

    Scenario:
    1. Send DELETE request with "../" in path
    2. Verify server rejects the request

    This tests protection against path traversal attacks.
*/
bool testDeleteDirectoryTraversal(Router& router)
{
    // Try to delete a file outside the uploads directory
    // The path sanitization should prevent this
    Request request = createDeleteRequest("/uploads/../../../etc/passwd");

    // Route the request
    Response response = router.route(request, 8080);

    // Should get either 403 (Forbidden) or 404 (path sanitized to nothing)
    int status = response.getStatusCode();
    return (status == 403 || status == 404);
}

/*
    Test 5: DELETE method not allowed

    Expected: 405 Method Not Allowed

    Scenario:
    1. Send DELETE request to a location that doesn't allow DELETE
    2. Verify response is 405

    This tests the method allowlist from configuration.
*/
bool testDeleteMethodNotAllowed(Router& router)
{
    // Create DELETE request for root path (usually only allows GET)
    Request request = createDeleteRequest("/index.html");

    // Route the request
    Response response = router.route(request, 8080);

    // Should get 405 if DELETE is not allowed for /
    // Note: This depends on your config having DELETE disabled for /
    return (response.getStatusCode() == 405);
}

/*
    Test 6: Verify 204 response has no body

    Expected: Response body is empty

    HTTP/1.1 specifies that 204 responses MUST NOT have a body.
*/
bool testNoContentResponse(Router& router)
{
    std::string testFile = "www/uploads/no_content_test.txt";

    // Setup: Create test file
    createTestFile(testFile, "test content");

    // Create DELETE request
    Request request = createDeleteRequest("/uploads/no_content_test.txt");

    // Route the request
    Response response = router.route(request, 8080);

    // Check that body is empty for 204 response
    if (response.getStatusCode() == 204)
    {
        return response.getBody().empty();
    }
    return false;
}

/*
    Test 7: Idempotency - Second DELETE returns 404

    Expected: First DELETE: 204, Second DELETE: 404

    DELETE is idempotent: multiple identical requests should have
    the same effect as a single request. After the first DELETE
    removes the file, subsequent DELETEs should return 404.
*/
bool testDeleteIdempotency(Router& router)
{
    std::string testFile = "www/uploads/idempotent_test.txt";

    // Setup: Create test file
    createTestFile(testFile, "test for idempotency");

    // Create DELETE request
    Request request = createDeleteRequest("/uploads/idempotent_test.txt");

    // First DELETE - should succeed
    Response response1 = router.route(request, 8080);
    if (response1.getStatusCode() != 204)
    {
        return false;
    }

    // Second DELETE - should return 404 (file already gone)
    Response response2 = router.route(request, 8080);
    return (response2.getStatusCode() == 404);
}


/*
    ============================================================================
    Main Test Runner
    ============================================================================
*/
int main(int argc, char** argv)
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "  DELETE Method Unit Tests" << std::endl;
    std::cout << "========================================\n" << std::endl;

    // Setup: Create upload directory
    createDirectory("www");
    createDirectory("www/uploads");

    // Load configuration
    std::string configPath = "config/default.conf";
    if (argc > 1)
    {
        configPath = argv[1];
    }

    std::cout << "Using config: " << configPath << std::endl;

    try
    {
        Config config(configPath);
        Router router(config);

        int passed = 0;
        int total = 0;

        // Run all tests
        std::cout << "\n--- Running Tests ---\n" << std::endl;

        total++;
        if (testSuccessfulDelete(router))
        {
            printResult("1. Successful file deletion", true);
            passed++;
        }
        else
        {
            printResult("1. Successful file deletion", false);
        }

        total++;
        if (testDeleteNonExistent(router))
        {
            printResult("2. Delete non-existent file (404)", true);
            passed++;
        }
        else
        {
            printResult("2. Delete non-existent file (404)", false);
        }

        total++;
        if (testDeleteDirectory(router))
        {
            printResult("3. Delete directory (409)", true);
            passed++;
        }
        else
        {
            printResult("3. Delete directory (409)", false);
        }

        total++;
        if (testDeleteDirectoryTraversal(router))
        {
            printResult("4. Directory traversal prevention", true);
            passed++;
        }
        else
        {
            printResult("4. Directory traversal prevention", false);
        }

        total++;
        if (testDeleteMethodNotAllowed(router))
        {
            printResult("5. DELETE method not allowed (405)", true);
            passed++;
        }
        else
        {
            printResult("5. DELETE method not allowed (405)", false);
        }

        total++;
        if (testNoContentResponse(router))
        {
            printResult("6. 204 response has no body", true);
            passed++;
        }
        else
        {
            printResult("6. 204 response has no body", false);
        }

        total++;
        if (testDeleteIdempotency(router))
        {
            printResult("7. DELETE idempotency", true);
            passed++;
        }
        else
        {
            printResult("7. DELETE idempotency", false);
        }

        // Summary
        std::cout << "\n========================================" << std::endl;
        std::cout << "  Results: " << passed << "/" << total << " tests passed" << std::endl;
        std::cout << "========================================\n" << std::endl;

        return (passed == total) ? 0 : 1;
    }
    catch (const ConfigException& e)
    {
        std::cerr << "Config error: " << e.what() << std::endl;
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
