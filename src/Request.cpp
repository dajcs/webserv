/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:55:28 by anemet            #+#    #+#             */
/*   Updated: 2025/12/09 20:39:01 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"
#include <algorithm>
#include <cstdlib>
#include <sstream>

// ==============================================
//       Constructors & Destructors
// ==============================================

/*
	Default Constructor
		_state(PARSE_REQUEST_LINE)

			- HTTP requests always begin with the request line (RFC 7230)
			- Format: "METHOD URI HTTP/VERSION\r\n"
			- Example: "GET /index.html HTTP/1.1\r\n"
*/

// Default Constructor
Request::Request() :
	_method(""),
	_uri(""),
	_path(""),
	_queryString(""),
	_httpVersion(""),
	_body(""),
	_state(PARSE_REQUEST_LINE),
	_errorCode(0),
	_buffer(""),
	_contentLength(0),
	_bodyBytesRead(0)
{}

// Destructor
Request::~Request() {}

// Copy constructor
Request::Request(const Request& other) :
	_method(other._method),
	_uri(other._uri),
	_path(other._path),
	_queryString(other._queryString),
	_httpVersion(other._httpVersion),
	_body(other._body),
	_state(other._state),
	_errorCode(other._errorCode),
	_buffer(other._buffer),
	_contentLength(other._contentLength),
	_bodyBytesRead(other._bodyBytesRead)
{}

// copy assignment operator
Request& Request::operator=(const Request& other)
{
	if (this != &other)
	{
		_method = other._method;
		_uri = other._uri;
		_path = other._path;
		_queryString = other._queryString;
		_httpVersion = other._httpVersion;
		_body = other._body;
		_state = other._state;
		_errorCode = other._errorCode;
		_buffer = other._buffer;
		_contentLength = other._contentLength;
		_bodyBytesRead = other._bodyBytesRead;
	}
	return *this;
}


// =============================================
//          Public Methods
// =============================================

/*
	Reset the request for reuse (important for keep-alive connections)

	HTTP/1.1 Keep-Alive:
		- Single TCP connection can handle multiple requests
		- After sending response, we reuse the connection
		- Must reset request state to parse next request
		- Saves overhead of creating new connections
*/
void Request::reset()
{
	_method.clear();
	_uri.clear();
	_path.clear();
	_queryString.clear();
	_httpVersion.clear();
	_body.clear();
	_headers.clear();
	_buffer.clear();

	_state = PARSE_REQUEST_LINE;
	_errorCode = 0;
	_contentLength = 0;
	_bodyBytesRead = 0;
}


/*
	parse() - Main parsing function, feeds incoming data to the request parser

	Incremental parsing:
		- HTTP data arrives over network in chunks (non-blocking I/O)
		- We might receive: "GET / HT" ... then later "TP/1.1\r\n..."
		- Must buffer partial data and wait for more
		- Can't use blocking read() - that would freeze entire server

	Input:
		data: raw bytes received from socket
	Return true when request is parsed and ready to process,
			false when we need more data
*/
bool Request::parse(const std::string& data)
{
	// Add incoming data to the buffer
	_buffer += data;

	// State machine: process based on current parsing stage
	// This allows handling requests that arrive in multiple recv() calls
	if (_state == PARSE_REQUEST_LINE)
	{
		// Look for end of line: \r\n (CRLF)
		// HTTP spec requires CRLF line endings
		size_t pos = _buffer.find("\r\n");
		if (pos == std::string::npos)
		{
			// Not enough data yet, wait for more
			// Prevent buffer overflow from malicious/broken clients
			if (_buffer.size() > 8192) // 8 kB limit for request line
			{
				_state = PARSE_ERROR;
				_errorCode = 414; // URI too long
				return true;
			}
			return false; // Need more data
		}

		// Extract request line (without \r\n)
		std::string requestLine = _buffer.substr(0, pos);

		// Remove processed data from buffer (including \r\n)
		_buffer.erase(0, pos + 2);

		// Parse the request line
		if (!parseRequestLine(requestLine))
		{
			_state = PARSE_ERROR;
			return true; // Error, but parsing is "complete"
		}

		// Move to next state: parsing headers
		_state = PARSE_HEADERS;

	} // end of PARSE_REQUEST_LINE


	// ==============================
	// 	PARSE_HEADERS State
	// ==============================
	/*
		HTTP Headers format (RFC 7230)
		Header-Name: Header-Value\r\n
		Another-Header: Another-Value\r\n
		\r\n  <- Empty line marks end of headers

		Example:
			GET / HTTP/1.1\r\n
			Host: localhost:8080\r\n
			User-Agent: Mozilla/5.0\r\n
			Accept: text/html\r\n
			\r\n  <- Headers complete, body starts here (if any)

		Important headers we need to handle:
			- Host: Required in HTTP/1.1, identifies the server
			- Content-Length: Size of request body in bytes
			- Content-Type: Format of request body (e.g., application/json)
			- Transfer Encoding: chunked (alternative to Content-Length)
			- Connection: keep-alive or close
	*/
	if (_state == PARSE_HEADERS)
	{
		// Process headers line by lin until we find the empty line
		while (true)
		{
			// Look for next line ending
			size_t pos = _buffer.find("\r\n");
			if (pos == std::string::npos)
			{
				// No complete line yet, need more data
				// But prevent header section from being too large
				if (_buffer.size() > 8192) // 8 kB size limit for all headers
				{
					_state = PARSE_ERROR;
					_errorCode = 431; // Request Header Fields Too Large
					return true;
				}
				return false; // Wait for more data
			}

			// Extract the line (without \r\n)
			std::string line = _buffer.substr(0, pos);

			// Remove processed line from buffer (including \r\n)
			_buffer.erase(0, pos + 2);

			// Empty line marks end of headers
			if (line.empty())
			{
				// Headers are complete!
				// Now we need to determine what comes next:

				// Check if HTTP/1.1 requires Host header
				if (_httpVersion == "HTTP/1.1")
				{
					// RFC 7230: HTTP/1.1 requests MUST include Host header
					if (getHeader("Host").empty())
					{
						_state = PARSE_ERROR;
						_errorCode = 400; // Bad Request
						return true;
					}
				}

				// Determine if request has a body
				// Body is present if:
				// 	1. Content-Length header exists OR
				// 	2. Transfer-Encoding: chunked is present

				std::string contentLength = getHeader("Content-Length");
				std::string transferEncoding = getHeader("Transfer-Encoding");

				if (!contentLength.empty())
				{
					// Content-Length body
					_contentLength = std::atol(contentLength.c_str());

					// TODO: Check against client_max_body_size from config
					// For now, use a default limit of 10MB
					if(_contentLength > 10485760) // 10 MB
					{
						_state = PARSE_ERROR;
						_errorCode = 413; // Payload Too Large
						return true;
					}

					if (_contentLength > 0)
					{
						// Request has a body, move to body parsing
						_state = PARSE_BODY;
					}
					else
					{
						// No body (Content-Length: 0)
						_state = PARSE_COMPLETE;
					}
				}
				else if (!transferEncoding.empty() &&
							transferEncoding.find("chunked") != std::string::npos)
				{
					// Chunked transfer encoding
					// TODO: Implement in Step 4.3
					_state = PARSE_CHUNKED_BODY;
				}
				else
				{
					// No body indicators, request is complete
					// This is normal for GET, DELETE, etc.
					_state = PARSE_COMPLETE;
				}

				break; // Exit header parsing loop

			}	// end of empty.line - marking end-of-header

			// Parse this header line
			if (!parseHeader(line))
			{
				_state = PARSE_ERROR;
				return true; // Error in header format
			}
		}
	}








	// TODO: Implement body parsing in Step 4.3
	// if (_state == PARSE_BODY) { ... }
	// if (_state == PARSE_CHUNKED_BODY) { ... }

	return (_state == PARSE_COMPLETE || _state == PARSE_ERROR);
}


// ===============================================
//    Private Methods  --  Request Line Parsing
// ===============================================

/*
	Parse the HTTP request line: "METHOD URI HTTP/VERSION"

	HTTP Request Line Structure (RFC 7230):
		- METHOD: The action to perform (GET, POST, DELETE, etc.)
		- URI: The resource identifier (path + optional query string)
		- HTTP-VERSION: Protocol version (HTTP/1.0 or HTTP/1.1)
		- Components are separated by single space (SP)

	Examples:
		"GET /index.html HTTP/1.1"
		"POST /api/upload?type=image HTTP/1.1"
		"DELETE /files/document.txt HTTP/1.0"

	Input:
		line: the first line of HTTP request (without \r\n)
	Return: true if valid
			false if malformed (triggers 400 Bad Request)
*/
bool Request::parseRequestLine(const std::string& line)
{
	// use stringstream
	std::stringstream ss(line);

	// Extract method, URI, and version separated by spaces
	if (!(ss >> _method >> _uri >> _httpVersion))
	{
		// Parsing failed - missing components
		// Example bad request: "GET" (missing URI and version)
		_errorCode = 400; // Bad Request
		return false;
	}

	// Check if there's extra garbage after the three tokens
	std::string extra;
	if (ss >> extra)
	{
		// Too many components
		// Example: "GET /index.html HTTP/1.1 EXTRA_STUFF"
		_errorCode = 400; // Bad Request
		return false;
	}

	// =============================
	//  Validate HTTP method
	// =============================
	// We must support at least: GET, POST, DELETE (per subject requirements)
	// Optional: HEAD, PUT, PATCH, OPTIONS, CONNECT, TRACE
	if (_method != "GET" && _method != "POST" && _method != "DELETE" &&
		_method != "HEAD" && _method != "PUT")
	{
		// Method exists but we don't implement it
		_errorCode = 501; // Not Implemented
		return false;
	}


	// =============================
	//  Validate URI
	// =============================
	/*
		URI must start with '/' (absolute path) or be '*' (OPTIONS)
		Example of valid URIs:
			"/"					-> root
			"/index.html"		-> file path
			"/api/users?id=123" -> path with query string
	*/
	if (_uri.empty() || (_uri[0] != '/' && _uri != "*"))
	{
		_errorCode = 400; // Bad Request
		return false;
	}

	// Limit URI length to prevent attacks
	if (_uri.length() > 2048) // commont limit
	{
		_errorCode = 414; // URI Too Long
		return false;
	}

	// ===============================
	//  Parse URI into Path and Query String
	/*
		URI format: /path/to/resource?key1=value1&key2=value2
		            |_______________| |_____________________|
					      path             query string

		Query string is optional, separated by '?'
	*/
	size_t queryPos = _uri.find('?');
	if (queryPos != std::string::npos)
	{
		// URI has a query string
		_path = _uri.substr(0, queryPos);			// string before '?'
		_queryString = _uri.substr(queryPos + 1);	// string after '?'
	}
	else
	{
		// No query string, the entire URI is the path
		_path = _uri;
		_queryString = "";
	}

	// ===================================
	//  Validate HTTP Version
	// ===================================
	//	We support HTTP/1.0 and HTTP/1.1
	if (_httpVersion != "HTTP/1.0" && _httpVersion != "HTTP/1.1")
	{
		_errorCode = 505; // HTTP Version Not Supported
		return false;
	}

	// ============
	//  SUCCESS!
	// ============
	// Request is valid, we can proceed to parse headers
	return true;
}

/*
	parseHeader() - Parse a single HTTP header line

	HTTP Header Format (RFC 7230):
		Header-Name: Header-Value
		- Header name and value separated by colon ':'
		- Optional whitespace after colon (leading white-space in value)
		- Header names are case-insensitive
		- Multiple headers with same name can exist (we'll keep last value)

	Examples:
		"Host: localhost:8080"
		"Content-Type: application/json"
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64)"

	Special cases to handle:
		- Multi-line headers (obsolete in HTTP/1.1, but we should handle)
		- Empty header values
		- Headers with multiple colons in the value

	Input:
		line: A single header line (without \r\n)
	Return:
		true if valid header, false if malformed
*/
bool Request::parseHeader(const std::string& line)
{
	// Find the colon separator between name and value
	size_t colonPos = line.find(':');

	if (colonPos == std::string::npos)
	{
		// No colon found -> malformed header
		// Example bad header: "HostLocalhost" (missing colon)
		_errorCode = 400; // Bad Request
		return false;
	}

	// Extract header name (everything before colon)
	std::string name = line.substr(0, colonPos);

	// Validate header name
	// RFC 7230: Header names must not be empty and should not have whitespace
	if (name.empty())
	{
		_errorCode = 400; // Bad Request
		return false;
	}

	// Check for whitespace in header name (not allowed)
	if (name.find(' ') != std::string::npos ||
		name.find('\t') != std::string::npos)
	{
		_errorCode = 400; // Bad Request
		return false;
	}

	// Extract header value (everything after colon)
	std::string value = line.substr(colonPos + 1);

	// Trim optional leading whitespace from value
	size_t valueStart = 0;
	while (valueStart < value.length() &&
			(value[valueStart] == ' ' || value[valueStart] == '\t'))
	{
		valueStart++;
	}
	value = value.substr(valueStart);

	// Trim trailing whitespace from value
	size_t valueEnd = value.length();
	while (valueEnd > 0 &&
			(value[valueEnd - 1] == ' ' || value[valueEnd - 1] == '\t'))
	{
		valueEnd--;
	}
	value = value.substr(0, valueEnd);

	// Convert header name to lowercase for case-insensitive storage
	// HTTP headers are case-insensitive (RFC 7230)
	std::string lowerName = name;
	std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

	// Store the header
	// Note: if header already exists, this will overwrite it
	// Some headers can appear multiple times (e.g., Cookie, Set-Cookie)
	// For simplicity we keep the last value (optional: handle multi-value headers)
	_headers[lowerName] = value;

	return true;
}

// TODO: Implement in Step 4.3
bool Request::parseChunkedBody()
{
	return false;
}

// =======================
//  GETTERS
// =======================

const std::string& Request::getMethod() const
{
	return _method;
}

const std::string& Request::getUri() const
{
	return _uri;
}

const std::string& Request::getPath() const
{
	return _path;
}

const std::string& Request::getQueryString() const
{
	return _queryString;
}

const std::string& Request::getHttpVersion() const
{
	return _httpVersion;
}

const std::string& Request::getBody() const
{
	return _body;
}

std::string Request::getHeader(const std::string& name) const
{
	// HTTP headers are case-insensitive (RFC 7230)
	// "Content-Type", "content-type", "CONTENT-TYPE" are all equivalent
	// We need to do case-insensitive lookup

	// Convert search name to lowercase for comparison
	std::string lowerName = name;
	/*
		apply standard C tolower() function to characters
			from lowerName.begint() to lowerName.end()
			write result to lowerName.begin() - this means string overwritten
	*/
	std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

	// Search through headers (assuming we store them in lowercase)
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
			it != _headers.end(); ++it)
	{
		std::string headerName = it->first;
		std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::tolower);

		if (headerName == lowerName)
		{
			return it->second;
		}
	}

	return ""; // Header not found
}

const std::map<std::string, std::string>& Request::getHeaders() const
{
	return _headers;
}

Request::ParseState Request::getState() const
{
	return _state;
}

bool Request::isComplete() const
{
	return (_state == PARSE_COMPLETE);
}

bool Request::hasError() const
{
	return _errorCode;
}

int Request::getErrorCode() const
{
	return _errorCode;
}

size_t Request::getContentLength() const
{
	return _contentLength;
}

size_t Request::getBodySize() const
{
	return _body.size();
}
