#include "MockRouter.hpp"
#include <sstream>
#include <ctime>

// ============================================================================
// MockRouter - For testing network layer independently
// ============================================================================
// This mock implementation allows Student 1 (Network Layer) to fully test:
// - Connection handling
// - Request buffering
// - Response writing
// - Keep-alive connections
// - Large response handling
// Without needing the real Router, Config, CGI, or file serving logic.
// ============================================================================

#ifndef MOCKROUTER_HPP
#define MOCKROUTER_HPP

#include "Router.hpp"

class MockRouter : public Router {
public:
    enum MockMode {
        MODE_ECHO,          // Echo back request info
        MODE_STATIC,        // Return static response
        MODE_LARGE,         // Return large response (test chunked writing)
        MODE_SLOW,          // Simulate slow response
        MODE_ERROR          // Always return error
    };

    MockRouter();
    virtual ~MockRouter();

    // Override the main routing method
    virtual Response route(const Request& request, int serverPort);

    // Configure mock behavior
    void setMode(MockMode mode);
    void setStaticResponse(const std::string& body, const std::string& contentType = "text/html");
    void setErrorCode(int code);
    void setLargeResponseSize(size_t bytes);
    
    // Statistics for testing
    size_t getRequestCount() const;
    void resetStats();

private:
    MockMode _mode;
    std::string _staticBody;
    std::string _staticContentType;
    int _errorCode;
    size_t _largeSize;
    size_t _requestCount;

    Response handleEcho(const Request& request, int serverPort);
    Response handleStatic();
    Response handleLarge();
    Response handleError();
};

#endif // MOCKROUTER_HPP

// ============================================================================
// Implementation
// ============================================================================

MockRouter::MockRouter()
    : Router()
    , _mode(MODE_ECHO)
    , _staticBody("<html><body><h1>Mock Response</h1></body></html>")
    , _staticContentType("text/html")
    , _errorCode(500)
    , _largeSize(1024 * 1024)  // 1MB default
    , _requestCount(0)
{
}

MockRouter::~MockRouter()
{
}

Response MockRouter::route(const Request& request, int serverPort)
{
    ++_requestCount;

    // Handle parse errors from request
    if (request.hasError()) {
        return Response::error(request.getErrorCode());
    }

    switch (_mode) {
        case MODE_ECHO:
            return handleEcho(request, serverPort);
        case MODE_STATIC:
            return handleStatic();
        case MODE_LARGE:
            return handleLarge();
        case MODE_ERROR:
            return handleError();
        default:
            return handleEcho(request, serverPort);
    }
}

Response MockRouter::handleEcho(const Request& request, int serverPort)
{
    std::ostringstream html;
    html << "<!DOCTYPE html>\n"
         << "<html><head><title>Mock Echo</title></head>\n"
         << "<body>\n"
         << "<h1>Request Received</h1>\n"
         << "<table border='1'>\n"
         << "<tr><td><b>Method</b></td><td>" << request.getMethod() << "</td></tr>\n"
         << "<tr><td><b>URI</b></td><td>" << request.getUri() << "</td></tr>\n"
         << "<tr><td><b>Path</b></td><td>" << request.getPath() << "</td></tr>\n"
         << "<tr><td><b>Query</b></td><td>" << request.getQueryString() << "</td></tr>\n"
         << "<tr><td><b>HTTP Version</b></td><td>" << request.getHttpVersion() << "</td></tr>\n"
         << "<tr><td><b>Server Port</b></td><td>" << serverPort << "</td></tr>\n"
         << "<tr><td><b>Body Size</b></td><td>" << request.getBodySize() << " bytes</td></tr>\n"
         << "</table>\n"
         << "<h2>Headers</h2>\n"
         << "<table border='1'>\n";

    const std::map<std::string, std::string>& headers = request.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
        html << "<tr><td>" << it->first << "</td><td>" << it->second << "</td></tr>\n";
    }
    html << "</table>\n";

    if (!request.getBody().empty()) {
        html << "<h2>Body</h2>\n"
             << "<pre>" << request.getBody() << "</pre>\n";
    }

    html << "<p><i>Request #" << _requestCount << "</i></p>\n"
         << "</body></html>";

    Response response = Response::ok(html.str(), "text/html");
    
    // Respect Connection header from request
    std::string connection = request.getHeader("Connection");
    if (connection == "close" || request.getHttpVersion() == "HTTP/1.0") {
        response.setConnection(false);
    } else {
        response.setConnection(true);
    }

    return response;
}

Response MockRouter::handleStatic()
{
    return Response::ok(_staticBody, _staticContentType);
}

Response MockRouter::handleLarge()
{
    // Generate a large response to test non-blocking write handling
    std::string body;
    body.reserve(_largeSize);
    
    const char* pattern = "0123456789ABCDEF";
    for (size_t i = 0; i < _largeSize; ++i) {
        body += pattern[i % 16];
    }

    Response response = Response::ok(body, "application/octet-stream");
    response.setHeader("X-Mock-Size", "large");
    return response;
}

Response MockRouter::handleError()
{
    return Response::error(_errorCode);
}

void MockRouter::setMode(MockMode mode)
{
    _mode = mode;
}

void MockRouter::setStaticResponse(const std::string& body, const std::string& contentType)
{
    _staticBody = body;
    _staticContentType = contentType;
}

void MockRouter::setErrorCode(int code)
{
    _errorCode = code;
}

void MockRouter::setLargeResponseSize(size_t bytes)
{
    _largeSize = bytes;
}

size_t MockRouter::getRequestCount() const
{
    return _requestCount;
}

void MockRouter::resetStats()
{
    _requestCount = 0;
}