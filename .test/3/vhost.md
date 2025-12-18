To support virtual hosting (multiple server blocks on the same port with different hostnames), you need to:

1. **Update Config validation** to allow same port with different `server_name`
2. **Update server/connection handling** to route based on `Host` header
3. **Update Router** to find server by hostname, not just port

Here are the required changes:

## 1. Update Config Validation

````cpp
// In validateConfig(), replace the duplicate address check logic:

void Config::validateConfig() const
{
    // Track host:port -> list of server_names for virtual hosting
    std::map<std::pair<std::string, int>, std::set<std::string> > usedAddresses;

    for (size_t i = 0; i < _servers.size(); ++i)
    {
        const ServerConfig& server = _servers[i];

        // Validate port number
        if (server.port <= 0 || server.port > 65535)
        {
            std::ostringstream oss;
            oss << "Invalid port number: " << server.port;
            throw ConfigException(oss.str());
        }

        std::pair<std::string, int> hostPort(server.host, server.port);

        // Get server names (use empty string for default server)
        std::vector<std::string> names = server.server_names;
        if (names.empty())
            names.push_back("");  // Default server marker

        for (size_t j = 0; j < names.size(); ++j)
        {
            if (usedAddresses[hostPort].count(names[j]))
            {
                std::ostringstream oss;
                if (names[j].empty())
                    oss << "Duplicate default server for " << server.host << ":" << server.port;
                else
                    oss << "Duplicate server_name '" << names[j] << "' for "
                        << server.host << ":" << server.port;
                throw ConfigException(oss.str());
            }
            usedAddresses[hostPort].insert(names[j]);
        }

        // ...existing location validation code...
    }
}
````

## 2. Update Router to Find Server by Host Header

````cpp
// ...existing code...

/*
    findServer() - Find server block matching port AND hostname (virtual hosting)

    Virtual hosting allows multiple websites on the same IP:port,
    distinguished by the Host header.

    Example:
        curl --resolve example.com:8080:127.0.0.1 http://example.com/
        -> Host header will be "example.com"
        -> We find the server block with server_name example.com on port 8080
*/
const ServerConfig* Router::findServer(int port, const std::string& hostname) const
{
    if (!_config)
        return NULL;

    const std::vector<ServerConfig>& servers = _config->getServers();
    const ServerConfig* defaultServer = NULL;

    // Extract hostname without port (Host header might be "example.com:8080")
    std::string host = hostname;
    size_t colonPos = host.find(':');
    if (colonPos != std::string::npos)
        host = host.substr(0, colonPos);

    for (size_t i = 0; i < servers.size(); ++i)
    {
        if (servers[i].port != port)
            continue;

        // Check if this server matches the hostname
        const std::vector<std::string>& names = servers[i].server_names;

        for (size_t j = 0; j < names.size(); ++j)
        {
            if (names[j] == host)
                return &servers[i];  // Exact match found
        }

        // Remember first server on this port as default
        if (!defaultServer)
            defaultServer = &servers[i];
    }

    // No exact match - return default server for this port
    return defaultServer;
}

// Update the route() function to use Host header
Response Router::route(const Request& request, int serverPort)
{
    // ...existing code for config check...

    // Get Host header for virtual hosting
    std::string hostHeader = request.getHeader("Host");

    // Find server using both port AND hostname
    const ServerConfig* server = findServer(serverPort, hostHeader);

    if (!server)
    {
        return createErrorResponse(500, NULL);
    }

    // ...rest of existing route() code...
}
````

## 3. Update Router Header

````cpp
// ...existing code...

class Router
{
public:
    // ...existing public methods...

private:
    // Update findServer signature
    const ServerConfig* findServer(int port, const std::string& hostname = "") const;

    // ...rest of existing private members...
};
````

## 4. Test with curl

After implementing, test with:

```bash
# Test first virtual host (localhost)
curl http://localhost:8080/

# Test second virtual host (example.com resolved to 127.0.0.1)
curl --resolve example.com:8080:127.0.0.1 http://example.com:8080/
```

The `--resolve` flag tells curl to resolve `example.com:8080` to `127.0.0.1`, so it connects to your local server but sends `Host: example.com` in the request header.











Looking at the issue, your server is showing 3 configured servers when `youpi.conf` only defines 1 server block. This suggests the configuration parser is loading multiple config files or there's an issue with how the config is being parsed.

Let me investigate the Config parsing logic: 

Searched for text `Configured servers`, 1 result

Searched for text `default.conf`, 20 results

Now let me look at the Config class to see how the servers are loaded: 

Read [](file:///home/anemet/webserv/src/Config.cpp#270-270), lines 270 to 350

Read [](file:///home/anemet/webserv/src/Config.cpp#125-125), lines 125 to 160

Now let me check the main.cpp to see how the config is being initialized: 

Read [](file:///home/anemet/webserv/src/main.cpp#200-200), lines 200 to 280

Found the bug! Look at this section in main.cpp:

```cpp
Config config;  // <-- This calls the DEFAULT constructor, which loads "config/default.conf"!

try
{
    std::cout << "[INFO] Parsing configuration..." << std::endl;
    config.parseFile(configPath);  // <-- This ADDS servers from youpi.conf to the existing ones!
```

The issue is:
1. `Config config;` calls the default constructor, which immediately loads and parses default.conf (see line 133-137 in Config.cpp)
2. Then `config.parseFile(configPath);` parses youpi.conf and **appends** 1 more server to the existing 2 from `default.conf`
3. Result: 3 servers total (2 from `default.conf` + 1 from `youpi.conf`)

The fix is to not call `parseFile()` in the default constructor, or to create the Config object with the correct path from the start.

Here's the fix for main.cpp:

````cpp
// ...existing code...

    // =====================
    //  3. Parse Configuration
    // =====================

    try
    {
        std::cout << "[INFO] Parsing configuration..." << std::endl;
        Config config(configPath);  // Use the parametrized constructor directly

        // Optional: Print parsed config for debugging
        // config.printConfig();

        std::cout << "[INFO] Configuration loaded successfully" << std::endl;
        std::cout << "[INFO] Configured servers: " << config.getServers().size() << std::endl;
// ...existing code...
````

The key change is replacing:
```cpp
Config config;
// ... 
config.parseFile(configPath);
```

with:
```cpp
Config config(configPath);
```

This uses the parametrized constructor which loads only the specified config file once.

**Note:** You'll also need to move the `Config config(configPath);` declaration inside the try block so it's in scope for the rest of the code, or restructure slightly. Let me check what happens after this section: 

Read [](file:///home/anemet/webserv/src/main.cpp#275-275), lines 275 to 342

Since config is used after the try-catch block, here's the complete fix:

````cpp
// ...existing code...

    // =====================
    //  3. Parse Configuration
    // =====================

    Config* configPtr = NULL;

    try
    {
        std::cout << "[INFO] Parsing configuration..." << std::endl;
        configPtr = new Config(configPath);

        // Optional: Print parsed config for debugging
        // configPtr->printConfig();

        std::cout << "[INFO] Configuration loaded successfully" << std::endl;
        std::cout << "[INFO] Configured servers: " << configPtr->getServers().size() << std::endl;

        // Print listening ports
        const std::vector<ServerConfig>& servers = configPtr->getServers();
        for (size_t i = 0; i < servers.size(); ++i)
        {
            std::cout << "[INFO]   - " << servers[i].host << ":" << servers[i].port << std::endl;
        }
    }
    catch (const ConfigException& e)
    {
        std::cerr << "[ERROR] Configuration error: " << e.what() << std::endl;
        delete configPtr;
        return EXIT_FAILURE;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[ERROR] Unexpected error loading config: " << e.what() << std::endl;
        delete configPtr;
        return EXIT_FAILURE;
    }

    // =====================
    //  4. Initialize Server
    // =====================

    Server server(*configPtr);
// ...existing code...
````

And don't forget to `delete configPtr;` at the end before `return EXIT_SUCCESS;`.

**Alternative (simpler) fix:** Modify the default `Config` constructor to NOT parse the file automatically:

````cpp
// ...existing code...
Config::Config() : _configPath("config/default.conf")
{
    // Don't parse here - let main() call parseFile() explicitly
    // parseFile(_configPath);
    // validateConfig();
}
// ...existing code...
````

This way the existing main.cpp code would work, but you'd need to add a `validateConfig()` call after `parseFile()` in main.








---






Read [](file:///home/anemet/webserv/src/Connection.cpp#280-280), lines 280 to 400

Read [](file:///home/anemet/webserv/src/Connection.cpp#540-540), lines 540 to 600

Let me check the reset function: 

Searched for text `void Connection::reset()` (`**/src/Connection.cpp`), 1 result

Read [](file:///home/anemet/webserv/src/Connection.cpp#795-795), lines 795 to 850

The code looks correct. Let me check what happens with the 405 response. Let me run a test manually:

Actually, looking at this more carefully, I notice the test is expecting the server to accept a POST to `/` but return SOMETHING (probably a 405). However, the tester gets an EOF which means the connection drops.

Let me check the `curl` output for POST to `/`: 

Searched for regex `HTTP/1\\.0|keepAlive.*false|close` (`**/src/*.cpp`), 20 results

Wait - I need to think more carefully. Looking at the tester output again:

1. `Test GET http://localhost:8080/` - passes with "content of youpi.bad_extension"
2. `Test POST http://localhost:8080/ with a size of 0` - fails with EOF

The problem might be that the tester is doing these tests sequentially on the SAME connection (keep-alive). After the GET request, the connection is kept alive. Then the POST comes in.

The server might be having trouble with the transition or the response is not being sent properly for the 405.

Let me trace through more carefully:

1. GET request comes in → 200 OK, Connection: keep-alive
2. Server sends response, keeps connection open 
3. POST request comes in on same connection
4. Server should return 405
5. Instead, EOF happens (connection closes)

Let me check if there's something going wrong with processing multiple requests or the keep-alive handling. Let me look at the error response more closely - specifically whether `addStandardHeaders()` is being called: 

Read [](file:///home/anemet/webserv/src/Router.cpp#165-165), lines 165 to 180

I need to see what happens when the 405 error response is generated. Let me verify the Response::error() method is working correctly. But actually, looking at the Response::error(code) method at line 673:

```cpp
Response Response::error(int code)
{
    Response response;
    response.setStatus(code);
    response.setContentType("text/html; charset=UTF-8");
    response.setBody(getDefaultErrorPage(code));
    response.addStandardHeaders();
    return response;
}
```

It calls `addStandardHeaders()` which sets the Connection header. By default `_keepAlive` is `true` in the Response constructor, so this should work.

Let me check if there's something with error responses being treated differently. Let me also check whether there's a logging issue - maybe the server is getting an error or crash: 

Searched for regex `EPOLLERR|EPOLLHUP|error` (`**/src/Server.cpp`), 20 results

Read [](file:///home/anemet/webserv/src/Server.cpp#935-935), lines 935 to 1010

Wait - I see a potential issue. After `processRequest(conn)` sets the response, the connection state changes to `CONN_WRITING`. But we don't immediately check if we need to write - we only write if `EPOLLOUT` is set.

Let me look at how the epoll events are updated: 

Read [](file:///home/anemet/webserv/src/Server.cpp#490-490), lines 490 to 560

The logic seems correct - after `processRequest()`, we call `getNeededEvents()` which returns `EPOLLOUT` when in `CONN_WRITING` state, then `modifyEpoll()` is called to update the events.

Let me check what Connection::getNeededEvents returns:

Looking at line 921-938:
```cpp
uint32_t Connection::getNeededEvents() const
{
	switch (_state)
	{
		case CONN_READING:
			return EPOLLIN;
		case CONN_WRITING:
			return EPOLLOUT;
		case CONN_CLOSED:
		case CONN_ERROR:
		default:
			return 0;
	}
}
```

This looks correct too.

Wait - let me check for a different issue. The test says POST with size 0. Let me check if the request parser is correctly handling `Content-Length: 0` POST requests: 

Read [](file:///home/anemet/webserv/src/Request.cpp#255-255), lines 255 to 285

This looks correct - `Content-Length: 0` correctly sets `_state = PARSE_COMPLETE`.

But wait! The ubuntu_tester might be POSTing with NO Content-Length header at all (not even `Content-Length: 0`). Let me look at what happens when there's no body indicator:

```cpp
else
{
    // No body indicators, request is complete
    // This is normal for GET, DELETE, etc.
    _state = PARSE_COMPLETE;
    return true;
}
```

This should also work - if no Content-Length or Transfer-Encoding is present, the request is considered complete.

Let me check what the Connection::parseRequest does when request is complete but the method is not allowed: 

Read [](file:///home/anemet/webserv/src/Connection.cpp#600-600), lines 600 to 720

I think I see a potential issue. After calling `setResponse()`, the connection's `_keepAlive` is set from the response's `shouldKeepAlive()`. But the response's `_keepAlive` is always `true` by default. 

However, if the Connection already determined `_keepAlive` from the request's HTTP version and headers (in `determineKeepAlive()`), it's possible there's a mismatch.

Actually looking at line 744 of Connection.cpp:
```cpp
_keepAlive = response.shouldKeepAlive();
```

This overwrites the Connection's `_keepAlive` with the Response's value. But the Response's `_keepAlive` is never updated from the Connection's determination!

Let me check if there's code that should sync the keep-alive setting: 

Searched for text `setKeepAlive` (`**/src/Server.cpp`), no results

Searched for text `setKeepAlive` (`**/src/Response.cpp`), no results

Searched for text `setConnection` (`**/src/Response.cpp`), 3 results

OK so the Response's `_keepAlive` defaults to `true`, and `addStandardHeaders()` adds a `Connection: keep-alive` header. This should work for HTTP/1.1 clients.

Let me think about this differently. The EOF error means the connection closed before the response was fully received. Let me check if there are any cases where the connection might close unexpectedly.

Actually, wait - let me re-read the test output:

```
Test GET http://localhost:8080/
content returned: content of youpi.bad_extension

Test POST http://localhost:8080/ with a size of 0
FATAL ERROR ON LAST TEST: Post "http://localhost:8080/": EOF
```

The tester says "Test POST http://localhost:8080/" - this is testing the ROOT `/` location. But the root location only allows GET! So the router should return a 405 error.

But wait - the tester expects the server to just reject the 405 gracefully (it's a test that's expected to fail with a proper error response, not crash). The "EOF" suggests the connection is being closed improperly.

Let me check one more thing - is there any issue when POST with no body is sent to a location that would call handlePost, but the method check happens first?

Actually, looking at Router::route() again:

```cpp
// Step 5: Check HTTP method
std::string method = request.getMethod();
if (!isMethodAllowed(method, *location))
{
    return errorResponse(405, server);
}
```

This should return 405 BEFORE we even get to handlePost. So the issue isn't in handlePost.

Let me check for a crash or issue in errorResponse with server parameter: 

Read [](file:///home/anemet/webserv/src/Router.cpp#1760-1760), lines 1760 to 1792

Hmm, I notice that when a custom error page is found, it doesn't call `addStandardHeaders()` on the response! This might be missing the Connection header.

But that's not the issue here since there's no custom 405 error page configured.

Let me check if Response::error() is being called correctly: 

Read [](file:///home/anemet/webserv/src/Response.cpp#670-670), lines 670 to 685

This looks fine. The issue might be somewhere else. Let me think about this more...

Actually, let me try running the same test with curl to see what happens:

Can you try running:
```bash
curl -v -X POST http://localhost:8080/ -H "Content-Length: 0"
```

And share the output? Also, can you share the server console output when the ubuntu_tester fails?

In the meantime, let me check if there's something wrong with how the Response build works: 

Searched for text `build()` (`**/src/Response.cpp`), 4 results

Read [](file:///home/anemet/webserv/src/Response.cpp#495-495), lines 495 to 580

Read [](file:///home/anemet/webserv/src/Response.cpp#420-420), lines 420 to 495

The Response building looks correct too. 

I wonder if the problem is specific to the ubuntu_tester. Let me look at an edge case - what if the tester closes the connection after getting a response it doesn't like?

Actually, I notice the comment says:
```
# / must answer to GET request ONLY
```

This is from the tester's instructions. The test POST to `/` with size 0 is designed to TEST that the server correctly rejects POST to `/`. The server should return a response (likely 405), not close the connection abruptly.

Let me check if maybe there's something happening with the Keep-Alive on error responses. Perhaps on error, we should be sending `Connection: close`?

Let me also think about whether the ubuntu_tester might be using HTTP/1.0 instead of HTTP/1.1. Let me check if our keep-alive logic handles that correctly:

Looking at Connection::determineKeepAlive():
```cpp
if (httpVersion == "HTTP/1.1")
{
    _keepAlive = (lowerConnection != "close");
}
else
{
    // HTTP/1.0: close by default
    _keepAlive = (lowerConnection == "keep-alive");
}
```

This looks correct. If HTTP/1.0 without `Connection: keep-alive`, the connection's `_keepAlive` is false.

But wait - the Response's `_keepAlive` is always true by default! And in `setResponse()`:
```cpp
_keepAlive = response.shouldKeepAlive();
```

So even if the Connection determined `_keepAlive = false` (HTTP/1.0), the Response overwrites it with `true`!

This could cause the issue: 
1. HTTP/1.0 client sends POST
2. Connection determines `_keepAlive = false`
3. Server generates 405 response with `Connection: keep-alive` (default)
4. Response is sent
5. Client (HTTP/1.0) expects connection to close but server keeps it open
6. Client gets confused or times out, reports EOF

Let me check if there's a mechanism to pass the keep-alive info from Connection/Request to Response: 

Searched for regex `keepAlive.*request|request.*keepAlive` (`**/src/*.cpp`), 1 result

I don't see any mechanism to pass the keep-alive from the request/connection to the response. This is likely the bug!

The fix should be to pass the connection's keep-alive status to the response. Looking at `processRequest` in Server.cpp:

```cpp
void Server::processRequest(Connection& conn)
{
    Request* request = conn.getRequest();
    // ...
    if (_config)
    {
        Router router(*_config);
        Response response = router.route(*request, conn.getServerPort());
        conn.setResponse(response);
    }
}
```

We should set the response's keep-alive based on the connection's keep-alive BEFORE calling `setResponse()`: 

Read [](file:///home/anemet/webserv/src/Server.cpp#1012-1012), lines 1012 to 1055

I can see the problem now. The `processRequest` function doesn't respect the connection's keep-alive setting when building the response.

Let me check if Connection has a getKeepAlive method: 

Searched for text `getKeepAlive` (`**/inc/Connection.hpp`), no results

Searched for text `keepAlive` (`**/inc/Connection.hpp`), 5 results

There's a `shouldKeepAlive()` method. Let me also check if Response has a `setConnection` method that takes a boolean:

Looking at line 294 in Response.cpp:
```cpp
void Response::setConnection(bool keepAlive)
{
    _keepAlive = keepAlive;
    setHeader("Connection", keepAlive ? "keep-alive" : "close");
}
```

Yes! So the fix is to call `response.setConnection(conn.shouldKeepAlive())` before setting the response on the connection.

Here's the fix for Server.cpp:

````cpp
// ...existing code...

void Server::processRequest(Connection& conn)
{
	Request* request = conn.getRequest();

	if (!request)
	{
		std::cerr << "  No request object!" << std::endl;
		Response errorResp = Response::error(500);
		errorResp.setConnection(conn.shouldKeepAlive());  // Respect connection's keep-alive
		conn.setResponse(errorResp);
		return;
	}

	// Set client IP from connection (for CGI REMOTE_ADDR)
	request->setClientIP(conn.getClientIP());

	std::cout << "  Processing: " << request->getMethod() << " "
			  << request->getPath() << std::endl;

	// Check for parsing errors
	if (request->hasError())
	{
		Response errorResp = Response::error(request->getErrorCode());
		errorResp.setConnection(conn.shouldKeepAlive());  // Respect connection's keep-alive
		conn.setResponse(errorResp);
		return;
	}

	// Route the request
	if (_config)
	{
		Router router(*_config);
		Response response = router.route(*request, conn.getServerPort());
		response.setConnection(conn.shouldKeepAlive());  // Respect connection's keep-alive
		conn.setResponse(response);
	}
	else
	{
		// No config - send error
		Response errorResp = Response::error(500);
		errorResp.setConnection(conn.shouldKeepAlive());  // Respect connection's keep-alive
		conn.setResponse(errorResp);
	}
}

// ...existing code...
````

This change ensures that the Response's `Connection` header matches what the client expects based on their HTTP version and `Connection` header.
