# Make sure you're in the webserv directory
cd /home/anemet/webserv

# Compile the test (standalone, no network dependencies)
c++ -std=c++98 -Wall -Wextra -Werror -I inc src/Request.cpp test_request_step4_1.cpp -o test_step4_1

# Run the tests
./test_step4_1


What this test covers:

✅ Valid requests - All HTTP methods (GET, POST, DELETE, HEAD, PUT)
✅ Query strings - Parsing URI paths vs query parameters
✅ HTTP versions - Both 1.0 and 1.1
✅ Error handling - 400, 414, 501, 505 responses
✅ Incremental parsing - Simulates non-blocking network I/O
✅ Buffer overflow protection - Security against malicious requests
✅ Keep-alive simulation - Request reuse via reset()
Why this works without network code:

The Request::parse() method accepts a std::string parameter
In production, this string comes from recv() on a socket
For testing, we just pass strings directly
This tests the parsing logic independently of network I/O
Next steps:

After this test passes, you'll be ready to:

Implement Step 4.2 (header parsing)
Implement Step 4.3 (body parsing)
Eventually integrate with the network layer (Steps 2-3)
The beauty of this design is that Student 2 can develop and test all HTTP parsing without waiting for Student 1's network code!






AI Request.cpp
```cpp
/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:55:28 by anemet            #+#    #+#             */
/*   Updated: 2025/12/09 09:05:02 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"
#include <algorithm>
#include <cstdlib>
#include <sstream>

// ============================================================================
// CONSTRUCTORS & DESTRUCTORS
// ============================================================================

/**
 * Default constructor - initializes a fresh request in PARSE_REQUEST_LINE state
 * 
 * Why start with PARSE_REQUEST_LINE?
 * - HTTP requests always begin with the request line (RFC 7230)
 * - Format: "METHOD URI HTTP/VERSION\r\n"
 * - Example: "GET /index.html HTTP/1.1\r\n"
 */
Request::Request()
    : _method("")
    , _uri("")
    , _path("")
    , _queryString("")
    , _httpVersion("")
    , _body("")
    , _state(PARSE_REQUEST_LINE)
    , _errorCode(0)
    , _buffer("")
    , _contentLength(0)
    , _bodyBytesRead(0)
{
}

Request::~Request()
{
}

Request::Request(const Request& other)
    : _method(other._method)
    , _uri(other._uri)
    , _path(other._path)
    , _queryString(other._queryString)
    , _httpVersion(other._httpVersion)
    , _body(other._body)
    , _headers(other._headers)
    , _state(other._state)
    , _errorCode(other._errorCode)
    , _buffer(other._buffer)
    , _contentLength(other._contentLength)
    , _bodyBytesRead(other._bodyBytesRead)
{
}

Request& Request::operator=(const Request& other)
{
    if (this != &other) {
        _method = other._method;
        _uri = other._uri;
        _path = other._path;
        _queryString = other._queryString;
        _httpVersion = other._httpVersion;
        _body = other._body;
        _headers = other._headers;
        _state = other._state;
        _errorCode = other._errorCode;
        _buffer = other._buffer;
        _contentLength = other._contentLength;
        _bodyBytesRead = other._bodyBytesRead;
    }
    return *this;
}

// ============================================================================
// PUBLIC METHODS
// ============================================================================

/**
 * Reset the request for reuse (important for keep-alive connections)
 * 
 * HTTP/1.1 Keep-Alive:
 * - Single TCP connection can handle multiple requests
 * - After sending response, we reuse the connection
 * - Must reset request state to parse next request
 * - Saves overhead of creating new connections
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

/**
 * Main parsing function - feeds incoming data to the request parser
 * 
 * Why incremental parsing?
 * - HTTP data arrives over network in chunks (non-blocking I/O)
 * - We might receive: "GET / HT" ... then later "TP/1.1\r\n..."
 * - Must buffer partial data and wait for more
 * - Can't use blocking read() - would freeze entire server
 * 
 * @param data: Raw bytes received from socket
 * @return true when request is fully parsed and ready to process
 */
bool Request::parse(const std::string& data)
{
    // Add incoming data to our buffer
    _buffer += data;
    
    // State machine: process based on current parsing stage
    // This allows handling requests that arrive in multiple recv() calls
    if (_state == PARSE_REQUEST_LINE) {
        // Look for end of line: \r\n (CRLF - Carriage Return Line Feed)
        // HTTP/1.1 spec requires CRLF line endings
        size_t pos = _buffer.find("\r\n");
        if (pos == std::string::npos) {
            // Not enough data yet, wait for more
            // Prevent buffer overflow from malicious/broken clients
            if (_buffer.size() > 8192) {  // 8KB limit for request line
                _state = PARSE_ERROR;
                _errorCode = 414;  // URI Too Long
                return true;
            }
            return false;  // Need more data
        }
        
        // Extract request line (without \r\n)
        std::string requestLine = _buffer.substr(0, pos);
        
        // Remove processed data from buffer (including \r\n)
        _buffer.erase(0, pos + 2);
        
        // Parse the request line
        if (!parseRequestLine(requestLine)) {
            _state = PARSE_ERROR;
            return true;  // Error, but parsing is "complete"
        }
        
        // Move to next state: parsing headers
        _state = PARSE_HEADERS;
    }
    
    // TODO: Implement header parsing in Step 4.2
    // TODO: Implement body parsing in Step 4.3
    
    return (_state == PARSE_COMPLETE || _state == PARSE_ERROR);
}

// ============================================================================
// PRIVATE METHODS - REQUEST LINE PARSING
// ============================================================================

/**
 * Parse the HTTP request line: "METHOD URI HTTP/VERSION"
 * 
 * HTTP Request Line Structure (RFC 7230):
 * - METHOD: The action to perform (GET, POST, DELETE, etc.)
 * - URI: The resource identifier (path + optional query string)
 * - HTTP-VERSION: Protocol version (HTTP/1.0 or HTTP/1.1)
 * - Components are separated by single space (SP)
 * 
 * Examples:
 *   "GET /index.html HTTP/1.1"
 *   "POST /api/upload?type=image HTTP/1.1"
 *   "DELETE /files/document.txt HTTP/1.0"
 * 
 * @param line: The first line of HTTP request (without \r\n)
 * @return true if valid, false if malformed (triggers 400 Bad Request)
 */
bool Request::parseRequestLine(const std::string& line)
{
    // Use string stream for easy token extraction
    std::istringstream iss(line);
    
    // Extract method, URI, and version separated by spaces
    if (!(iss >> _method >> _uri >> _httpVersion)) {
        // Parsing failed - missing components
        // Example bad request: "GET" (missing URI and version)
        _errorCode = 400;  // Bad Request
        return false;
    }
    
    // Check if there's extra garbage after the three tokens
    std::string extra;
    if (iss >> extra) {
        // Too many components
        // Example: "GET /index.html HTTP/1.1 EXTRA_STUFF"
        _errorCode = 400;  // Bad Request
        return false;
    }
    
    // ========================================================================
    // VALIDATE HTTP METHOD
    // ========================================================================
    // We must support at least: GET, POST, DELETE (per subject requirements)
    // Optional: HEAD, PUT, PATCH, OPTIONS, CONNECT, TRACE
    if (_method != "GET" && _method != "POST" && _method != "DELETE" &&
        _method != "HEAD" && _method != "PUT") {
        // Method exists but we don't implement it
        _errorCode = 501;  // Not Implemented
        return false;
    }
    
    // ========================================================================
    // VALIDATE URI
    // ========================================================================
    // URI must start with '/' (absolute path) or be '*' (OPTIONS)
    // Example valid URIs:
    //   "/"                    -> root
    //   "/index.html"          -> file path
    //   "/api/users?id=123"    -> path with query string
    if (_uri.empty() || (_uri[0] != '/' && _uri != "*")) {
        _errorCode = 400;  // Bad Request
        return false;
    }
    
    // Limit URI length to prevent attacks
    if (_uri.length() > 2048) {  // Common limit
        _errorCode = 414;  // URI Too Long
        return false;
    }
    
    // ========================================================================
    // PARSE URI INTO PATH AND QUERY STRING
    // ========================================================================
    // URI format: /path/to/resource?key1=value1&key2=value2
    //             |________________| |____________________|
    //                   path              query string
    //
    // Query string is optional, separated by '?'
    size_t queryPos = _uri.find('?');
    if (queryPos != std::string::npos) {
        // URI has a query string
        _path = _uri.substr(0, queryPos);           // Everything before '?'
        _queryString = _uri.substr(queryPos + 1);   // Everything after '?'
    } else {
        // No query string, entire URI is the path
        _path = _uri;
        _queryString = "";
    }
    
    // ========================================================================
    // VALIDATE HTTP VERSION
    // ========================================================================
    // We support HTTP/1.0 and HTTP/1.1 (per subject requirements)
    // Format: "HTTP/X.Y" where X.Y is version number
    if (_httpVersion != "HTTP/1.0" && _httpVersion != "HTTP/1.1") {
        // Examples of invalid versions:
        //   "HTTP/2.0"  -> too new, not supported
        //   "HTTP/0.9"  -> too old
        //   "HTTP"      -> malformed
        //   "http/1.1"  -> wrong case (HTTP is case-sensitive)
        _errorCode = 505;  // HTTP Version Not Supported
        return false;
    }
    
    // ========================================================================
    // SUCCESS!
    // ========================================================================
    // Request line is valid, we can proceed to parse headers
    return true;
}

// TODO: Implement in Step 4.2
bool Request::parseHeader(const std::string& line)
{
    (void)line;
    return false;
}

// TODO: Implement in Step 4.3
bool Request::parseChunkedBody()
{
    return false;
}

// ============================================================================
// GETTERS
// ============================================================================

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
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    // Search through headers (assuming we store them in lowercase)
    for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
         it != _headers.end(); ++it) {
        std::string headerName = it->first;
        std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::tolower);
        
        if (headerName == lowerName) {
            return it->second;
        }
    }
    
    return "";  // Header not found
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
    return (_state == PARSE_ERROR);
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

```
