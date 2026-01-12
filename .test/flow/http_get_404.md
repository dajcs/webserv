# Signal Flow: 404 Not Found Request

Here's the complete signal flow when a client requests a non-existent file with `curl -v http://localhost:8080/nonexistent.html`:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         CLIENT CONNECTION PHASE                             │
│                        (Same as successful request)                         │
└─────────────────────────────────────────────────────────────────────────────┘

epoll_wait(4, events, 64, 1000)
    │
    └──► Returns: 1
         events[0] = {events=EPOLLIN, data.fd=3}
    │
    ▼
acceptNewConnection(listenFd=3)
    │
    ├──► accept(3, &clientAddr, &clientLen) → fd=5
    ├──► fcntl(5, F_GETFL) → flags
    ├──► fcntl(5, F_SETFL, flags | O_NONBLOCK) → 0
    ├──► epoll_ctl(4, EPOLL_CTL_ADD, 5, {EPOLLIN, fd=5}) → 0
    └──► Connection conn(5, clientAddr, 8080)


┌─────────────────────────────────────────────────────────────────────────────┐
│                         REQUEST READING PHASE                               │
└─────────────────────────────────────────────────────────────────────────────┘

curl sends:
    "GET /nonexistent.html HTTP/1.1\r\n"
    "Host: localhost:8080\r\n"
    "User-Agent: curl/7.68.0\r\n"
    "Accept: */*\r\n"
    "\r\n"
    │
    ▼
epoll_wait(4, events, 64, 1000)
    │
    └──► Returns: 1
         events[0] = {events=EPOLLIN, data.fd=5}
    │
    ▼
handleClientEvent(clientFd=5, events=EPOLLIN)
    │
    └──► conn.readData()
             │
             ├──► recv(5, buffer[8192], 8192, 0)
             │        └──► Returns: 92 bytes
             │
             └──► parseRequest()
                      │
                      └──► _request->parse(_readBuffer)
                               │
                               ├──► parseRequestLine("GET /nonexistent.html HTTP/1.1")
                               │        │
                               │        ├──► _method = "GET"
                               │        ├──► _uri = "/nonexistent.html"
                               │        ├──► _path = "/nonexistent.html"
                               │        ├──► _queryString = ""
                               │        └──► _httpVersion = "HTTP/1.1"
                               │
                               ├──► Parse headers:
                               │        _headers["host"] = "localhost:8080"
                               │        _headers["user-agent"] = "curl/7.68.0"
                               │        _headers["accept"] = "*/*"
                               │
                               └──► _state = PARSE_COMPLETE (no body for GET)


┌─────────────────────────────────────────────────────────────────────────────┐
│                      REQUEST PROCESSING PHASE                               │
│                    *** THIS IS WHERE 404 HAPPENS ***                        │
└─────────────────────────────────────────────────────────────────────────────┘

processRequest(conn)
    │
    ├──► request = conn.getRequest()
    │
    ├──► request->setClientIP("127.0.0.1")
    │
    ├──► request->hasError() → false (request parsing succeeded)
    │
    └──► Router router(*_config)
         │
         └──► router.route(request, serverPort=8080)
                  │
                  ├──► hostHeader = request.getHeader("Host")
                  │        └──► "localhost:8080"
                  │
                  ├──► findServer(port=8080, hostname="localhost:8080")
                  │        │
                  │        └──► Returns: &servers[0]  // localhost server
                  │
                  ├──► requestPath = request.getPath()
                  │        └──► "/nonexistent.html"
                  │
                  ├──► method = request.getMethod()
                  │        └──► "GET"
                  │
                  ├──► findLocation(server, path="/nonexistent.html", method="GET")
                  │        │
                  │        ├──► Check location "/" :
                  │        │        "/nonexistent.html".compare(0, 1, "/") == 0 ✓
                  │        │        matchLength = 1
                  │        │        bestMatch = &locations[0]  // root location
                  │        │
                  │        ├──► Check location "/files":
                  │        │        "/nonexistent.html" does not start with "/files"
                  │        │
                  │        ├──► Check location "/uploads":
                  │        │        "/nonexistent.html" does not start with "/uploads"
                  │        │
                  │        └──► Returns: &LocationConfig{
                  │                        path="/",
                  │                        root="www",
                  │                        index="index.html",
                  │                        methods={GET, POST}
                  │                      }
                  │
                  ├──► location->redirect_url.empty() → true (no redirect)
                  │
                  ├──► isMethodAllowed("GET", location)
                  │        │
                  │        └──► location.allowed_methods.count("GET") > 0 → true ✓
                  │
                  ├──► resolvePath("/nonexistent.html", location)
                  │        │
                  │        ├──► root = "www"
                  │        ├──► locationPath = "/"
                  │        ├──► remainder = "/nonexistent.html"
                  │        │
                  │        └──► Returns: "www/nonexistent.html"
                  │
                  ├──► isCgiRequest("www/nonexistent.html", location)
                  │        │
                  │        └──► false (no cgi_extension configured for "/")
                  │
                  ├──► requestPath != "/login", "/dashboard", "/logout"
                  │
                  ├──► method == "GET" → true
                  │
                  └──► handleGet(request, location)
                           │
                           ├──► path = resolvePath("/nonexistent.html", location)
                           │        └──► "www/nonexistent.html"
                           │
                           ├──► stat("www/nonexistent.html", &pathStat)
                           │        │
                           │        └──► Returns: -1 (ENOENT - file not found!)
                           │             errno = ENOENT (No such file or directory)
                           │
                           │    ┌─────────────────────────────────────────────┐
                           │    │        FILE NOT FOUND - 404 ERROR           │
                           │    └─────────────────────────────────────────────┘
                           │
                           └──► return errorResponse(404)
                                    │
                                    └──► errorResponse(404, server)
                                             │
                                             ├──► Check for custom error page:
                                             │        server->error_pages.find(404)
                                             │        └──► Found: "www/errors/404.html"
                                             │
                                             ├──► std::ifstream file("www/errors/404.html")
                                             │        │
                                             │        └──► file.is_open() → true ✓
                                             │             (custom error page exists)
                                             │
                                             ├──► contents << file.rdbuf()
                                             │        └──► Reads custom 404 page HTML
                                             │
                                             ├──► file.close()
                                             │
                                             └──► Response response
                                                      response.setStatus(404, "Not Found")
                                                      response.setContentType("text/html")
                                                      response.setBody(customErrorPageContent)
                                                      │
                                                      └──► Returns: Response


┌─────────────────────────────────────────────────────────────────────────────┐
│              ALTERNATIVE: NO CUSTOM ERROR PAGE CONFIGURED                   │
│         (If www/errors/404.html doesn't exist or isn't configured)          │
└─────────────────────────────────────────────────────────────────────────────┘

errorResponse(404, server)
    │
    ├──► server->error_pages.find(404)
    │        └──► Not found OR file doesn't exist
    │
    └──► return Response::error(404)
             │
             ├──► Response response
             │
             ├──► response.setStatus(404)
             │        └──► _statusCode = 404
             │            _reasonPhrase = getReasonPhrase(404) → "Not Found"
             │
             ├──► response.setContentType("text/html; charset=UTF-8")
             │
             ├──► response.setBody(getDefaultErrorPage(404))
             │        │
             │        └──► Generates HTML:
             │             "<!DOCTYPE html>\n"
             │             "<html>\n"
             │             "<head>\n"
             │             "    <meta charset=\"UTF-8\">\n"
             │             "    <title>404 Not Found</title>\n"
             │             "    <style>\n"
             │             "        body { font-family: Arial, sans-serif; "
             │             "               text-align: center; padding: 50px; }\n"
             │             "        h1 { font-size: 50px; color: #333; }\n"
             │             "        p { color: #666; }\n"
             │             "        hr { border: none; border-top: 1px solid #ddd; "
             │             "             margin: 20px 0; }\n"
             │             "    </style>\n"
             │             "</head>\n"
             │             "<body>\n"
             │             "    <h1>404</h1>\n"
             │             "    <p>Not Found</p>\n"
             │             "    <hr>\n"
             │             "    <p><small>webserv/1.0</small></p>\n"
             │             "</body>\n"
             │             "</html>\n"
             │
             ├──► response.addStandardHeaders()
             │        ├──► addDateHeader()
             │        │        └──► setHeader("Date", "Sun, 12 Jan 2026 10:00:00 GMT")
             │        ├──► addServerHeader()
             │        │        └──► setHeader("Server", "webserv/1.0")
             │        └──► setConnection(true)
             │                 └──► setHeader("Connection", "keep-alive")
             │
             └──► Returns: Response (404 Not Found)


┌─────────────────────────────────────────────────────────────────────────────┐
│                        RESPONSE SENDING PHASE                               │
└─────────────────────────────────────────────────────────────────────────────┘

conn.setResponse(response)
    │
    ├──► response.build()
    │        │
    │        └──► buildIfNeeded()
    │                 │
    │                 ├──► ss << "HTTP/1.1 404 Not Found\r\n"
    │                 │
    │                 ├──► For each header:
    │                 │        ss << "Date: Sun, 12 Jan 2026 10:00:00 GMT\r\n"
    │                 │        ss << "Server: webserv/1.0\r\n"
    │                 │        ss << "Content-Type: text/html; charset=UTF-8\r\n"
    │                 │        ss << "Content-Length: 387\r\n"  // size of error page
    │                 │        ss << "Connection: keep-alive\r\n"
    │                 │
    │                 ├──► ss << "\r\n"  // End of headers
    │                 │
    │                 └──► ss << _body   // Error page HTML
    │
    ├──► _writeBuffer = response.build()
    ├──► _writeOffset = 0
    ├──► _state = CONN_WRITING
    │
    └──► _keepAlive = response.shouldKeepAlive() → true
    │
    ▼
modifyEpoll(5, EPOLLOUT)
    │
    └──► epoll_ctl(4, EPOLL_CTL_MOD, 5, {events=EPOLLOUT, data.fd=5})


┌─────────────────────────────────────────────────────────────────────────────┐
│                         WRITE DATA PHASE                                    │
└─────────────────────────────────────────────────────────────────────────────┘

epoll_wait(4, events, 64, 1000)
    │
    └──► Returns: 1
         events[0] = {events=EPOLLOUT, data.fd=5}
    │
    ▼
handleClientEvent(clientFd=5, events=EPOLLOUT)
    │
    └──► conn.writeData()
             │
             ├──► send(5, response_data, ~500, MSG_NOSIGNAL)
             │        │
             │        └──► Returns: ~500 (all bytes sent)
             │
             │    Response sent to client:
             │    ─────────────────────────────────────────────
             │    HTTP/1.1 404 Not Found\r\n
             │    Date: Sun, 12 Jan 2026 10:00:00 GMT\r\n
             │    Server: webserv/1.0\r\n
             │    Content-Type: text/html; charset=UTF-8\r\n
             │    Content-Length: 387\r\n
             │    Connection: keep-alive\r\n
             │    \r\n
             │    <!DOCTYPE html>
             │    <html>
             │    <head>
             │        <meta charset="UTF-8">
             │        <title>404 Not Found</title>
             │        <style>
             │            body { font-family: Arial, sans-serif;
             │                   text-align: center; padding: 50px; }
             │            h1 { font-size: 50px; color: #333; }
             │            p { color: #666; }
             │            hr { border: none; border-top: 1px solid #ddd;
             │                 margin: 20px 0; }
             │        </style>
             │    </head>
             │    <body>
             │        <h1>404</h1>
             │        <p>Not Found</p>
             │        <hr>
             │        <p><small>webserv/1.0</small></p>
             │    </body>
             │    </html>
             │    ─────────────────────────────────────────────
             │
             └──► handleWriteComplete()
                      │
                      ├──► _keepAlive == true
                      │
                      └──► conn.reset()
                               │
                               ├──► _readBuffer.clear()
                               ├──► _writeBuffer.clear()
                               ├──► delete _request; _request = new Request()
                               ├──► _state = CONN_READING
                               └──► updateActivity()
    │
    ▼
modifyEpoll(5, EPOLLIN)
    │
    └──► epoll_ctl(4, EPOLL_CTL_MOD, 5, {events=EPOLLIN, data.fd=5})


┌─────────────────────────────────────────────────────────────────────────────┐
│                   CURL OUTPUT (Client Perspective)                          │
└─────────────────────────────────────────────────────────────────────────────┘

$ curl -v http://localhost:8080/nonexistent.html
*   Trying 127.0.0.1:8080...
* Connected to localhost (127.0.0.1) port 8080 (#0)
> GET /nonexistent.html HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/7.68.0
> Accept: */*
>
* Mark bundle as not supporting multiuse
< HTTP/1.1 404 Not Found
< Date: Sun, 12 Jan 2026 10:00:00 GMT
< Server: webserv/1.0
< Content-Type: text/html; charset=UTF-8
< Content-Length: 387
< Connection: keep-alive
<
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>404 Not Found</title>
    ...
</head>
<body>
    <h1>404</h1>
    <p>Not Found</p>
    <hr>
    <p><small>webserv/1.0</small></p>
</body>
</html>
* Connection #0 to host localhost left intact
```

## Key Difference from Successful Request

The critical difference happens at the `stat()` system call:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    COMPARISON: 200 OK vs 404 NOT FOUND                      │
└─────────────────────────────────────────────────────────────────────────────┘

                    Successful Request                 404 Request
                    ─────────────────                 ───────────────
Request Path:       GET /                             GET /nonexistent.html
                         │                                 │
                         ▼                                 ▼
resolvePath():      "www/"                            "www/nonexistent.html"
                         │                                 │
                         ▼                                 ▼
stat():             stat("www/", &st)                 stat("www/nonexistent.html", &st)
                    Returns: 0 ✓                      Returns: -1 ✗
                    st.st_mode = S_IFDIR              errno = ENOENT
                         │                                 │
                         ▼                                 ▼
Action:             serveDirectory()                  errorResponse(404)
                    → serveFile("www/index.html")     → Response::error(404)
                         │                                 │
                         ▼                                 ▼
Response:           HTTP/1.1 200 OK                   HTTP/1.1 404 Not Found
                    + index.html content              + error page HTML
```

## Summary: System Calls for 404 Request

| Phase | System Call | Parameters | Returns |
|-------|-------------|------------|---------|
| **Accept** | `epoll_wait` | `(4, events, 64, 1000)` | 1 |
| | `accept` | `(3, &addr, &len)` | 5 |
| | `fcntl` | `(5, F_GETFL)` | flags |
| | `fcntl` | `(5, F_SETFL, O_NONBLOCK)` | 0 |
| | `epoll_ctl` | `(4, ADD, 5, EPOLLIN)` | 0 |
| **Read** | `epoll_wait` | `(4, events, 64, 1000)` | 1 |
| | `recv` | `(5, buf, 8192, 0)` | 92 |
| **Route** | `stat` | `("www/nonexistent.html", &st)` | **-1 (ENOENT)** ⚠️ |
| **Error Page** | `open` | `("www/errors/404.html")` | 6 |
| | `read` | `(6, ...)` | content |
| | `close` | `(6)` | 0 |
| | `time` | `(NULL)` | timestamp |
| **Write** | `epoll_ctl` | `(4, MOD, 5, EPOLLOUT)` | 0 |
| | `epoll_wait` | `(4, events, 64, 1000)` | 1 |
| | `send` | `(5, response, ~500, MSG_NOSIGNAL)` | ~500 |
| | `epoll_ctl` | `(4, MOD, 5, EPOLLIN)` | 0 |

The key system call that triggers the 404 is **`stat("www/nonexistent.html", &st)`** returning `-1` with `errno = ENOENT`.
