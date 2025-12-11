/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:55:39 by anemet            #+#    #+#             */
/*   Updated: 2025/12/11 22:05:50 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"
#include <sstream>
#include <ctime>
#include <cstring>


/*
	=================================
		RESPONSE OVERVIEW
	=================================

	The Response class encapsulates an HTTP response that we send back to clients.

	HTTP Response Structure (RFC 7230):
	-----------------------------------
	HTTP/1.1 200 OK							<- Status Line
	Date: Wed, 11 Dec 2025 12:00:00 GMT		<- Date header (recommended)
	Server: webserv/1.0						<- Server identification
	Content-Type: text/html					<- MIME type of body
	Content-Length: 1234					<- Size of body in bytes
	Connection: keep-alive					<- Connection handling
											<- Empty line (CRLF)
	<html>									<- Body
	<body>Hello World</body>
	</html>

	Status Line Components:
	- HTTP Version: HTTP/1.1 (we support both 1.0 and 1.1)
	- Status Code: 3-digit numeric code (200, 404, 500, etc.)
	- Reason Phrase: Human-readable description ("OK", "Not Found")

	Status Code Categories:
	- 1xx Informational: Request received, continuing process
	- 2xx Success:   200 OK, 201 Created, 204 No Content
	- 3xx Redirect:  301 Moved Permanently, 302 Found, 304 Not Modified
	- 4xx Client Error: 400 Bad Request, 403 Forbidden, 404 Not Found
	- 5xx Server Error: 500 Internal Server Error, 502 Bad Gateway

	Required Headers for HTTP/1.1:
	- Content-Length OR Transfer-Encoding: chunked (to know body size)
	- Date: Current server time (recommended by RFC)
	- Server: Identifies the server software (optional but common)
	- Content-Type: MIME type (critical for browser rendering)
	- Connection: keep-alive or close (controls TCP connection)

	Why correct formatting matters:
	- Browsers are strict about HTTP format
	- Missing CRLF (\r\n) breaks parsing completely
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
	_statusCode(200),
	_reasonPhrase("OK"),
	_dirty(true),
	_keepAlive(true)
{}

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
		text/html		- HTML pages
		text/css		 - Stylesheets
		application/json - JSON data
		image/jpeg	   - JPEG images
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
	Connection: close	  - Close connection after this response

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



// ================================
//  Standard Headers (Step 6.1)
// ================================

/*
	formatHttpDate() - Format timestamp as HTTP date string

	HTTP/1.1 uses a specific date format (RFC 7231):
	"Wed, 11 Dec 2025 12:00:00 GMT"

	This format is:
	- Day name (3 chars), comma, space
	- Day (2 digits), space
	- Month name (3 chars), space
	- Year (4 digits), space
	- Time (HH:MM:SS), space
	- "GMT" (always GMT, never local time)

	Why GMT?
	HTTP is a global protocol. Using local time would cause confusion
	between servers and clients in different timezones. GMT provides
	a universal reference point.
*/
std::string Response::formatHttpDate(time_t timestamp)
{
	struct tm* gmt = gmtime(&timestamp);
	if (!gmt)
	{
		return "Thu, 01 Jan 1970 00:00:00 GMT"; // Fallback to epoch
	}

	// Day and month names as required by HTTP date format
	static const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	static const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
									"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	std::stringstream ss;
	ss << days[gmt->tm_wday] << ", ";

	// Day with leading zero if needed
	if (gmt->tm_mday < 10)
		ss << "0";
	ss << gmt->tm_mday << " ";

	ss << months[gmt->tm_mon] << " ";
	ss << (1900 + gmt->tm_year) << " ";

	// Time with leading zeros
	if (gmt->tm_hour < 10)
		ss << "0";
	ss << gmt->tm_hour << ":";
	if (gmt->tm_min < 10)
		ss << "0";
	ss << gmt->tm_min << ":";
	if (gmt->tm_sec < 10)
		ss << "0";
	ss << gmt->tm_sec << " GMT";

	return ss.str();
}


/*
	addDateHeader() - Add current date/time header

	The Date header indicates when the response was generated.
	This is useful for:
	- Caching (determining if content is fresh)
	- Logging and debugging
	- Time synchronization

	RFC 7231 recommends including Date in all responses.
*/
void Response::addDateHeader()
{
	setHeader("Date", formatHttpDate(time(NULL)));
}

/*
	addServerHeader() - Add server identification header

	The Server header identifies the server software.
	Example: "Server: webserv/1.0"

	This is optional but common. It helps:
	- Debugging (what server is responding?)
	- Statistics (what servers are popular?)
	- Security research (identifying vulnerable versions)

	Note: Some security-conscious servers omit this header
	or use a generic value to prevent fingerprinting.
*/
void Response::addServerHeader()
{
	setHeader("Server", "webserv/1.0");
}

/*
	addStandardHeaders() - Add all standard headers at once

	Convenience method that adds:
	- Date: Current server time
	- Server: Server identification
	- Connection: Based on keep-alive setting

	Call this before building the response to ensure
	all recommended headers are present.
*/
void Response::addStandardHeaders()
{
	addDateHeader();
	addServerHeader();
	setConnection(_keepAlive);
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

	// STATUS LINE: HTTP/1.1 200 OK\r\n
	ss << "HTTP/1.1 " << _statusCode << " " << _reasonPhrase << "\r\n";

	/*
		HEADERS
		Format: Header-Name: Header-Value CRLF

		Each header on its own line, colon-space between name and value.
		Order doesn't matter (mostly), but consistency helps debugging.
	*/
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
		it != _headers.end(); ++it)
	{
		ss << it->first << ": " << it->second << "\r\n";
	}

	/*
		AUTO-ADD Content-Length if not manually set

		Content-Length is essential for HTTP/1.1:
		- Tells browser exactly how many bytes to expect
		- Enables persistent connections (keep-alive)
		- Without it, browser doesn't know when body ends

		Exception: 204 `No Content` responses shouldn't have Content-Length
	*/
	if (_headers.find("Content-Length") == _headers.end() && _statusCode != 204)
	{
		ss << "Content-Length: " << _body.size() << "\r\n";
	}

	/*
		END OF HEADERS
		Empty line (just CRLF) separates headers from body
		This is CRITICAL - missing empty line breaks HTTP parsing
	*/
	ss << "\r\n";

	/*
		BODY
		The actual content being sent.
		Can be text (HTML, JSON) or binary (images, files).
		Size must match Content-Length header exactly.
	*/
	ss << _body;

	_builtResponse = ss.str();
	_dirty = false;
}

/*
	build() - Get the complete HTTP response as a string

	Returns the full HTTP response ready to send via send().

	Example output:
		"HTTP/1.1 200 OK\r\n"
		"Date: Wed, 11 Dec 2025 12:00:00 GMT\r\n"
		"Server: webserv/1.0\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 42\r\n"
		"Connection: keep-alive\r\n"
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
	- Logging
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

const std::string& Response::getReasonPhraseValue() const
{
	return _reasonPhrase;
}

/*
	getHeader() - Get a specific header value

	Returns the header value if found, empty string otherwise.
	Useful for testing and debugging.
*/
std::string Response::getHeader(const std::string& name) const
{
	std::map<std::string, std::string>::const_iterator it = _headers.find(name);
	if (it != _headers.end())
	{
		return it->second;
	}
	return "";
}

/*
	hasHeader() - Check if a header exists

	Returns true if the header is set, false otherwise.
*/
bool Response::hasHeader(const std::string& name) const
{
	return _headers.find(name) != _headers.end();
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
	response.addStandardHeaders();
	return response;
}

/*
	getDefaultErrorPage() - Generate HTML for error page

	Creates a simple, clean error page with:
	- Error code and reason in title
	- Large heading with error code
	- Server identification

	This is used when no custom error page is configured.
*/
std::string Response::getDefaultErrorPage(int code)
{
	std::ostringstream body;
	body << "<!DOCTYPE html>\n";
	body << "<html>\n";
	body << "<head>\n";
	body << "    <meta charset=\"UTF-8\">\n";
	body << "    <title>" << code << " " << getReasonPhrase(code) << "</title>\n";
	body << "    <style>\n";
	body << "        body { font-family: Arial, sans-serif; text-align: center; padding: 50px; }\n";
	body << "        h1 { font-size: 50px; color: #333; }\n";
	body << "        p { color: #666; }\n";
	body << "        hr { border: none; border-top: 1px solid #ddd; margin: 20px 0; }\n";
	body << "    </style>\n";
	body << "</head>\n";
	body << "<body>\n";
	body << "    <h1>" << code << "</h1>\n";
	body << "    <p>" << getReasonPhrase(code) << "</p>\n";
	body << "    <hr>\n";
	body << "    <p><small>webserv/1.0</small></p>\n";
	body << "</body>\n";
	body << "</html>\n";
	return body.str();
}


/*
	error() - Create a default error response

	Generates a complete error response with:
	- Appropriate status code and reason phrase
	- Default HTML error page
	- All standard headers
*/
Response Response::error(int code)
{
	Response response;
	response.setStatus(code);
	response.setContentType("text/html; charset=UTF-8");
	response.setBody(getDefaultErrorPage(code));
	response.addStandardHeaders();
	return response;
}

/*
	error() - Create error response with custom body

	Allows custom error pages (configured per-server).
	Use this when the config specifies a custom error page.
*/
Response Response::error(int code, const std::string& customBody)
{
	Response response;
	response.setStatus(code);
	response.setContentType("text/html; charset=UTF-8");
	response.setBody(customBody);
	response.addStandardHeaders();
	return response;
}

/*
	redirect() - Create a redirect response

	HTTP redirects tell the browser to go to a different URL.

	Common redirect codes:
	- 301 Moved Permanently: URL has permanently changed (cached)
	- 302 Found: Temporary redirect (not cached)
	- 303 See Other: Redirect after POST (use GET)
	- 307 Temporary Redirect: Keep original method
	- 308 Permanent Redirect: Keep original method, permanent

	The Location header contains the new URL.
	Body contains fallback HTML for clients that don't follow redirects.
*/
Response Response::redirect(int code, const std::string& location)
{
	Response response;
	response.setStatus(code);
	response.setHeader("Location", location);
	response.setContentType("text/html; charset=UTF-8");

	std::ostringstream body;
	body << "<!DOCTYPE html>\n";
	body << "<html>\n";
	body << "<head>\n";
	body << "    <meta charset=\"UTF-8\">\n";
	body << "    <title>Redirect</title>\n";
	body << "    <meta http-equiv=\"refresh\" content=\"0; url=" << location << "\">\n";
	body << "</head>\n";
	body << "<body>\n";
	body << "    <h1>Redirecting...</h1>\n";
	body << "    <p>If you are not redirected automatically, ";
	body << "<a href=\"" << location << "\">click here</a>.</p>\n";
	body << "</body>\n";
	body << "</html>\n";

	response.setBody(body.str());
	response.addStandardHeaders();
	return response;
}

/*
	noContent() - Create a 204 No Content response

	Used for successful operations that don't return content.
	Typical use: DELETE method (file deleted successfully).

	204 responses:
	- MUST NOT have a body
	- MUST NOT have Content-Length header (or it should be 0)
	- Still include standard headers (Date, Server)
*/
Response Response::noContent()
{
	Response response;
	response.setStatus(204, "No Content");
	response.addStandardHeaders();
	// No body, no Content-Length for 204
	return response;
}



// ===============================
//  Static Helpers
// ===============================

/*
	getReasonPhrase() - Get standard reason phrase for status code

	HTTP status codes have standard reason phrases defined in RFCs.
	While the phrase is technically optional (and ignored by HTTP/2+),
	it helps with debugging and log readability.

	We support all common status codes that webserv might use.
*/
std::string Response::getReasonPhrase(int code)
{
	switch (code)
	{
		// 2xx Success
		case 200: return "OK";
		case 201: return "Created";
		case 204: return "No Content";

		// 3xx Redirection
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 303: return "See Other";
		case 304: return "Not Modified";
		case 307: return "Temporary Redirect";
		case 308: return "Permanent Redirect";

		// 4xx Client Errors
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 408: return "Request Timeout";
		case 409: return "Conflict";
		case 411: return "Length Required";
		case 413: return "Payload Too Large";
		case 414: return "URI Too Long";
		case 415: return "Unsupported Media Type";

		// 5xx Server Errors
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 502: return "Bad Gateway";
		case 503: return "Service Unavailable";
		case 504: return "Gateway Timeout";
		case 505: return "HTTP Version Not Supported";

		default:  return "Unknown";
	}
}

/*
	getMimeType() - Get MIME type from file extension

	MIME types (Media Types) tell the browser how to handle content.
	Without correct MIME type, browser might:
	- Download instead of display HTML
	- Show garbled text for binary files
	- Refuse to execute JavaScript

	Format: type/subtype (e.g., "text/html", "image/png")

	Security note: Always set Content-Type explicitly.
	Browser "sniffing" can cause security issues.
*/
std::string Response::getMimeType(const std::string& extension)
{
	// Text types
	if (extension == ".html" || extension == ".htm")
		return "text/html; charset=UTF-8";
	if (extension == ".css")
		return "text/css; charset=UTF-8";
	if (extension == ".js")
		return "application/javascript; charset=UTF-8";
	if (extension == ".json")
		return "application/json; charset=UTF-8";
	if (extension == ".xml")
		return "application/xml; charset=UTF-8";
	if (extension == ".txt")
		return "text/plain; charset=UTF-8";

	// Image types
	if (extension == ".jpg" || extension == ".jpeg")
		return "image/jpeg";
	if (extension == ".png")
		return "image/png";
	if (extension == ".gif")
		return "image/gif";
	if (extension == ".ico")
		return "image/x-icon";
	if (extension == ".svg")
		return "image/svg+xml";
	if (extension == ".webp")
		return "image/webp";

	// Document types
	if (extension == ".pdf")
		return "application/pdf";
	if (extension == ".zip")
		return "application/zip";
	if (extension == ".gz" || extension == ".gzip")
		return "application/gzip";
	if (extension == ".tar")
		return "application/x-tar";

	// Media types
	if (extension == ".mp3")
		return "audio/mpeg";
	if (extension == ".mp4")
		return "video/mp4";
	if (extension == ".webm")
		return "video/webm";

	// Font types
	if (extension == ".woff")
		return "font/woff";
	if (extension == ".woff2")
		return "font/woff2";
	if (extension == ".ttf")
		return "font/ttf";

	// Default for unknown types
	// application/octet-stream tells browser to download, not display
	return "application/octet-stream";
}
