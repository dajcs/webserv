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







---
































Looking at the server logs, I can see the issue. The POST request is arriving in many small chunks (107 bytes, 38 bytes, 1 byte, 2 bytes, 1 byte, 3 bytes, 1 byte, 1 byte...), but the request never completes. The server times out after 60 seconds waiting for the complete request.

The problem is in how the request data is being accumulated and parsed. Let me trace through the flow:

1. First request (GET /) works fine - receives 47+47+1 = 95 bytes total
2. Second request (POST /) arrives in fragments but never triggers `hasCompleteRequest()`

The issue is likely in the `Connection::readData()` and `parseRequest()` logic. After the first request completes and the connection resets, the buffer should be clear, but the request parsing might not be working correctly for subsequent requests.

Let me check the Connection code more closely: 

Read [](file:///home/anemet/webserv/src/Connection.cpp#270-270), lines 270 to 400

Read [](file:///home/anemet/webserv/src/Connection.cpp#590-590), lines 590 to 660

Now let me check the Request::parse() function to see what's happening with the POST request: 

Read [](file:///home/anemet/webserv/src/Request.cpp#130-130), lines 130 to 260

Looking at the server logs more carefully:

```
  [Connection fd=5] Received 107 bytes (buffer now: 107 bytes)
  [Connection fd=5] Received 38 bytes (buffer now: 38 bytes)
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
```

Note that the buffer size resets between receives - it goes from 107 to 38 to 1, not accumulating. This means the data is being cleared after each `parseRequest()` call even though the request isn't complete.

Looking at `parseRequest()`:

```cpp
bool complete = _request->parse(_readBuffer);

// Clear the read buffer - the Request has consumed/stored it
_readBuffer.clear();
```

The read buffer is cleared after EACH call to `_request->parse()`, which is correct because the Request is supposed to append the data to its internal `_buffer`. 

But wait - looking at the logs again, the buffer sizes shown are FROM INSIDE `readData()`:
```cpp
std::cout << "  [Connection fd=" << _fd << "] Received "
          << bytesRead << " bytes (buffer now: "
          << _readBuffer.size() << " bytes)" << std::endl;
```

This prints BEFORE `parseRequest()` is called... but the buffer size seems weird. If we're appending to `_readBuffer`, it should grow. Let me re-check:

Actually wait - looking more carefully at `readData()`:

```cpp
_readBuffer.append(buffer, bytesRead);
// ... print ...
if (parseRequest())
```

The print happens AFTER append but BEFORE parseRequest. But the sizes printed (107, 38, 1...) should be cumulative if we're appending... unless the buffer was cleared from the previous call.

OH! I see it now. In `Connection::reset()`:

```cpp
void Connection::reset()
{
    // Clear buffers
    _readBuffer.clear();
    _writeBuffer.clear();
    ...
```

This clears the read buffer. And `reset()` is called after each completed response in `handleWriteComplete()`. So after the GET request completes and the connection is reset for keep-alive, the read buffer is cleared.

BUT the issue is different - looking at the log output again:
```
  [Connection fd=5] Received 107 bytes (buffer now: 107 bytes)
  ...
  [Connection fd=5] Received 38 bytes (buffer now: 38 bytes)
```

If we received 107 bytes first, then 38 bytes, the buffer should show 145 bytes total, not 38. The buffer is being cleared somewhere...

Wait, the issue is that each `parseRequest()` call does `_readBuffer.clear()`. Even if the request isn't complete!

```cpp
bool complete = _request->parse(_readBuffer);
// Clear the read buffer - the Request has consumed/stored it
_readBuffer.clear();  // <-- Always cleared, even if parse returns false!
```

This is correct behavior because `Request::parse()` appends the data to its own internal `_buffer`. The Connection's `_readBuffer` should be cleared since the Request now owns that data. 

So the bytes are actually being accumulated in `Request::_buffer`, not `Connection::_readBuffer`. The logging is just showing what was just received, not the total.

Let me check what `Request::parse()` is returning for the POST request. The issue might be that the request is never reaching a "complete" state: 

Read [](file:///home/anemet/webserv/src/Request.cpp#275-275), lines 275 to 300

I see! When `Content-Length: 0` is encountered:
- `_contentLength = 0`
- `if (_contentLength > 0)` is FALSE
- `else` branch sets `_state = PARSE_COMPLETE`

But wait, there's no `return true;` after setting `_state = PARSE_COMPLETE`! Let me check: 

Read [](file:///home/anemet/webserv/src/Request.cpp#260-260), lines 260 to 310

I see the bug now! When `Content-Length: 0` is set:
1. `_state = PARSE_COMPLETE` is set (line 280)
2. BUT there's NO `return true;` after this!
3. The code hits `break;` (line 297), exits the while loop
4. Falls out of the `if (_state == PARSE_HEADERS)` block
5. Continues to the rest of `parse()` function

Let me check what happens after the PARSE_HEADERS block: 

Read [](file:///home/anemet/webserv/src/Request.cpp#420-420), lines 420 to 450

At the end of `parse()`:

```cpp
// if we reach here, we're in PARSE_COMPLETE or PARSE_ERROR state
return (_state == PARSE_COMPLETE || _state == PARSE_ERROR);
```

So if `_state` was set to `PARSE_COMPLETE` for `Content-Length: 0`, and then we hit `break`, we'd exit the headers while loop, exit the `if (_state == PARSE_HEADERS)` block, and then reach this final return which should return `true`.

But wait - there's also a `PARSE_BODY` check between headers and the final return. Let me see: 

Read [](file:///home/anemet/webserv/src/Request.cpp#305-305), lines 305 to 420

OK, so the flow after setting `_state = PARSE_COMPLETE` with `Content-Length: 0` is:
1. `break` - exits while loop
2. Falls out of `if (_state == PARSE_HEADERS)`
3. `if (_state == PARSE_BODY)` - NO, state is PARSE_COMPLETE
4. `if (_state == PARSE_CHUNKED_BODY)` - NO, state is PARSE_COMPLETE
5. `return (_state == PARSE_COMPLETE || _state == PARSE_ERROR);` - returns TRUE

This should work correctly! Let me think about what else could be wrong...

Looking at the server logs again more carefully:

```
  [Connection fd=5] Received 107 bytes (buffer now: 107 bytes)
...
  [Connection fd=5] Received 38 bytes (buffer now: 38 bytes)
...
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
...
  [Connection fd=5] Received 2 bytes (buffer now: 2 bytes)
```

Wait - each receive shows the buffer size equals exactly the bytes received. This means the `_readBuffer` is NOT accumulating. But why?

Oh wait! I need to re-read `readData()`:

```cpp
_readBuffer.append(buffer, bytesRead);
```

This APPENDS to `_readBuffer`. But the logs show the buffer isn't growing. Let me check if there's something clearing `_readBuffer` that I missed...

Actually, let me trace through more carefully. After the GET request completes:
1. Response is sent
2. `handleWriteComplete()` is called
3. `reset()` is called because of keep-alive
4. `_readBuffer.clear()` and `_request = new Request()` happen

Then for the POST request:
1. First recv: 107 bytes → `_readBuffer.append()` → buffer = 107 bytes
2. `parseRequest()` is called → `_request->parse(_readBuffer)` → `_readBuffer.clear()`
3. Buffer is 0 now
4. Second recv: 38 bytes → `_readBuffer.append()` → buffer = 38 bytes
5. And so on...

This is CORRECT behavior. The `_readBuffer` is supposed to be cleared each time because the `Request::_buffer` takes ownership of the data.

So the real issue is: why isn't the Request parser completing?

Let me look at what the tester is actually sending. The tester says "POST with size 0". Let me check if maybe the request doesn't have a `Content-Length: 0` header...

Actually, looking at the total bytes received: 107 + 38 + 1 + 2 + 1 + 3 + 1 + 1 = 154 bytes. That's a lot for a simple POST with no body!

Let me think about what that POST request might look like:
```
POST / HTTP/1.1\r\n
Host: localhost:8080\r\n
Content-Length: 0\r\n
\r\n
```

That's about 55-60 bytes total. But we're receiving 154+ bytes...

Maybe the tester is using chunked encoding? Or maybe there's something weird about how it's sending the data?

Let me add some debug output to see what's actually in the Request buffer. But first, let me check if there's an issue with how headers are being parsed when the tester sends data in fragments:

Looking at the header parsing loop:
```cpp
while (true)
{
    size_t pos = _buffer.find("\r\n");
    if (pos == std::string::npos)
    {
        // No complete line yet, need more data
        ...
        return false; // Wait for more data
    }
```

This should correctly wait for complete lines. 

Wait - I just realized something. The log shows:
```
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Connection fd=5] Keep-alive: yes (HTTP version: HTTP/1.1)
  [Connection fd=5] Request complete!
```

This was for the GET request. It received 47+47+1 = 95 bytes total and completed successfully.

Then for POST:
```
  [Connection fd=5] Received 107 bytes (buffer now: 107 bytes)
  [Connection fd=5] Received 38 bytes (buffer now: 38 bytes)
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Connection fd=5] Received 2 bytes (buffer now: 2 bytes)
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Connection fd=5] Received 3 bytes (buffer now: 3 bytes)
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  Client fd=5 timed out
```

No "Request complete!" for the POST. So the request is never reaching PARSE_COMPLETE.

Maybe the issue is that the tester is NOT sending `Content-Length: 0` at all, and the parser expects something that never comes?

Actually - wait! I think I see the issue. The tester might be sending data very slowly, byte by byte. And because data arrives in tiny fragments, the parsing might be stuck waiting for the `\r\n` that marks the end of headers... but maybe one of the `\r\n` sequences is being split across recv() calls?

Let me check: if `\r` arrives in one recv() and `\n` arrives in the next:
1. Buffer has `...\r`
2. `_buffer.find("\r\n")` returns npos (not found)
3. Return false, wait for more data
4. Next recv: `\n...`
5. `_buffer.append("\n...")` 
6. BUT WAIT - we do `_readBuffer.clear()` after parse()!

OH! There's the bug!

In `parseRequest()`:
```cpp
bool complete = _request->parse(_readBuffer);
_readBuffer.clear();  // <-- ALWAYS clears, even if not complete!
```

But in `Request::parse()`:
```cpp
_buffer += data;  // Appends to Request's internal buffer
```

So the data IS being accumulated in `Request::_buffer`. But let me double-check: are we properly feeding the buffer in subsequent calls?

Actually, looking at `parseRequest()` again:
```cpp
if (_readBuffer.empty())
{
    return _request->isComplete() || _request->hasError();
}
```

If `_readBuffer` is empty, we don't call `parse()` at all. We just check if it's already complete.

So the flow is:
1. recv() gets 107 bytes
2. `_readBuffer` = 107 bytes
3. `parseRequest()` called
4. `_request->parse(_readBuffer)` - data is appended to `Request::_buffer`
5. `_readBuffer.clear()` - Connection buffer cleared
6. parse() returns false (incomplete)
7. parseRequest() returns false

Next cycle:
1. recv() gets 38 bytes
2. `_readBuffer` = 38 bytes (fresh data)
3. `parseRequest()` called
4. `_request->parse(_readBuffer)` - 38 bytes appended to `Request::_buffer` (now 145 bytes total)
5. etc.

This should be working correctly...

Let me check if maybe the ubuntu_tester is using HTTP/1.0 and we're not handling the lack of `Content-Length` correctly: 

Read [](file:///home/anemet/webserv/src/Request.cpp#283-283), lines 283 to 300

Wait! I see a bug! When we have Content-Length: 0:
1. `_state = PARSE_COMPLETE` is set
2. NO `return true;`!
3. `break;` exits the while loop
4. Falls through to end of the `if (_state == PARSE_HEADERS)` block
5. Then checks `if (_state == PARSE_BODY)` - NO
6. Then checks `if (_state == PARSE_CHUNKED_BODY)` - NO
7. Final return: `return (_state == PARSE_COMPLETE || _state == PARSE_ERROR);`

This should return TRUE... but wait, there's still code between the `break` and the end of the PARSE_HEADERS block!

Let me look more carefully: 

Read [](file:///home/anemet/webserv/src/Request.cpp#295-295), lines 295 to 315

OK so the flow is:
1. After `Content-Length: 0` → `_state = PARSE_COMPLETE` (no return)
2. `break;` exits the `while(true)` loop
3. End of `if (_state == PARSE_HEADERS)` block (line 311)
4. Continues to `if (_state == PARSE_BODY)` (line 315) - NO (state is PARSE_COMPLETE)
5. Continues to `if (_state == PARSE_CHUNKED_BODY)` (line 431) - NO
6. Final return on line 447: `return (_state == PARSE_COMPLETE || _state == PARSE_ERROR);`

This SHOULD return `true` because `_state == PARSE_COMPLETE`.

But wait - let me check if the tester's request even has headers being fully received. Let me look at what happens if the final `\r\n\r\n` (end of headers) is split:

If the request is:
```
POST / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n
```

And it arrives as:
- First chunk: `POST / HTTP/1.1\r\nHost: localhost:8080\r\n\r` (note: \r only)
- Second chunk: `\n` 

Then:
1. First parse: buffer has `...\r`
2. Looking for `\r\n` at end of headers (empty line)
3. The last line parsed is `\r` (just carriage return)
4. ... wait, this shouldn't cause issues because we look for `\r\n` not just `\n`

Hmm, actually the empty line at end of headers should be `\r\n` and that becomes an empty string when extracted.

Let me think about this differently. The tester receives 107+38+1+2+1+3+1+1 = 154 bytes but still doesn't complete. A simple POST with no body should be about 50-70 bytes. What's in those extra bytes?

I wonder if the tester is sending content/body that we're not expecting. Or maybe it's HTTP/1.0 without Content-Length, expecting the server to close the connection to signal end of request?

Actually wait - I just realized something. The tester says "POST with size of 0". For the *server* to interpret "size 0", it either needs:
1. `Content-Length: 0` header
2. No Content-Length and no body

If there's NO Content-Length header and NO Transfer-Encoding header, the current code does:
```cpp
else
{
    // No body indicators, request is complete
    // This is normal for GET, DELETE, etc.
    _state = PARSE_COMPLETE;
    return true;
}
```

This should work! But what if the tester IS sending a body without Content-Length?

Let me check if there's a case where we're stuck waiting. What if Content-Length header exists but we parse it as 0 incorrectly? 

Searched for text `Content-Length` (`**/src/Request.cpp`), 10 results

Let me look at the getHeader function to make sure it's case-insensitive (HTTP headers are case-insensitive): 

Searched for text `getHeader` (`**/src/Request.cpp`), 5 results

Read [](file:///home/anemet/webserv/src/Request.cpp#895-895), lines 895 to 930

Good, header lookup is case-insensitive.

Let me add more debugging. I suspect the tester might be sending chunked encoding without the header, or maybe there's something else going on. Can you add some debug prints to see what's happening?

Actually, let me think about this more. Let me calculate again:
- 107 bytes first chunk
- Then more chunks...

A normal POST request to `/` might look like:
```
POST / HTTP/1.1\r\n
Host: localhost:8080\r\n
Content-Length: 0\r\n
User-Agent: ubuntu_tester\r\n
Accept: */*\r\n
\r\n
```

That's roughly 107 bytes including all headers. So the first recv gets all the headers.

Then why the additional 38+1+2+1+3+1+1 bytes?

OH WAIT! I think I understand now. The ubuntu_tester might be using HTTP/1.0 or sending `Connection: close`. After the GET request completes with keep-alive, the tester might be REUSING the connection but sending the next request slowly/incorrectly, OR it might have closed and reopened.

Actually, looking at the log again:
```
  [Connection fd=5] Keep-alive: waiting for next request
  [Connection fd=5] Reset for next request
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN 
  [Connection fd=5] Received 107 bytes (buffer now: 107 bytes)
```

The connection IS being reused (same fd=5). So the tester sends the POST on the same connection.

Let me think... 107 bytes is received. Then the request parser runs. But the "Request complete!" message never appears.

Let me look at what happens INSIDE the Request::parse() when 107 bytes arrive. Let me trace through:

1. `_buffer += data` - _buffer now has 107 bytes
2. `if (_state == PARSE_REQUEST_LINE)` - YES (initial state)
3. `_buffer.find("\r\n")` - should find end of "POST / HTTP/1.1"
4. Parses request line successfully
5. `_state = PARSE_HEADERS`
6. Falls into `if (_state == PARSE_HEADERS)` block
7. Loops parsing headers...

Wait - after parsing the request line and setting `_state = PARSE_HEADERS`, does the code fall through to the headers block, or does it return? 

Read [](file:///home/anemet/webserv/src/Request.cpp#145-145), lines 145 to 195

I see it! After parsing the request line, the code sets `_state = PARSE_HEADERS` and then... FALLS THROUGH to the next `if (_state == PARSE_HEADERS)` block! This is correct behavior - it continues parsing headers in the same call.

So with 107 bytes, it should:
1. Parse request line "POST / HTTP/1.1"
2. Set state to PARSE_HEADERS
3. Fall through to headers parsing
4. Parse all headers until the empty line
5. Set state to PARSE_COMPLETE (if no body)
6. Return true

But "Request complete!" is never logged. So either:
- The headers aren't complete in those 107 bytes
- Something is going wrong in header parsing

Let me count a typical POST request's size:
```
POST / HTTP/1.1\r\n                  = 18 bytes
Host: localhost:8080\r\n             = 22 bytes
Content-Length: 0\r\n                = 19 bytes
User-Agent: ubuntu_tester/1.0\r\n    = 30 bytes
Accept: */*\r\n                      = 12 bytes
\r\n                                 = 2 bytes
                                     = 103 bytes total
```

So 107 bytes SHOULD contain the complete request with empty body!

Maybe there's an issue with the Go HTTP client (ubuntu_tester is likely written in Go) sending headers that we're not parsing correctly?

Let me check if maybe the Host header is missing or has a different format:

Actually, I wonder if the problem is with the `Connection` header. Let me check what the tester sends:

Can you add a debug print in `Request::parseHeader()` to see what headers are being received? Add this at the start of the function: 

Read [](file:///home/anemet/webserv/src/Request.cpp#600-600), lines 600 to 670

Wait! I see a potential issue. Headers are stored with LOWERCASE names:

```cpp
std::string lowerName = name;
std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
_headers[lowerName] = value;
```

So headers are stored as "content-length", "host", etc.

But in `getHeader()`:
```cpp
std::string lowerName = name;
std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
...
for (...) {
    std::string headerName = it->first;  // Already lowercase!
    std::transform(headerName.begin(), headerName.end(), headerName.begin(), ::tolower);
    if (headerName == lowerName) { return it->second; }
}
```

This is a bit redundant (double lowercase), but should work correctly.

Let me look for another issue. Let me check `parseRequestLine`: 

Read [](file:///home/anemet/webserv/src/Request.cpp#475-475), lines 475 to 580

This all looks correct.

Actually, let me add some focused debugging. The simplest fix would be to add logging to see exactly what's happening. But since I can't run the code, let me try a different approach.

Let me check if maybe there's a bug in how we handle the complete request check: 

Searched for text `hasCompleteRequest` (`**/src/Connection.cpp`), 2 results

Read [](file:///home/anemet/webserv/src/Connection.cpp#755-755), lines 755 to 770

Searched for text `isComplete` (`**/src/Request.cpp`), 1 result

Read [](file:///home/anemet/webserv/src/Request.cpp#935-935), lines 935 to 955

Everything looks correct. Let me think about this from a different angle.

The server logs show "Request complete!" for the GET request but NOT for the POST. And the POST data arrives in multiple small chunks over time until the connection times out.

One possible explanation: The ubuntu_tester might be reading the response before finishing writing the request. If the tester:
1. Sends 107 bytes of POST request
2. Waits for something (maybe reading the socket)
3. Server processes but request isn't complete?
4. Tester eventually sends more data in tiny chunks
5. Times out

But that doesn't explain why the request wouldn't complete after 107 bytes...

Wait! I just thought of something. Let me check the log more carefully:

```
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN 
  [Connection fd=5] Received 107 bytes (buffer now: 107 bytes)
```

There's no "Request complete!" after these 107 bytes. So `parseRequest()` returned `false`, meaning the Request::parse() returned `false`.

But 107 bytes should be enough for a complete POST with no body. Unless... the parser thinks there's a body!

Let me check: maybe the tester is sending `Transfer-Encoding: chunked` without actually sending chunked data?

If the headers include `Transfer-Encoding: chunked`, then:
```cpp
else if (!transferEncoding.empty() &&
            transferEncoding.find("chunked") != std::string::npos)
{
    // Chunked transfer encoding
    _state = PARSE_CHUNKED_BODY;
}
```

And then the parser would wait for chunked body data that never comes!

**I think this is the bug!** The Go HTTP client (ubuntu_tester) might be sending `Transfer-Encoding: chunked` even for a zero-length body, and the parser waits for chunk data that never arrives.

Let me look at the chunked body parsing: 

Read [](file:///home/anemet/webserv/src/Request.cpp#690-690), lines 690 to 860

I see! For chunked encoding, even an empty body must send `0\r\n\r\n` (the zero-sized final chunk). If the Go client sends `Transfer-Encoding: chunked` but doesn't send the terminating `0\r\n\r\n`, the parser will wait forever.

But looking at the data sizes (107 bytes, then 38, 1, 2, 1, 3, 1, 1), those small fragments after the initial headers could be the Go client sending chunk termination bytes slowly, one at a time!

Actually, let me count: the termination for an empty chunked body is `0\r\n\r\n` = 5 bytes. But we're receiving 38+1+2+1+3+1+1 = 47 bytes more after the initial 107 bytes. That's way more than needed.

Wait, I think I need to add debug logging to see what's actually in the request buffer. 

For now, let me propose a quick diagnostic fix - add some logging to see what's happening:

````cpp
// In the parse() function, add debug logging:

bool Request::parse(const std::string& data)
{
    // Add incoming data to the buffer
    _buffer += data;
    
    // DEBUG: Print buffer contents (first 200 chars)
    std::cout << "  [Request] Buffer size: " << _buffer.size() 
              << ", State: " << _state << std::endl;
    if (_buffer.size() > 0 && _buffer.size() <= 200) {
        std::cout << "  [Request] Buffer: [";
        for (size_t i = 0; i < _buffer.size(); i++) {
            char c = _buffer[i];
            if (c == '\r') std::cout << "\\r";
            else if (c == '\n') std::cout << "\\n";
            else if (c >= 32 && c < 127) std::cout << c;
            else std::cout << "\\x" << std::hex << (int)(unsigned char)c << std::dec;
        }
        std::cout << "]" << std::endl;
    }
    
    // ...existing code...
````

Can you add this debug logging and run the tester again? This will show us exactly what's being received and what state the parser is in.

Alternatively, if you want a quick fix to test, try adding `Content-Length: 0` handling at a higher priority. But first, let's understand what's actually being sent.
