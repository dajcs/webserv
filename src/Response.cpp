/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:55:39 by anemet            #+#    #+#             */
/*   Updated: 2025/12/11 16:49:21 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"
#include <sstream>


/*
	=================================
		RESPONSE OVERVIEW
	=================================

	The Response class encapsulates an HTTP response that we send back to clients.

	HTTP Response Structure:
	------------------------
	HTTP/1.1 200 OK                          <- Status Line
	Content-Type: text/html                  <- Headers
	Content-Length: 1234
	Connection: keep-alive
												<- Empty line (CRLF)
	<html>                                   <- Body
	<body>Hello World</body>
	</html>

	Status Line Components:
	- HTTP Version: HTTP/1.1
	- Status Code: 200 (numeric code indicating result)
	- Reason Phrase: OK (human-readable description)

	Common Status Codes:
	- 2xx Success:   200 OK, 201 Created, 204 No Content
	- 3xx Redirect:  301 Moved Permanently, 302 Found, 304 Not Modified
	- 4xx Client Error: 400 Bad Request, 403 Forbidden, 404 Not Found, 405 Method Not Allowed
	- 5xx Server Error: 500 Internal Server Error, 502 Bad Gateway, 504 Gateway Timeout

	Headers:
	- Key-value pairs providing metadata about the response
	- Content-Type: tells browser how to interpret the body (MIME type)
	- Content-Length: size of body in bytes (important for HTTP/1.1)
	- Connection: keep-alive allows reusing TCP connection for multiple requests

	Body:
	- The actual content being sent (HTML, JSON, image data, etc.)
	- Can be text or binary data
	- Size must match Content-Length header

	Why Response Building Matters:
	- Browsers are strict about HTTP format
	- Missing CRLF (\r\n) breaks parsing
	- Incorrect Content-Length causes hangs or truncation
	- Wrong Content-Type makes browsers misinterpret data
*/



// ================================
//  Constructors and Destructors
// ================================

/*
	Default Constructor

	Creates a response with sensible defaults:
	- Status: 200 OK (success)
	- Keep-Alive: true (for HTTP/1.1 connection reuse)
	- Dirty flag: true (needs building before sending)

	The "dirty" pattern:
	We don't build the full HTTP string immediately. Instead, we mark
	the response as "dirty" when modified, then build it only when
	needed (lazy evaluation). This is efficient for responses that
	are modified multiple times before sending.
*/
Response::Response() :
_statusCode(200), _reasonPhrase("OK"), _dirty(true), _keepAlive(true) {}

/*
	Destructor

	Nothing to clean up - all members use RAII (strings, maps)
	C++98 doesn't require explicit cleanup for standard containers
*/
Response::~Response() {};

/*
	Copy Constructor

	Deep copy of all response data. Needed for returning Response
	objects by value (common pattern in our router).

	Example:
		Response Router::route(Request& req) {
			Response res;
			res.setStatus(200);
			return res;  // Copy constructor called here
		}
*/
Response::Response(const Response& other) :
	_statusCode(other._statusCode),
	_reasonPhrase(other._reasonPhrase),
	_headers(other._headers),
	_body(other._body),
	_builtResponse(other._builtResponse),
	_dirty(other._dirty),
	_keepAlive(other._keepAlive)
{}

/*
	Copy Assignment Operator

	Handles assignment between existing Response objects.
	Self-assignment check prevents issues like: response = response;
*/
Response& Response::operator=(const Response& other)
{
	if (this != &other)
	{
		_statusCode = other._statusCode;
		_reasonPhrase = other._reasonPhrase;
		_headers = other._headers;
		_body = other._body;
		_builtResponse = other._builtResponse;
		_dirty = other._dirty;
		_keepAlive = other._keepAlive;
	}
	return *this;
}

// =======================
//  Setters
// =======================

/*
	setStatus() - Set HTTP status code

	Automatically looks up the standard reason phrase.
	Marks response as dirty so it will be rebuilt.

	Example:
		response.setStatus(404);
		// Result: "HTTP/1.1 404 Not Found"
*/
void Response::setStatus(int code)
{
	_statusCode = code;
	_reasonPhrase = getReasonPhrase(code);
	_dirty = true;
}

/*
	setStatus() - Set status with custom reason phrase

	Allows non-standard reason phrases if needed.
	HTTP spec says reason phrase can be anything,
	but standard phrases improve compatibility.

	Example:
		response.setStatus(420, "Enhance Your Calm");
*/
void Response::setStatus(int code, const std::string& reason)
{
	_statusCode = code;
	_reasonPhrase = reason;
	_dirty = true;
}

/*
	setHeader() - Add or update an HTTP header

	Headers are case-sensitive in our implementation
	(technically case-insensitive per HTTP spec, but most servers use exact case)

	Common headers:
		Content-Type: text/html
		Content-Length: 1234
		Location: /new-url (for redirects)
		Set-Cookie: session=xyz
		Cache-Control: no-cache

	Example:
		response.setHeader("Content-Type", "application/json");
		response.setHeader("X-Custom-Header", "custom-value");
*/
void Response::setHeader(const std::string& name, const std::string& value)
{
	_headers[name] = value;
	_dirty = true;
}

/*
	setBody() - Set response body from string

	The body contains the actual content being sent.
	Marks response as dirty so Content-Length will be recalculated.

	Example:
		response.setBody("<html><body>Hello</body></html>");
*/
void Response::setBody(const std::string& body)
{
	_body = body;
	_dirty = true;
}

/*
	setBody() - Set response body from raw data

	Used for binary data (images, files, etc.)
	String assign() handles binary data safely (no null termination issues)

	Example:
		char imageData[1024];
		int bytesRead = read(fd, imageData, 1024);
		response.setBody(imageData, bytesRead);
*/
void Response::setBody(const char* data, size_t length)
{
	_body.assign(data, length);
	_dirty = true;
}

/*
	setContentType() - Convenience method for Content-Type header

	Content-Type tells the browser how to interpret the body.
	This is CRITICAL - wrong type breaks rendering!

	Common types:
		text/html        - HTML pages
		text/css         - Stylesheets
		application/json - JSON data
		image/jpeg       - JPEG images
		application/octet-stream - Binary/unknown

	Example:
		response.setContentType("application/json");
*/
void Response::setContentType(const std::string& type)
{
	setHeader("Content-Type", type);
}

/*
	setContentLength() - Set Content-Length header

	Content-Length is the size of the body in bytes.
	HTTP/1.1 requires this for persistent connections.

	Without Content-Length:
	- Browser doesn't know when response ends
	- Connection must close after each response (inefficient)
	- Or must use chunked encoding (more complex)

	We auto-calculate this in build() if not set manually.
*/
void Response::setContentLength(size_t length)
{
	std::stringstream ss;
	ss << length;
	setHeader("Content-Length", ss.str());
}

/*
	setConnection() - Set Connection header

	Connection: keep-alive - Keep TCP connection open for next request
	Connection: close      - Close connection after this response

	HTTP/1.1 keep-alive benefits:
	- Reuse TCP connection (avoid 3-way handshake overhead)
	- Faster subsequent requests
	- Lower CPU/memory usage

	When to use close:
	- Server overloaded (limit concurrent connections)
	- Error conditions
	- Client sent HTTP/1.0 without Connection: keep-alive
*/
void Response::setConnection(bool keepAlive)
{
	_keepAlive = keepAlive;
	setHeader("Connection", keepAlive ? "keep-alive" : "close");
}


// ===============================
//  Building the Response
// ===============================

/*
	buildIfNeeded() - Lazy build pattern

	Only rebuilds the HTTP response string if it's been modified.
	This is efficient when response is tweaked multiple times.

	HTTP Response Format (RFC 2616):
		Status-Line CRLF
		*(Header-Field CRLF)
		CRLF
		[ message-body ]

	CRLF = \r\n (Carriage Return + Line Feed)
	- Critical: HTTP requires \r\n, not just \n
	- Browser will reject responses with wrong line endings
*/
void Response::buildIfNeeded() const
{
	if (!_dirty)
	{
		return;
	}

	std::stringstream ss;

	// Status line: HTTP/1.1 200 OK\r\n
	ss << "HTTP/1.1 " << _statusCode << " " << _reasonPhrase << "\r\n";

	// Headers: each "Name: Value\r\n"
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
														it != _headers.end(); ++it)
	{
		ss << it->first << ": " << it->second << "\r\n";
	}

	// Auto-add Content-Length if not manually set
	// This is essential for HTTP/1.1 persistent connections
	if (_headers.find("Content-Length") == _headers.end())
	{
		ss << "Content-Length: " << _body.size() << "\r\n";
	}

	// End of headers (blank line separates headers from body)
	ss << "\r\n";

	// Body (can be text or binary data)
	ss << _body;

	_builtResponse = ss.str();
	_dirty = false;
}

/*
	build() - Get the complete HTTP response as a string

	Returns the full HTTP response ready to send via send().

	Example output:
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 42\r\n"
		"\r\n"
		"<html><body>Hello World</body></html>"
*/
std::string Response::build() const
{
	buildIfNeeded();
	return _builtResponse;
}

/*
	getData() - Get reference to built response

	More efficient than build() - returns const reference
	instead of copying the string. Use this when sending
	data through send() or write().
*/
const std::string& Response::getData() const
{
	buildIfNeeded();
	return _builtResponse;
}

/*
	getSize() - Get total response size in bytes

	Useful for:
	- Progress tracking
	- Bandwidth calculations
	- Debugging
*/
size_t Response::getSize() const
{
	buildIfNeeded();
	return _builtResponse.size();
}


// ==============================
//  Getters
// ==============================

int Response::getStatusCode() const
{
	return _statusCode;
}

const std::string& Response::getBody() const
{
	return _body;
}

bool Response::shouldKeepAlive() const
{
	return _keepAlive;
}


// ===============================
//  Static Factory Methods
// ===============================

/*
	ok() - Quick factory for 200 OK responses

	Convenience method for common case of successful response.

	Example:
		return Response::ok("<html>Success!</html>");

	Instead of:
		Response res;
		res.setStatus(200);
		res.setContentType("text/html");
		res.setBody("<html>Success!</html>");
		return res;
*/
Response Response::ok(const std::string& body, const std::string& contentType)
{
	Response response;
	response.setStatus(200);
	response.setContentType(contentType);
	response.setBody(body);
	return response;
}
