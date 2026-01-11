# Signal Flow: HTTP Request Processing

Here's the complete signal flow when a client sends `curl -v http://localhost:8080/`:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         CLIENT CONNECTION PHASE                             │
└─────────────────────────────────────────────────────────────────────────────┘

curl sends: TCP SYN to localhost:8080
    │
    ▼
Kernel: Completes TCP 3-way handshake, adds connection to listen backlog
    │
    ▼
epoll_wait(4, events, 64, 1000)
    │
    └──► Returns: 1
         events[0] = {events=EPOLLIN, data.fd=3}  // Listening socket ready
    │
    ▼
isListenSocket(3) → true
    │
    ▼
acceptNewConnection(listenFd=3)
    │
    ├──► accept(3, &clientAddr, &clientLen)
    │        │
    │        └──► Returns: 5 (new client socket fd)
    │             clientAddr = {sin_family=AF_INET,
    │                          sin_port=htons(54321),  // ephemeral port
    │                          sin_addr=127.0.0.1}
    │
    ├──► getListenSocketByFd(3)
    │        └──► Returns: &ListenSocket{fd=3, host="0.0.0.0", port=8080}
    │
    ├──► setNonBlocking(5)
    │        ├──► fcntl(5, F_GETFL, 0) → flags
    │        └──► fcntl(5, F_SETFL, flags | O_NONBLOCK) → 0
    │
    ├──► addToEpoll(5, EPOLLIN)
    │        └──► epoll_ctl(4, EPOLL_CTL_ADD, 5, {events=EPOLLIN, data.fd=5})
    │
    ├──► Connection conn(5, clientAddr, serverPort=8080)
    │        │
    │        ├──► inet_ntop(AF_INET, &clientAddr.sin_addr, ipBuffer, 16)
    │        │        └──► ipBuffer = "127.0.0.1"
    │        ├──► _clientIP = "127.0.0.1"
    │        ├──► _clientPort = 54321
    │        ├──► _serverPort = 8080
    │        ├──► _connectTime = time(NULL)
    │        ├──► _lastActivity = _connectTime
    │        ├──► _state = CONN_READING
    │        ├──► _keepAlive = true
    │        └──► _request = new Request()
    │
    ├──► _connections[5] = conn
    │
    └──► Returns: 5


┌─────────────────────────────────────────────────────────────────────────────┐
│                         REQUEST READING PHASE                               │
└─────────────────────────────────────────────────────────────────────────────┘

curl sends HTTP request:
    "GET / HTTP/1.1\r\n"
    "Host: localhost:8080\r\n"
    "User-Agent: curl/7.68.0\r\n"
    "Accept: */*\r\n"
    "\r\n"
    │
    ▼
epoll_wait(4, events, 64, 1000)
    │
    └──► Returns: 1
         events[0] = {events=EPOLLIN, data.fd=5}  // Client socket readable
    │
    ▼
isListenSocket(5) → false
    │
    ▼
handleClientEvent(clientFd=5, events=EPOLLIN)
    │
    ├──► _connections.find(5) → iterator to Connection
    │
    ├──► Check EPOLLERR | EPOLLHUP → false (no errors)
    │
    └──► conn.readData()
             │
             ├──► recv(5, buffer[8192], 8192, 0)
             │        │
             │        └──► Returns: 78 (bytes received)
             │             buffer = "GET / HTTP/1.1\r\nHost: localhost:8080\r\n
             │                       User-Agent: curl/7.68.0\r\nAccept: */*\r\n\r\n"
             │
             ├──► _readBuffer.append(buffer, 78)
             │        └──► _readBuffer = "GET / HTTP/1.1\r\n..."
             │
             ├──► updateActivity()
             │        └──► _lastActivity = time(NULL)
             │
             └──► parseRequest()
                      │
                      └──► _request->parse(_readBuffer)
                               │
                               │  ┌─────────────────────────────────────────┐
                               │  │     REQUEST PARSING STATE MACHINE       │
                               │  └─────────────────────────────────────────┘
                               │
                               ├──► _state == PARSE_REQUEST_LINE
                               │        │
                               │        ├──► _buffer.find("\r\n") → pos=14
                               │        │
                               │        ├──► requestLine = "GET / HTTP/1.1"
                               │        │
                               │        ├──► _buffer.erase(0, 16)  // Remove line + \r\n
                               │        │
                               │        └──► parseRequestLine("GET / HTTP/1.1")
                               │                 │
                               │                 ├──► stringstream ss(line)
                               │                 ├──► ss >> _method >> _uri >> _httpVersion
                               │                 │        _method = "GET"
                               │                 │        _uri = "/"
                               │                 │        _httpVersion = "HTTP/1.1"
                               │                 │
                               │                 ├──► Validate method: "GET" ✓
                               │                 ├──► Validate URI: "/" starts with '/' ✓
                               │                 │
                               │                 ├──► Parse URI into path and query:
                               │                 │        _uri.find('?') → npos
                               │                 │        _path = "/"
                               │                 │        _queryString = ""
                               │                 │
                               │                 ├──► Validate HTTP version: "HTTP/1.1" ✓
                               │                 │
                               │                 └──► Returns: true
                               │
                               ├──► _state = PARSE_HEADERS
                               │
                               ├──► Parse headers loop:
                               │        │
                               │        ├──► Line: "Host: localhost:8080"
                               │        │        └──► parseHeader() → _headers["host"] = "localhost:8080"
                               │        │
                               │        ├──► Line: "User-Agent: curl/7.68.0"
                               │        │        └──► parseHeader() → _headers["user-agent"] = "curl/7.68.0"
                               │        │
                               │        ├──► Line: "Accept: */*"
                               │        │        └──► parseHeader() → _headers["accept"] = "*/*"
                               │        │
                               │        └──► Line: "" (empty = end of headers)
                               │                 │
                               │                 ├──► HTTP/1.1 requires Host ✓
                               │                 ├──► No Content-Length header
                               │                 ├──► No Transfer-Encoding header
                               │                 └──► _state = PARSE_COMPLETE (no body for GET)
                               │
                               └──► Returns: true (request complete)
                      │
                      ├──► _readBuffer.clear()
                      │
                      ├──► determineKeepAlive()
                      │        │
                      │        ├──► _request->getHttpVersion() → "HTTP/1.1"
                      │        ├──► _request->getHeader("Connection") → ""
                      │        └──► HTTP/1.1 + no "close" header → _keepAlive = true
                      │
                      └──► Returns: true


┌─────────────────────────────────────────────────────────────────────────────┐
│                       REQUEST PROCESSING PHASE                              │
└─────────────────────────────────────────────────────────────────────────────┘

conn.hasCompleteRequest() → true
    │
    ▼
processRequest(conn)
    │
    ├──► request = conn.getRequest()
    │
    ├──► request->setClientIP("127.0.0.1")
    │
    ├──► request->hasError() → false
    │
    └──► Router router(*_config)
         │
         └──► router.route(request, serverPort=8080)
                  │
                  ├──► _config != NULL ✓
                  │
                  ├──► hostHeader = request.getHeader("Host")
                  │        └──► "localhost:8080"
                  │
                  ├──► findServer(port=8080, hostname="localhost:8080")
                  │        │
                  │        ├──► host = "localhost" (strip port)
                  │        │
                  │        ├──► For each server in _config->getServers():
                  │        │        │
                  │        │        ├──► servers[0]: port=8080, server_names=["localhost"]
                  │        │        │        └──► "localhost" == "localhost" → MATCH!
                  │        │        │
                  │        │        └──► Returns: &servers[0]
                  │        │
                  │        └──► Returns: ServerConfig* (port 8080, localhost)
                  │
                  ├──► requestPath = request.getPath() → "/"
                  ├──► method = request.getMethod() → "GET"
                  │
                  ├──► findLocation(server, path="/", method="GET")
                  │        │
                  │        ├──► For each location in server.locations:
                  │        │        │
                  │        │        ├──► location "/" :
                  │        │        │        path="/" matches "/"
                  │        │        │        matchLength = 1
                  │        │        │        bestMatch = &locations[0]
                  │        │        │
                  │        │        ├──► location "/files":
                  │        │        │        "/" does not start with "/files"
                  │        │        │
                  │        │        └──► ... (other locations don't match)
                  │        │
                  │        └──► Returns: &LocationConfig{path="/", root="www",
                  │                                      index="index.html",
                  │                                      methods={GET,POST}}
                  │
                  ├──► location->redirect_url.empty() → true (no redirect)
                  │
                  ├──► isMethodAllowed("GET", location)
                  │        │
                  │        └──► location.allowed_methods.count("GET") > 0 → true
                  │
                  ├──► resolvePath("/", location)
                  │        │
                  │        ├──► root = "www"
                  │        ├──► locationPath = "/"
                  │        ├──► remainder = "/"  (requestPath == locationPath)
                  │        │
                  │        └──► Returns: "www/"
                  │
                  ├──► isCgiRequest("www/", location) → false (no cgi_extension)
                  │
                  ├──► requestPath != "/login", "/dashboard", "/logout"
                  │
                  ├──► method == "GET" → true
                  │
                  └──► handleGet(request, location)
                           │
                           ├──► path = resolvePath("/", location) → "www/"
                           │
                           ├──► stat("www/", &pathStat)
                           │        └──► Returns: 0 (success)
                           │             pathStat.st_mode = S_IFDIR | 0755
                           │
                           ├──► S_ISDIR(pathStat.st_mode) → true
                           │
                           └──► serveDirectory("www/", location)
                                    │
                                    ├──► location.index = "index.html" (not empty)
                                    │
                                    ├──► indexPath = "www/" + "index.html" = "www/index.html"
                                    │
                                    ├──► stat("www/index.html", &indexStat)
                                    │        └──► Returns: 0 (file exists)
                                    │
                                    ├──► S_ISREG(indexStat.st_mode) → true
                                    │
                                    └──► serveFile("www/index.html")
                                             │
                                             ├──► ifstream file("www/index.html", ios::binary)
                                             │        └──► Opens successfully
                                             │
                                             ├──► contents << file.rdbuf()
                                             │        └──► Reads entire file into stringstream
                                             │
                                             ├──► file.close()
                                             │
                                             ├──► extension = ".html"
                                             │
                                             ├──► getMimeTypeForFile("www/index.html")
                                             │        │
                                             │        └──► getMimeType(".html")
                                             │                 └──► Returns: "text/html; charset=UTF-8"
                                             │
                                             ├──► Response response
                                             │        response.setStatus(200, "OK")
                                             │        response.setContentType("text/html; charset=UTF-8")
                                             │        response.setContentLength(body.size())
                                             │        response.setBody(contents.str())
                                             │        response.addStandardHeaders()
                                             │            ├──► addDateHeader()
                                             │            │        └──► formatHttpDate(time(NULL))
                                             │            │                 └──► "Sat, 11 Jan 2026 12:00:00 GMT"
                                             │            ├──► addServerHeader()
                                             │            │        └──► "webserv/1.0"
                                             │            └──► setConnection(true)
                                             │                     └──► "keep-alive"
                                             │
                                             └──► Returns: Response


┌─────────────────────────────────────────────────────────────────────────────┐
│                        RESPONSE SENDING PHASE                               │
└─────────────────────────────────────────────────────────────────────────────┘

conn.setResponse(response)
    │
    ├──► response.build()
    │        │
    │        └──► buildIfNeeded()
    │                 │
    │                 ├──► _dirty == true → build response
    │                 │
    │                 ├──► ss << "HTTP/1.1 200 OK\r\n"
    │                 │
    │                 ├──► For each header:
    │                 │        ss << "Date: Sat, 11 Jan 2026 12:00:00 GMT\r\n"
    │                 │        ss << "Server: webserv/1.0\r\n"
    │                 │        ss << "Content-Type: text/html; charset=UTF-8\r\n"
    │                 │        ss << "Content-Length: 1234\r\n"
    │                 │        ss << "Connection: keep-alive\r\n"
    │                 │
    │                 ├──► ss << "\r\n"  // End of headers
    │                 │
    │                 ├──► ss << _body   // HTML content
    │                 │
    │                 └──► _builtResponse = ss.str()
    │
    ├──► _writeBuffer = response.build()  // Full HTTP response
    ├──► _writeOffset = 0
    ├──► _state = CONN_WRITING
    │
    └──► _keepAlive = response.shouldKeepAlive() → true
    │
    ▼
conn.getNeededEvents()
    │
    └──► _state == CONN_WRITING → return EPOLLOUT
    │
    ▼
modifyEpoll(5, EPOLLOUT)
    │
    └──► epoll_ctl(4, EPOLL_CTL_MOD, 5, {events=EPOLLOUT, data.fd=5})
    │
    ▼
Returns: true (keep connection)


┌─────────────────────────────────────────────────────────────────────────────┐
│                         WRITE DATA PHASE                                    │
└─────────────────────────────────────────────────────────────────────────────┘

epoll_wait(4, events, 64, 1000)
    │
    └──► Returns: 1
         events[0] = {events=EPOLLOUT, data.fd=5}  // Client socket writable
    │
    ▼
handleClientEvent(clientFd=5, events=EPOLLOUT)
    │
    ├──► conn.hasDataToWrite() → true
    │
    └──► conn.writeData()
             │
             ├──► remaining = _writeBuffer.size() - _writeOffset
             │        └──► e.g., 1500 bytes
             │
             ├──► dataPtr = _writeBuffer.c_str() + 0
             │
             ├──► send(5, dataPtr, 1500, MSG_NOSIGNAL)
             │        │
             │        └──► Returns: 1500 (all bytes sent)
             │
             ├──► _writeOffset += 1500
             │
             ├──► updateActivity()
             │
             ├──► _writeOffset >= _writeBuffer.size() → true
             │
             └──► handleWriteComplete()
                      │
                      ├──► _keepAlive == true
                      │
                      └──► conn.reset()
                               │
                               ├──► _readBuffer.clear()
                               ├──► _writeBuffer.clear()
                               ├──► _writeOffset = 0
                               │
                               ├──► delete _request
                               ├──► _request = new Request()
                               │
                               ├──► _state = CONN_READING
                               ├──► _keepAlive = true
                               ├──► _maxBodySizeUpdated = false
                               │
                               └──► updateActivity()
             │
             └──► Returns: true
    │
    ▼
conn.getNeededEvents()
    │
    └──► _state == CONN_READING → return EPOLLIN
    │
    ▼
modifyEpoll(5, EPOLLIN)
    │
    └──► epoll_ctl(4, EPOLL_CTL_MOD, 5, {events=EPOLLIN, data.fd=5})


┌─────────────────────────────────────────────────────────────────────────────┐
│                      CONNECTION CLOSE (curl exits)                          │
└─────────────────────────────────────────────────────────────────────────────┘

curl receives response, closes connection
    │
    ▼
epoll_wait(4, events, 64, 1000)
    │
    └──► Returns: 1
         events[0] = {events=EPOLLIN|EPOLLHUP, data.fd=5}
         (or just EPOLLIN with recv() returning 0)
    │
    ▼
handleClientEvent(clientFd=5, events=EPOLLIN|EPOLLHUP)
    │
    ├──► events & EPOLLHUP → true
    │        OR
    ├──► conn.readData()
    │        └──► recv(5, buffer, 8192, 0) → 0 (EOF)
    │             └──► Returns: false
    │
    └──► Returns: false (close connection)
    │
    ▼
closeClientConnection(5)
    │
    ├──► removeFromEpoll(5)
    │        └──► epoll_ctl(4, EPOLL_CTL_DEL, 5, &ev)
    │
    ├──► close(5)
    │
    └──► _connections.erase(5)


┌─────────────────────────────────────────────────────────────────────────────┐
│                         BACK TO WAITING                                     │
└─────────────────────────────────────────────────────────────────────────────┘

epoll_wait(4, events, 64, 1000)
    │
    └──► Returns: 0 (timeout, no events)
         │
         └──► Loop continues, waiting for next connection...
```

## Summary: System Calls in Order

| Phase | System Call | Parameters | Returns |
|-------|-------------|------------|---------|
| **Accept** | `epoll_wait` | `(4, events, 64, 1000)` | 1 |
| | `accept` | `(3, &addr, &len)` | 5 |
| | `fcntl` | `(5, F_GETFL)` | flags |
| | `fcntl` | `(5, F_SETFL, O_NONBLOCK)` | 0 |
| | `epoll_ctl` | `(4, ADD, 5, EPOLLIN)` | 0 |
| **Read** | `epoll_wait` | `(4, events, 64, 1000)` | 1 |
| | `recv` | `(5, buf, 8192, 0)` | 78 |
| **Route** | `stat` | `("www/", &st)` | 0 |
| | `stat` | `("www/index.html", &st)` | 0 |
| | `open` | `("www/index.html")` | 6 |
| | `read` | `(6, ...)` | content |
| | `close` | `(6)` | 0 |
| | `time` | `(NULL)` | timestamp |
| **Write** | `epoll_ctl` | `(4, MOD, 5, EPOLLOUT)` | 0 |
| | `epoll_wait` | `(4, events, 64, 1000)` | 1 |
| | `send` | `(5, response, len, MSG_NOSIGNAL)` | len |
| | `epoll_ctl` | `(4, MOD, 5, EPOLLIN)` | 0 |
| **Close** | `epoll_wait` | `(4, events, 64, 1000)` | 1 |
| | `recv` | `(5, buf, 8192, 0)` | 0 (EOF) |
| | `epoll_ctl` | `(4, DEL, 5, &ev)` | 0 |
| | `close` | `(5)` | 0 |
