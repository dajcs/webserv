/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:55:28 by anemet            #+#    #+#             */
/*   Updated: 2025/12/17 14:18:52 by anemet           ###   ########.fr       */
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
	_clientIP(""),
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
	_clientIP(other._clientIP),
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
		_clientIP = other._clientIP;
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
	_clientIP.clear();
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

	// ==============================
	// 	PARSE_REQUEST_LINE State
	// ==============================

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
		// Process headers line by line until we find the empty line
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
					// For now, use a default limit of 1MB
					if(_contentLength > 10485760) // 1MB
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
					_state = PARSE_CHUNKED_BODY;
				}
				else
				{
					// No body indicators, request is complete
					// This is normal for GET, DELETE, etc.
					_state = PARSE_COMPLETE;
					return true;
				}

				break; // Exit header parsing loop

			}	// end of empty.line - marking end-of-header

			// Parse this header line
			if (!parseHeader(line))
			{
				_state = PARSE_ERROR;
				return true; // Error in header format
			}
		} // end of while(true) for headers


	} // end of _state: PARSE_HEADERS


	// ====================================
	// 	PARSE_BODY State (Content-Length)
	// ====================================

	/*
		Content-Length Body Handling

		HTTP/1.1 uses Content-Length header to specify exact body size
		Example POST request:
			POST /upload HTTP/1.1\r\n
			Host: localhost\r\n
			Content-Type: application/json\r\n
			Content-Length: 27\r\n
			\r\n
			{"name":"test","value":42}

		How it works:
			- Server knows exactly how many bytes to expect (_contentLength)
			- Read data from buffer until we have all bytes
			- Body might arrive in multiple recv() calls over network
			- Must handle partial body data gracefully

		Why this matters:
			- POST requests upload files, form data, JSON payloads
			- Server needs complete body before processing (no partial data)
			- CGI scripts expect full body in stdin
			- We can't just read indefinitely - network is unreliable

		Real-world scenario:
			Client uploads 5 MB file:
			- recv() call 1: gets 64 KB chunk
			- recv() call 2: gets 128 KB chunk
			- recv() call 3: gets 256 KB chunk
			- ... continues until all 5 MB received
	*/
	if (_state == PARSE_BODY)
	{
		// Calculate how many bytes we still need
		size_t bytesNeeded = _contentLength - _bodyBytesRead;

		// How many bytes are available in buffer?
		size_t bytesAvailable = _buffer.size();

		// Take the smaller of: what we need vs what we have
		size_t bytesToRead = (bytesAvailable < bytesNeeded) ?
								bytesAvailable : bytesNeeded;

		// Append data from buffer to body
		_body.append(_buffer, 0, bytesToRead);

		// Remove consumed data from buffer
		_buffer.erase(0, bytesToRead);

		// Update counter
		_bodyBytesRead += bytesToRead;

		// Check if we have the complete body
		if (_bodyBytesRead >= _contentLength)
		{
			// Body complete! Request is ready to process
			_state = PARSE_COMPLETE;
			return true;
		}

		// Still waiting for more body data
		return false;
	}


	// =========================================
	// 	PARSE_CHUNKED_BODY state
	// =========================================
	/*
		Chunked Transfer Encoding (RFC 7230 Section 4.1)

		Used when server/client doesn't know total size upfront
		Example: dynamically generated content, live streaming

		Format:
			<chunk-size in hex>\r\n
			<chunk-data>\r\n
			<chunk-size in hex>\r\n
			<chunk-data>\r\n
			...
			0\r\n
			\r\n

		Real example:
			POST /upload HTTP/1.1\r\n
			Host: localhost\r\n
			Transfer-Encoding: chunked\r\n
			\r\n
			5\r\n
			Hello\r\n
			7\r\n
			World!\r\n
			0\r\n
			\r\n

		Breakdown:
			"5\r\n" = next chunk is 5 bytes
			"Hello\r\n" = the 5 byte chunk + CRLF
			"7\r\n" = next chunk is 7 bytes
			" World!\r\n" = the 7 byte chunk + CRLF
			"0\r\n" = chunk size 0 = end of body
			"\r\n" = final CRLF

		Why chunked encoding exists:
			- Server generates response on-the-fly (can't pre-calculate size)
			- Client uploads large file without knowing exact size
			- Allows streaming data without buffering entire payload
			- HTTP/1.1 keep-alive requires knowing when body ends

		Important for webserv:
			- Must un-chunk before sending to CGI (CGI expects raw data)
			- Enforce size limits even for chuked data (prevent DoS)
			- Handle malformed chunks (return 400 Bad Request)
	*/
	if (_state == PARSE_CHUNKED_BODY)
	{
		if (!parseChunkedBody())
		{
			// Need more data, or parsing error occured
			// Error code set by parseChunkedBody() in case of error
			return (_state == PARSE_ERROR);
		}

		// Chunked body complete
		_state = PARSE_COMPLETE;
		return true;
	}

	// if we reach here, we're in PARSE_COMPLETE or PARSE_ERROR state
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


/*
	parseChunkedBody() - Parse HTTP chunked transfer encoding

	Chunk format:
		<size-hex>\r\n<data>\r\n<size-hex>\r\n<data>\r\n ... 0\r\n\r\n

	State machine approach:
		1. Read chunk size line (hex number)
		2. Read chunk data (exactly size bytes)
		3. Read trailing CRLF after data
		4. Repeat until size = 0
		5. Read final CRLF

	Returns:
		true if chunked body is complete
		false if need more data or error occured
*/
bool Request::parseChunkedBody()
{
	/*
		Chunked encoding state machine:
		We need to track which part of the chunk we're parsing

		States:
		- Reading chunk size line
		- Reading chunk data
		- Reading trailing CRLF after chunk data
		- Reading final CRLF after last chunk of size 0
	*/

	while (true)
	{
		// =================================
		//  Step 1: Parse Chunk Size Line
		// =================================
		// Format: "<hex-size>\r\n"
		// Example: "1A\r\n" means next chunk is 26 bytes (0x1A = 26)

		// Look for the CRLF that ends the chunk size line
		size_t crlfPos = _buffer.find("\r\n");

		if (crlfPos == std::string::npos)
		{
			// Haven't received complete chunk size line yet
			// Wait for more data

			// but prevent malicious clients from sending huge chunk size
			if (_buffer.size() > 100) // shouldn't be > 100 chars before \r\n
			{
				_state = PARSE_ERROR;
				_errorCode = 400; // Bad Request
				return false;
			}
			return false; // need more data
		}

		// Extract chunk size line (without \r\n)
		std::string chunkSizeLine = _buffer.substr(0, crlfPos);

		// Remove chunk size line from buffer (including \r\n)
		_buffer.erase(0, crlfPos + 2);

		// ==================================
		// Step 2: Parse Hex Chunk Size
		// ==================================
		// Convert hex string to integer
		// Example: "1A" -> 26, "FF" -> 255, "0" -> 0

		// Validate hex string (only 0-9, A-F, a-f allowed)
		for (size_t i = 0; i < chunkSizeLine.length(); i++)
		{
			char c = chunkSizeLine[i];
			if (!(	(c >= '0' && c <= '9') ||
					(c >= 'A' && c <= 'F') ||
					(c >= 'a' && c <= 'f')))
			{
				// Invalid character in chunk size
				_state = PARSE_ERROR;
				_errorCode = 400; // Bad Request
				return false;
			}
		}

		// Convert hex string to number
		char* endPtr;
		/*
			strtoul: convert null terminated C-string to unsigned long
					set endPtr to first non-convertable character
					number base: 16 (hex)
		*/
		size_t chunkSize = std::strtoul(chunkSizeLine.c_str(), &endPtr, 16);

		// Check for conversion errors
		if (*endPtr != '\0')
		{
			_state = PARSE_ERROR;
			_errorCode = 400; // Bad Request
			return false;
		}

		// ====================================
		//  Step 3: Handle Last Chunk (Size 0)
		// ====================================
		/*
			Chunk size 0 marks end of chunked body
			Format: "0\r\n\r\n"

			After "0\r\n", there should be one more "\r\n"
			This final CRLF marks absolute end of body
		*/
		if (chunkSize == 0)
		{
			// Last chunk! Check for final CRLF
			if (_buffer.size() < 2)
			{
				// Need more data for final CRLF
				return false;
			}

			// Verify final CRLF exists
			if (_buffer.substr(0, 2) != "\r\n")
			{
				_state = PARSE_ERROR;
				_errorCode = 400; // Bad Request
				return false;
			}

			// Remove final CRLF
			_buffer.erase(0, 2);

			// Chunked body complete!
			// _body now contains the un-chunked data ready for processing
			return true;
		}

		// ===================================
		//  Step 4: Enforce Body Size Limit
		// ===================================
		/*
			Even with chunked encoding, we must limit total body size
			Prevent DoS attacks with infinite chunks

			Example attack:
				Client sends: 1000000\r\n<1MB data>\r\n1000000\r\n<1MB data>...
				Without limit, serer runs out of memory
		*/
		if (_body.size() + chunkSize > 1048576) // 1MB limit
		{
			_state = PARSE_ERROR;
			_errorCode = 413; // Payload Too Large
			return false;
		}

		// ==========================
		//  Step 5: Read Chunk Data
		// ==========================
		// We need: chunkSize bytes + "\r\n" (2 bytes)
		size_t totalNeeded = chunkSize + 2;

		if (_buffer.size() < totalNeeded)
		{
			// Don't have complete chunk yet, need more data
			return false;
		}

		// Extract chunk data (without trailing \r\n)
		std::string chunkData = _buffer.substr(0, chunkSize);

		// Verify trailing CRLF after chunk data
		if (_buffer.substr(chunkSize, 2) !="\r\n")
		{
			_state = PARSE_ERROR;
			_errorCode = 400; // Bad Request - malformed chunk
			return false;
		}

		// Remove chunk data + trailing CRLF from buffer
		_buffer.erase(0, totalNeeded);

		// ================================
		//  Step 6: Append chunk to Body
		// ================================
		/*
			Un-chunking process:
			We remove the chunk framing (size + CRLF) and just add raw data
		*/
		_body.append(chunkData);

		// Loop continues to parse next chunk
		// Will return false if buffer empty (need more data)
		// Will return true when we hit the "0\r\n\r\n" final chunk

	} // end of while(true)

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

void Request::setClientIP(const std::string& ip)
{
	_clientIP = ip;
}

const std::string& Request::getClientIP() const
{
	return _clientIP;
}
