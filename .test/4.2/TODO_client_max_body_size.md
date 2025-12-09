

dajcs: config is implemented, is it possible to use the real client_max_body_size ?

GitHub Copilot: Yes! Since you have Config implemented, you can definitely use the real `client_max_body_size` from the configuration. However, the `Request` class needs access to the server configuration to check this limit.

Here are two approaches:

## Option 1: Pass max body size to Request (Simpler)

Modify `Request` to accept a max body size parameter:

````cpp
// ...existing code...

class Request {
public:
    // ...existing code...

    Request();
    ~Request();
    Request(const Request& other);
    Request& operator=(const Request& other);

    // Feed raw data to the parser, returns true when request is complete
    bool parse(const std::string& data);

    // Set the maximum allowed body size from config
    void setMaxBodySize(size_t maxSize);

    // ...existing code...

private:
    // ...existing code...
    size_t _contentLength;
    size_t _bodyBytesRead;
    size_t _maxBodySize;  // Add this member

    // ...existing code...
};
````

````cpp
// ...existing code...

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
	_bodyBytesRead(0),
	_maxBodySize(10485760)  // 10MB default
{}

// ...existing code...

void Request::setMaxBodySize(size_t maxSize)
{
	_maxBodySize = maxSize;
}

// ...existing code...

bool Request::parse(const std::string& data)
{
	// ...existing code...

	if (_state == PARSE_HEADERS)
	{
		// ...existing code...

		while (true)
		{
			// ...existing code...

			// Empty line marks end of headers
			if (line.empty())
			{
				// ...existing code...

				std::string contentLength = getHeader("Content-Length");
				std::string transferEncoding = getHeader("Transfer-Encoding");

				if (!contentLength.empty())
				{
					// Content-Length body
					_contentLength = std::atol(contentLength.c_str());

					// Use configured max body size instead of hardcoded value
					if (_contentLength > _maxBodySize)
					{
						_state = PARSE_ERROR;
						_errorCode = 413; // Payload Too Large
						return true;
					}

					// ...existing code...
				}
				// ...existing code...
			}
			// ...existing code...
		}
	}

	// ...existing code...
}
````

**Usage in your connection handler:**

````cpp
// When creating a request for a connection:
Request req;
req.setMaxBodySize(serverConfig.client_max_body_size);
req.parse(data);
````

## Option 2: Pass ServerConfig reference (More flexible)

If you need access to multiple config options:

````cpp
// ...existing code...

#include "Config.hpp"  // Add this include

class Request {
public:
    // ...existing code...

    // Set the server configuration
    void setServerConfig(const ServerConfig* config);

    // ...existing code...

private:
    // ...existing code...
    const ServerConfig* _serverConfig;  // Add this member

    // ...existing code...
};
````

````cpp
// ...existing code...

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
	_bodyBytesRead(0),
	_serverConfig(NULL)  // Initialize to NULL
{}

// ...existing code...

void Request::setServerConfig(const ServerConfig* config)
{
	_serverConfig = config;
}

bool Request::parse(const std::string& data)
{
	// ...existing code...

	if (_state == PARSE_HEADERS)
	{
		while (true)
		{
			// ...existing code...

			if (line.empty())
			{
				// ...existing code...

				if (!contentLength.empty())
				{
					_contentLength = std::atol(contentLength.c_str());

					// Use config value, or default to 10MB if no config
					size_t maxBodySize = _serverConfig ?
						_serverConfig->client_max_body_size : 10485760;

					if (_contentLength > maxBodySize)
					{
						_state = PARSE_ERROR;
						_errorCode = 413; // Payload Too Large
						return true;
					}

					// ...existing code...
				}
				// ...existing code...
			}
			// ...existing code...
		}
	}

	// ...existing code...
}
````

## Recommendation

**Use Option 1** (pass max body size) because:
- Simpler and cleaner
- Request doesn't need to know about the entire config structure
- Better separation of concerns
- Easier to test

The `Request` class should focus on parsing HTTP, not managing configuration. The connection handler (which has access to both config and request) can bridge the two.

**Usage example:**
````cpp
// In your connection/server code:
const ServerConfig* config = /* get config based on host:port */;
Request request;
request.setMaxBodySize(config->client_max_body_size);

// Then parse incoming data:
request.parse(receivedData);
````

This way, the TODO comment in your code is resolved, and you're using the actual configured limit! 🎉
