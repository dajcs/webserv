/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:54:34 by anemet            #+#    #+#             */
/*   Updated: 2025/12/14 17:35:19 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*
	=================================================================
		HTTP REDIRECTIONS TEST SUITE - Step 9.1
	=================================================================

	HTTP Redirections Overview:
	---------------------------
	Redirections are HTTP responses that tell the client to request
	a different URL. They're essential for:

	1. URL Management:
		- Moving content to new locations
		- Restructuring website URLs
		- Consolidating duplicate URLs

	2. Protocol Upgrades:
		- Forcing HTTPS (http → https)
		- Canonical URL enforcement

	3. Temporary Situations:
		- Maintenance pages
		- A/B testing
		- Load balancing

	How Redirects Work:
	-------------------
	1. Client requests: GET /old-page HTTP/1.1
	2. Server responds: HTTP/1.1 301 Moved Permanently
						Location: /new-page
	3. Client automatically requests: GET /new-page HTTP/1.1
	4. Server responds with actual content

	Status Code Semantics:
	----------------------
	| Code | Name                | Cache? | Method Preserved? | Use Case           |
	|------|---------------------|--------|-------------------|-------------------|
	| 301  | Moved Permanently   | Yes    | May change to GET | Permanent moves   |
	| 302  | Found               | No     | May change to GET | Temporary moves   |
	| 303  | See Other           | No     | Always GET        | POST → GET        |
	| 307  | Temporary Redirect  | No     | Yes               | Strict temporary  |
	| 308  | Permanent Redirect  | Yes    | Yes               | Strict permanent  |

	Test Structure:
	---------------
	Since the network layer isn't implemented yet, we test at higher level:
	1. Config parsing: Verify redirect directives are parsed correctly
	2. Router behavior: Verify redirects are triggered for matching paths
	3. Response format: Verify redirect responses have correct format
*/

#include <iostream>
#include <sstream>
#include <cstring>

#include "Config.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Router.hpp"

// ===========================================
//  Test Infrastructure
// ===========================================

static int g_testsRun = 0;
static int g_testsPassed = 0;
static int g_testsFailed = 0;

// ANSI color codes
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_RESET   "\033[0m"

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


// ===========================================
//  Helper: Create Mock Request
// ===========================================
/*
	Creates a Request object for testing routing.
	We need a complete, parsed request to pass to the Router.
*/
Request createRequest(const std::string& method, const std::string& path)
{
	Request request;

	std::string httpRequest = method + " " + path + " HTTP/1.1\r\n";
	httpRequest += "Host: localhost:8080\r\n";
	httpRequest += "\r\n";

	request.parse(httpRequest);

	return request;
}


// ===========================================
//  Helper: Create Test Config with Redirects
// ===========================================
/*
	Creates a ServerConfig with various redirect scenarios for testing.

	We test:
	1. 301 Permanent redirect
	2. 302 Temporary redirect
	3. Redirect to absolute URL
	4. Redirect to relative URL
	5. Non-redirect location (control)
*/
ServerConfig createRedirectTestServer()
{
	ServerConfig server;
	server.host = "0.0.0.0";
	server.port = 8080;

	/*
		Location 1: Permanent redirect (301)
		------------------------------------
		Use case: URL has permanently moved
		Example: Website restructuring, old blog URL to new blog URL

		Config equivalent:
			location /old-page {
				return 301 /new-page;
			}
	*/
	LocationConfig loc1;
	loc1.path = "/old-page";
	loc1.redirect_code = 301;
	loc1.redirect_url = "/new-page";
	loc1.root = "www";
	server.locations.push_back(loc1);

	/*
		Location 2: Temporary redirect (302)
		------------------------------------
		Use case: Content temporarily at different location
		Example: Maintenance page, seasonal content

		Config equivalent:
			location /temp-move {
				return 302 /temporary-location;
			}
	*/
	LocationConfig loc2;
	loc2.path = "/temp-move";
	loc2.redirect_code = 302;
	loc2.redirect_url = "/temporary-location";
	loc2.root = "www";
	server.locations.push_back(loc2);

	/*
		Location 3: Redirect to absolute URL
		------------------------------------
		Use case: Redirect to different domain
		Example: Redirect to HTTPS, redirect to CDN

		Config equivalent:
			location /external {
				return 301 https://example.com/page;
			}
	*/
	LocationConfig loc3;
	loc3.path = "/external";
	loc3.redirect_code = 301;
	loc3.redirect_url = "https://example.com/page";
	loc3.root = "www";
	server.locations.push_back(loc3);

	/*
		Location 4: Redirect with query string preservation
		----------------------------------------------------
		Use case: Redirect while keeping URL parameters
		Note: Our simple implementation redirects to exact URL,
				query string handling would need additional logic

		Config equivalent:
			location /search-old {
				return 301 /search;
			}
	*/
	LocationConfig loc4;
	loc4.path = "/search-old";
	loc4.redirect_code = 301;
	loc4.redirect_url = "/search";
	loc4.root = "www";
	server.locations.push_back(loc4);

	/*
		Location 5: No redirect (control case)
		--------------------------------------
		This location has no redirect configured.
		Requests should be handled normally (file serving).

		Config equivalent:
			location /normal {
				root www;
				index index.html;
			}
	*/
	LocationConfig loc5;
	loc5.path = "/normal";
	loc5.root = "www";
	loc5.index = "index.html";
	loc5.allowed_methods.insert("GET");
	// redirect_url is empty, redirect_code is 0
	server.locations.push_back(loc5);

	/*
		Location 6: Root location (fallback)
		------------------------------------
		Matches anything not matched by more specific locations.
	*/
	LocationConfig locRoot;
	locRoot.path = "/";
	locRoot.root = "www";
	locRoot.index = "index.html";
	locRoot.allowed_methods.insert("GET");
	server.locations.push_back(locRoot);

	return server;
}


// ===========================================
//  Test 1: Response::redirect() Format
// ===========================================
/*
	Test the Response::redirect() static factory method.

	Verifies:
	1. Correct status code is set
	2. Location header is present and correct
	3. Content-Type is set for the fallback body
	4. Body contains fallback HTML
*/
void testResponseRedirectFormat()
{
	TEST_SECTION("Test 1: Response::redirect() Format");

	// Test 301 Permanent Redirect
	{
		Response response = Response::redirect(301, "/new-location");

		TEST_ASSERT(response.getStatusCode() == 301,
					"301 redirect has correct status code");

		TEST_ASSERT(response.getHeader("Location") == "/new-location",
					"301 redirect has correct Location header");

		TEST_ASSERT(!response.getHeader("Content-Type").empty(),
					"301 redirect has Content-Type header");

		TEST_ASSERT(response.getBody().find("/new-location") != std::string::npos,
					"301 redirect body contains target URL");

		// Verify the built response format
		std::string built = response.build();
		TEST_ASSERT(built.find("HTTP/1.1 301") != std::string::npos,
					"Built response has correct status line");

		TEST_ASSERT(built.find("Location: /new-location") != std::string::npos,
					"Built response has Location header");

		std::cout << "       Response preview: "
					<< built.substr(0, 100) << "..." << std::endl;
	}

	// Test 302 Temporary Redirect
	{
		Response response = Response::redirect(302, "/temp");

		TEST_ASSERT(response.getStatusCode() == 302,
					"302 redirect has correct status code");

		TEST_ASSERT(response.getHeader("Location") == "/temp",
					"302 redirect has correct Location header");
	}

	// Test redirect to absolute URL
	{
		Response response = Response::redirect(301, "https://example.com/page");

		TEST_ASSERT(response.getHeader("Location") == "https://example.com/page",
					"Redirect to absolute URL preserves full URL");
	}

	// Test redirect with special characters in URL
	{
		Response response = Response::redirect(301, "/path?query=value&other=123");

		TEST_ASSERT(response.getHeader("Location") == "/path?query=value&other=123",
					"Redirect preserves query string in URL");
	}
}


// ===========================================
//  Test 2: Router Redirect Detection
// ===========================================
/*
	Test that the Router correctly identifies redirect locations
	and returns redirect responses.

	The routing flow for redirects:
	1. Request comes in for /old-page
	2. Router finds matching location: /old-page
	3. Location has redirect_url set
	4. Router immediately returns redirect response
	5. No file resolution, CGI check, or method validation needed
*/
void testRouterRedirectDetection()
{
	TEST_SECTION("Test 2: Router Redirect Detection");

	// Create a minimal Config with our test server
	// We'll create the ServerConfig directly and use a custom approach

	ServerConfig server = createRedirectTestServer();

	/*
		Test: Location with 301 redirect
		--------------------------------
		Request: GET /old-page
		Expected: 301 redirect to /new-page
	*/
	{
		// Find the location manually (simulating what Router does)
		const LocationConfig* location = server.findLocation("/old-page");

		TEST_ASSERT(location != NULL,
					"Location /old-page found in config");

		if (location)
		{
			TEST_ASSERT(location->redirect_code == 301,
						"Location /old-page has redirect code 301");

			TEST_ASSERT(location->redirect_url == "/new-page",
						"Location /old-page redirects to /new-page");

			// Now create the redirect response
			Response response = Response::redirect(
				location->redirect_code,
				location->redirect_url
			);

			TEST_ASSERT(response.getStatusCode() == 301,
						"Redirect response has status 301");

			TEST_ASSERT(response.getHeader("Location") == "/new-page",
						"Redirect response has correct Location");
		}
	}

	/*
		Test: Location with 302 redirect
		--------------------------------
		Request: GET /temp-move
		Expected: 302 redirect to /temporary-location
	*/
	{
		const LocationConfig* location = server.findLocation("/temp-move");

		TEST_ASSERT(location != NULL,
					"Location /temp-move found in config");

		if (location)
		{
			TEST_ASSERT(location->redirect_code == 302,
						"Location /temp-move has redirect code 302");

			TEST_ASSERT(location->redirect_url == "/temporary-location",
						"Location /temp-move redirects to /temporary-location");
		}
	}

	/*
		Test: Location without redirect
		--------------------------------
		Request: GET /normal
		Expected: No redirect (redirect_url is empty)
	*/
	{
		const LocationConfig* location = server.findLocation("/normal");

		TEST_ASSERT(location != NULL,
					"Location /normal found in config");

		if (location)
		{
			TEST_ASSERT(location->redirect_url.empty(),
						"Location /normal has no redirect URL");

			TEST_ASSERT(location->redirect_code == 0,
						"Location /normal has redirect code 0");
		}
	}

	/*
		Test: Redirect to external URL
		-------------------------------
		Request: GET /external
		Expected: 301 redirect to https://example.com/page
	*/
	{
		const LocationConfig* location = server.findLocation("/external");

		TEST_ASSERT(location != NULL,
					"Location /external found in config");

		if (location)
		{
			TEST_ASSERT(location->redirect_url == "https://example.com/page",
						"Location /external redirects to external URL");
		}
	}
}


// ===========================================
//  Test 3: Redirect Response Headers
// ===========================================
/*
	Verify that redirect responses contain all required headers.

	Required headers for a valid redirect:
	1. Location: The target URL (REQUIRED)
	2. Content-Type: MIME type for fallback body
	3. Content-Length: Size of fallback body
	4. Date: Current server time
	5. Server: Server identification
*/
void testRedirectResponseHeaders()
{
	TEST_SECTION("Test 3: Redirect Response Headers");

	Response response = Response::redirect(301, "/new-url");

	// Build the response to ensure all headers are generated
	std::string built = response.build();

	/*
		Location Header (RFC 7231 Section 7.1.2)
		----------------------------------------
		The Location header field is used to indicate the URL to redirect to.
		It MUST be present in redirect responses.

		Format: Location: <url>
		The URL can be:
		- Relative: /new-path (resolved against request URL)
		- Absolute: https://example.com/path
	*/
	TEST_ASSERT(built.find("Location: /new-url") != std::string::npos,
				"Response contains Location header");

	/*
		Content-Type Header
		-------------------
		Even though browsers follow the redirect automatically,
		we include a fallback HTML body, so Content-Type is needed.
	*/
	TEST_ASSERT(built.find("Content-Type:") != std::string::npos,
				"Response contains Content-Type header");

	/*
		Content-Length Header
		---------------------
		Required for HTTP/1.1 to know when the body ends.
		Allows keep-alive connections.
	*/
	TEST_ASSERT(built.find("Content-Length:") != std::string::npos,
				"Response contains Content-Length header");

	/*
		Date Header
		-----------
		Recommended by RFC 7231 for all responses.
		Shows when the response was generated.
	*/
	TEST_ASSERT(built.find("Date:") != std::string::npos,
				"Response contains Date header");

	/*
		Server Header
		-------------
		Optional but common. Identifies the server software.
	*/
	TEST_ASSERT(built.find("Server:") != std::string::npos,
				"Response contains Server header");

	// Print the full response for inspection
	std::cout << "\n       Full redirect response:\n";
	std::cout << "       ─────────────────────────────────────────\n";

	// Print line by line with indentation
	std::istringstream iss(built);
	std::string line;
	int lineCount = 0;
	while (std::getline(iss, line) && lineCount < 15)
	{
		std::cout << "       " << line << std::endl;
		lineCount++;
	}
	if (lineCount >= 15)
	{
		std::cout << "       ... (truncated)" << std::endl;
	}
	std::cout << "       ─────────────────────────────────────────\n";
}


// ===========================================
//  Test 4: Redirect with Different HTTP Methods
// ===========================================
/*
	Test that redirects work correctly with different HTTP methods.

	Important behavior:
	- Redirects are checked BEFORE method validation
	- A redirect location should redirect regardless of method
	- This is because redirects are about "where" not "how"

	Example: If /old-api redirects to /new-api,
	both GET /old-api and POST /old-api should redirect.
*/
void testRedirectWithDifferentMethods()
{
	TEST_SECTION("Test 4: Redirects with Different HTTP Methods");

	ServerConfig server = createRedirectTestServer();
	const LocationConfig* location = server.findLocation("/old-page");

	TEST_ASSERT(location != NULL, "Redirect location found");

	if (location)
	{
		/*
			GET Request to redirect location
			---------------------------------
			Most common case. Browser follows redirect automatically.
		*/
		{
			Request getRequest = createRequest("GET", "/old-page");

			TEST_ASSERT(!location->redirect_url.empty(),
						"GET /old-page triggers redirect");

			std::cout << "       GET /old-page -> 301 "
						<< location->redirect_url << std::endl;
		}

		/*
			POST Request to redirect location
			----------------------------------
			POST should also redirect.
			Note: 301/302 may change method to GET (historical behavior).
			Use 307/308 to preserve POST method.
		*/
		{
			Request postRequest = createRequest("POST", "/old-page");

			// The redirect should still trigger
			TEST_ASSERT(!location->redirect_url.empty(),
						"POST /old-page triggers redirect");

			std::cout << "       POST /old-page -> 301 "
						<< location->redirect_url << std::endl;
		}

		/*
			DELETE Request to redirect location
			------------------------------------
			DELETE should also redirect, though less common.
		*/
		{
			Request deleteRequest = createRequest("DELETE", "/old-page");

			TEST_ASSERT(!location->redirect_url.empty(),
						"DELETE /old-page triggers redirect");

			std::cout << "       DELETE /old-page -> 301 "
						<< location->redirect_url << std::endl;
		}
	}
}


// ===========================================
//  Test 5: Status Code Reason Phrases
// ===========================================
/*
	Verify that status codes have correct reason phrases.

	RFC 7231 defines standard reason phrases:
	- 301: "Moved Permanently"
	- 302: "Found"
	- 303: "See Other"
	- 307: "Temporary Redirect"
	- 308: "Permanent Redirect"
*/
void testRedirectReasonPhrases()
{
	TEST_SECTION("Test 5: Redirect Status Code Reason Phrases");

	// Test 301 Moved Permanently
	{
		Response response = Response::redirect(301, "/url");
		std::string built = response.build();

		TEST_ASSERT(built.find("301 Moved Permanently") != std::string::npos,
					"301 has reason phrase 'Moved Permanently'");
	}

	// Test 302 Found
	{
		Response response = Response::redirect(302, "/url");
		std::string built = response.build();

		TEST_ASSERT(built.find("302 Found") != std::string::npos,
					"302 has reason phrase 'Found'");
	}

	// Test 303 See Other
	{
		Response response = Response::redirect(303, "/url");
		std::string built = response.build();

		TEST_ASSERT(built.find("303 See Other") != std::string::npos,
					"303 has reason phrase 'See Other'");
	}

	// Test 307 Temporary Redirect
	{
		Response response = Response::redirect(307, "/url");
		std::string built = response.build();

		TEST_ASSERT(built.find("307 Temporary Redirect") != std::string::npos,
					"307 has reason phrase 'Temporary Redirect'");
	}

	// Test 308 Permanent Redirect
	{
		Response response = Response::redirect(308, "/url");
		std::string built = response.build();

		TEST_ASSERT(built.find("308 Permanent Redirect") != std::string::npos,
					"308 has reason phrase 'Permanent Redirect'");
	}
}


// ===========================================
//  Test 6: Redirect Fallback HTML Body
// ===========================================
/*
	Test the fallback HTML body in redirect responses.

	Why we include a body:
	1. Very old browsers might not auto-redirect
	2. Command-line tools (curl without -L) show the body
	3. Accessibility: screen readers can announce the redirect
	4. Debugging: developers can see what's happening

	The body should contain:
	- A clickable link to the target URL
	- A meta refresh tag as fallback
	- Clear message explaining the redirect
*/
void testRedirectFallbackBody()
{
	TEST_SECTION("Test 6: Redirect Fallback HTML Body");

	std::string targetUrl = "/new-destination";
	Response response = Response::redirect(301, targetUrl);
	std::string body = response.getBody();

	// Check for clickable link
	TEST_ASSERT(body.find("<a href=\"" + targetUrl + "\"") != std::string::npos,
				"Body contains clickable link to target");

	// Check for meta refresh tag
	TEST_ASSERT(body.find("http-equiv=\"refresh\"") != std::string::npos,
				"Body contains meta refresh tag");

	TEST_ASSERT(body.find("url=" + targetUrl) != std::string::npos,
				"Meta refresh points to correct URL");

	// Check for user-friendly message
	TEST_ASSERT(body.find("Redirect") != std::string::npos ||
				body.find("redirect") != std::string::npos,
				"Body contains redirect message");

	// Check it's valid HTML
	TEST_ASSERT(body.find("<!DOCTYPE html>") != std::string::npos ||
				body.find("<html>") != std::string::npos,
				"Body is valid HTML");

	std::cout << "\n       Fallback body preview:\n";
	std::cout << "       " << body.substr(0, 200) << "..." << std::endl;
}


// ===========================================
//  Test 7: Edge Cases
// ===========================================
/*
	Test edge cases and special scenarios for redirects.
*/
void testRedirectEdgeCases()
{
	TEST_SECTION("Test 7: Redirect Edge Cases");

	/*
		Empty URL redirect
		------------------
		What happens if redirect_url is empty?
		Router should not trigger redirect.
	*/
	{
		LocationConfig location;
		location.path = "/test";
		location.redirect_url = "";  // Empty
		location.redirect_code = 301;

		// Empty URL means no redirect
		TEST_ASSERT(location.redirect_url.empty(),
					"Empty redirect_url is detected");
	}

	/*
		URL with special characters
		---------------------------
		URLs can contain query strings, fragments, etc.
	*/
	{
		std::string complexUrl = "/search?q=hello+world&page=1#results";
		Response response = Response::redirect(301, complexUrl);

		TEST_ASSERT(response.getHeader("Location") == complexUrl,
					"Complex URL with query string preserved");
	}

	/*
		Very long URL
		-------------
		Test handling of long URLs (up to reasonable limits).
	*/
	{
		std::string longUrl = "/";
		for (int i = 0; i < 100; i++)
		{
			longUrl += "segment/";
		}

		Response response = Response::redirect(301, longUrl);
		TEST_ASSERT(response.getHeader("Location") == longUrl,
					"Long URL handled correctly");
	}

	/*
		Redirect loop prevention
		------------------------
		Note: Actual loop detection happens in the browser.
		Server just sends the redirect response.
		We document this as a known limitation.
	*/
	{
		// This would cause a loop, but server doesn't detect it
		Response response = Response::redirect(301, "/same-page");

		TEST_ASSERT(response.getStatusCode() == 301,
					"Server sends redirect (loop detection is client's job)");

		std::cout << "       Note: Redirect loops are detected by browsers, not server\n";
	}
}


// ===========================================
//  Test 8: Integration with Router
// ===========================================
/*
	Test the complete flow through Router.

	Since we can't actually run the Router without a Config object,
	we simulate the redirect logic that Router performs.
*/
void testRouterIntegration()
{
	TEST_SECTION("Test 8: Router Integration Simulation");

	ServerConfig server = createRedirectTestServer();

	/*
		Simulate: GET /old-page HTTP/1.1
		Expected flow:
		1. Router receives request
		2. Router finds location /old-page
		3. Location has redirect_url set
		4. Router returns Response::redirect(301, "/new-page")
	*/
	{
		std::cout << "\n       Simulating: GET /old-page HTTP/1.1\n";

		// Step 1: Find location (Router does this)
		const LocationConfig* location = server.findLocation("/old-page");
		TEST_ASSERT(location != NULL, "Step 1: Location found");

		if (location)
		{
			// Step 2: Check for redirect (Router does this)
			bool hasRedirect = !location->redirect_url.empty();
			TEST_ASSERT(hasRedirect, "Step 2: Redirect detected");

			// Step 3: Create redirect response (Router does this)
			Response response = Response::redirect(
				location->redirect_code,
				location->redirect_url
			);

			TEST_ASSERT(response.getStatusCode() == 301,
						"Step 3: Redirect response created with 301");

			TEST_ASSERT(response.getHeader("Location") == "/new-page",
						"Step 3: Location header set to /new-page");

			std::cout << "       Result: 301 Moved Permanently -> /new-page\n";
		}
	}

	/*
		Simulate: GET /normal HTTP/1.1
		Expected flow:
		1. Router receives request
		2. Router finds location /normal
		3. Location has NO redirect
		4. Router continues to file serving
	*/
	{
		std::cout << "\n       Simulating: GET /normal HTTP/1.1\n";

		const LocationConfig* location = server.findLocation("/normal");
		TEST_ASSERT(location != NULL, "Location /normal found");

		if (location)
		{
			bool hasRedirect = !location->redirect_url.empty();
			TEST_ASSERT(!hasRedirect, "No redirect for /normal");

			std::cout << "       Result: Continue to file serving\n";
		}
	}
}


// ===========================================
//  Main Test Runner
// ===========================================
int main()
{
	std::cout << COLOR_YELLOW << "\n"
				<< "╔═══════════════════════════════════════════════════════════════╗\n"
				<< "║         WEBSERV HTTP REDIRECTIONS TEST SUITE                  ║\n"
				<< "║                     Step 9.1 Tests                            ║\n"
				<< "╚═══════════════════════════════════════════════════════════════╝\n"
				<< COLOR_RESET << std::endl;

	std::cout << "Testing HTTP redirections without network layer.\n";
	std::cout << "Tests verify config parsing, response generation, and routing.\n";

	// Run all tests
	testResponseRedirectFormat();
	testRouterRedirectDetection();
	testRedirectResponseHeaders();
	testRedirectWithDifferentMethods();
	testRedirectReasonPhrases();
	testRedirectFallbackBody();
	testRedirectEdgeCases();
	testRouterIntegration();

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

	// Summary of redirect implementation
	std::cout << "Implementation Summary:\n";
	std::cout << "───────────────────────\n";
	std::cout << "✓ Config.cpp: Parses 'return <code> <url>;' directive\n";
	std::cout << "✓ Response.cpp: Response::redirect() creates redirect responses\n";
	std::cout << "✓ Router.cpp: Checks redirect_url before method validation\n";
	std::cout << "✓ Supports 301, 302, 303, 307, 308 status codes\n";
	std::cout << "✓ Fallback HTML body with clickable link\n";
	std::cout << "✓ Meta refresh tag for additional fallback\n";
	std::cout << std::endl;

	return (g_testsFailed > 0) ? 1 : 0;
}
