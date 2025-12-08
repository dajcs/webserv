#ifndef MOCKROUTER_HPP
#define MOCKROUTER_HPP

#include "Router.hpp"

class MockRouter : public Router {
public:
    enum MockMode {
        MODE_ECHO,          // Echo back request info
        MODE_STATIC,        // Return static response
        MODE_LARGE,         // Return large response (test chunked writing)
        MODE_ERROR          // Always return error
    };

    MockRouter();
    virtual ~MockRouter();

    virtual Response route(const Request& request, int serverPort);

    void setMode(MockMode mode);
    void setStaticResponse(const std::string& body, const std::string& contentType = "text/html");
    void setErrorCode(int code);
    void setLargeResponseSize(size_t bytes);

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

#endif
