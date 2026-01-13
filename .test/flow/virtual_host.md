# Signal Flow: Virtual Host Request

The complete signal flow when requesting:

`curl -v --resolve marigold.hotel:8080:127.0.0.1 http://marigold.hotel:8080/`


```cpp
┌─────────────────────────────────────────────────────────────────────────────┐
│                    CURL --resolve EXPLANATION                               │
└─────────────────────────────────────────────────────────────────────────────┘

The --resolve flag tells curl:
  "When connecting to marigold.hotel:8080, use IP 127.0.0.1"

This bypasses DNS lookup and allows testing virtual hosts locally.

Without --resolve:
  curl http://marigold.hotel:8080/
  → DNS lookup for "marigold.hotel" → FAILS (not a real domain)

With --resolve:
  curl --resolve marigold.hotel:8080:127.0.0.1 http://marigold.hotel:8080/
  → Skip DNS, connect to 127.0.0.1:8080
  → Send Host: marigold.hotel header
  → Server uses Host header for virtual host routing


┌─────────────────────────────────────────────────────────────────────────────┐
│                    CONNECTION & REQUEST READING                             │
└─────────────────────────────────────────────────────────────────────────────┘

epoll_wait(4, events, 64, 1000) → 1
    │
    ▼
accept(3, &clientAddr, &clientLen) → fd=5
    │
    │   clientAddr.sin_addr = 127.0.0.1
    │   clientAddr.sin_port = 54321 (ephemeral client port)
    │
    ▼
fcntl(5, F_GETFL) → flags
fcntl(5, F_SETFL, flags | O_NONBLOCK) → 0
epoll_ctl(4, EPOLL_CTL_ADD, 5, {EPOLLIN, fd=5}) → 0
    │
    ▼
Connection conn(5, clientAddr, serverPort=8080)
    │
    ├──► _fd = 5
    ├──► _clientIP = "127.0.0.1"
    ├──► _clientPort = 54321
    ├──► _serverPort = 8080          ← KEY: Which port received this request
    ├──► _state = CONN_READING
    └──► _request = new Request()


┌─────────────────────────────────────────────────────────────────────────────┐
│                    HTTP REQUEST RECEIVED                                    │
└─────────────────────────────────────────────────────────────────────────────┘

epoll_wait(4, events, 64, 1000) → 1
events[0] = {events=EPOLLIN, data.fd=5}
    │
    ▼
handleClientEvent(clientFd=5, events=EPOLLIN)
    │
    └──► conn.readData()
             │
             └──► recv(5, buffer[8192], 8192, 0)
                      │
                      └──► Returns: ~95 bytes

curl sends:
════════════════════════════════════════════════════════════════════════════════
GET / HTTP/1.1
Host: marigold.hotel:8080      ← KEY HEADER! Virtual host identification
User-Agent: curl/7.68.0
Accept: */ *

════════════════════════════════════════════════════════════════════════════════
    │
    ▼
_request->parse(_readBuffer)
    │
    │  ┌─────────────────────────────────────────────────────────────────┐
    │  │                  PARSE_REQUEST_LINE                             │
    │  └─────────────────────────────────────────────────────────────────┘
    │
    ├──► parseRequestLine("GET / HTTP/1.1")
    │        │
    │        ├──► _method = "GET"
    │        ├──► _uri = "/"
    │        ├──► _path = "/"
    │        ├──► _queryString = ""
    │        └──► _httpVersion = "HTTP/1.1"
    │
    │  ┌─────────────────────────────────────────────────────────────────┐
    │  │                  PARSE_HEADERS                                  │
    │  └─────────────────────────────────────────────────────────────────┘
    │
    ├──► Parse headers:
    │        _headers["host"] = "marigold.hotel:8080"    ← VIRTUAL HOST KEY!
    │        _headers["user-agent"] = "curl/7.68.0"
    │        _headers["accept"] = "*/ *"
    │
    ├──► Empty line found → end of headers
    │
    ├──► No Content-Length, no Transfer-Encoding
    │        → GET requests have no body
    │
    └──► _state = PARSE_COMPLETE
         Returns: true


┌─────────────────────────────────────────────────────────────────────────────┐
│                    REQUEST ROUTING - VIRTUAL HOST SELECTION                 │
└─────────────────────────────────────────────────────────────────────────────┘

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
                  │  ┌─────────────────────────────────────────────────────────┐
                  │  │   Step 1: Get Host header for virtual hosting           │
                  │  └─────────────────────────────────────────────────────────┘
                  │
                  ├──► hostHeader = request.getHeader("Host")
                  │        │
                  │        └──► Returns: "marigold.hotel:8080"
                  │
                  │  ┌─────────────────────────────────────────────────────────┐
                  │  │   Step 2: Find matching server (VIRTUAL HOST LOGIC)     │
                  │  └─────────────────────────────────────────────────────────┘
                  │
                  ├──► findServer(port=8080, hostname="marigold.hotel:8080")
                  │        │
                  │        │  ┌─────────────────────────────────────────────────┐
                  │        │  │  VIRTUAL HOST SELECTION ALGORITHM               │
                  │        │  │                                                 │
                  │        │  │  Multiple server blocks can listen on same port │
                  │        │  │  We match by server_name directive              │
                  │        │  └─────────────────────────────────────────────────┘
                  │        │
                  │        ├──► Extract hostname without port:
                  │        │        host = "marigold.hotel:8080"
                  │        │        colonPos = host.find(':') → 15
                  │        │        host = "marigold.hotel"
                  │        │
                  │        ├──► Iterate through all servers:
                  │        │
                  │        │    ┌─────────────────────────────────────────────┐
                  │        │    │ Server Block 1 (index 0):                   │
                  │        │    │   listen 8080;                              │
                  │        │    │   server_name localhost;                    │
                  │        │    │   ...                                       │
                  │        │    └─────────────────────────────────────────────┘
                  │        │        │
                  │        │        ├──► servers[0].port == 8080 ✓ (matches)
                  │        │        │
                  │        │        ├──► Check server_names:
                  │        │        │        names = ["localhost"]
                  │        │        │        "localhost" == "marigold.hotel" ? NO
                  │        │        │
                  │        │        └──► defaultServer = &servers[0]
                  │        │             (First server on port is default)
                  │        │
                  │        │    ┌─────────────────────────────────────────────┐
                  │        │    │ Server Block 2 (index 1):                   │
                  │        │    │   listen 8080;                              │
                  │        │    │   server_name marigold.hotel;               │
                  │        │    │   ...                                       │
                  │        │    └─────────────────────────────────────────────┘
                  │        │        │
                  │        │        ├──► servers[1].port == 8080 ✓ (matches)
                  │        │        │
                  │        │        ├──► Check server_names:
                  │        │        │        names = ["marigold.hotel"]
                  │        │        │        "marigold.hotel" == "marigold.hotel" ✓
                  │        │        │
                  │        │        └──► EXACT MATCH FOUND!
                  │        │             Return: &servers[1]
                  │        │
                  │        └──► Returns: &ServerConfig{
                  │                        host = "0.0.0.0",
                  │                        port = 8080,
                  │                        server_names = ["marigold.hotel"],
                  │                        locations = [
                  │                            LocationConfig{
                  │                                path = "/",
                  │                                root = "www/marigold",
                  │                                index = "index.html",
                  │                                allowed_methods = {GET}
                  │                            }
                  │                        ]
                  │                      }
                  │
                  │  ┌─────────────────────────────────────────────────────────┐
                  │  │   Step 3: Find location (using marigold server config)  │
                  │  └─────────────────────────────────────────────────────────┘
                  │
                  ├──► requestPath = request.getPath()
                  │        └──► "/"
                  │
                  ├──► method = request.getMethod()
                  │        └──► "GET"
                  │
                  ├──► findLocation(server, path="/", method="GET")
                  │        │
                  │        ├──► Check location "/" in marigold server:
                  │        │        "/" == "/" ✓
                  │        │        matchLength = 1 ← BEST MATCH
                  │        │
                  │        └──► Returns: &LocationConfig{
                  │                        path = "/",
                  │                        root = "www/marigold",      ← DIFFERENT ROOT!
                  │                        index = "index.html",
                  │                        allowed_methods = {GET}
                  │                      }
                  │
                  ├──► location->redirect_url.empty() → true (no redirect)
                  │
                  ├──► isMethodAllowed("GET", location)
                  │        │
                  │        └──► location.allowed_methods.count("GET") > 0 → true ✓
                  │
                  ├──► resolvePath("/", location)
                  │        │
                  │        ├──► root = "www/marigold"      ← MARIGOLD'S ROOT
                  │        ├──► locationPath = "/"
                  │        ├──► remainder = "/"
                  │        │
                  │        └──► Returns: "www/marigold/"
                  │
                  ├──► isCgiRequest("www/marigold/", location) → false
                  │
                  ├──► method == "GET" → true
                  │
                  └──► handleGet(request, location, server)


┌─────────────────────────────────────────────────────────────────────────────┐
│                    SERVE MARIGOLD'S CONTENT                                 │
└─────────────────────────────────────────────────────────────────────────────┘

handleGet(request, location, server)
    │
    ├──► path = resolvePath("/", location)
    │        │
    │        └──► Returns: "www/marigold/"
    │
    ├──► stat("www/marigold/", &pathStat)
    │        │
    │        └──► Returns: 0 (success)
    │             pathStat.st_mode = S_IFDIR | 0755  (it is a directory)
    │
    ├──► S_ISDIR(pathStat.st_mode) → true
    │
    └──► serveDirectory("www/marigold/", location, server)
             │
             │  ┌─────────────────────────────────────────────────────────┐
             │  │   Step 1: Try index file                                │
             │  └─────────────────────────────────────────────────────────┘
             │
             ├──► location.index = "index.html" (not empty)
             │
             ├──► Build index path:
             │        indexPath = "www/marigold/"
             │        indexPath += "index.html"
             │        indexPath = "www/marigold/index.html"
             │
             ├──► stat("www/marigold/index.html", &indexStat)
             │        │
             │        └──► Returns: 0 (file exists!)
             │             indexStat.st_mode = S_IFREG | 0644 (regular file)
             │
             ├──► S_ISREG(indexStat.st_mode) → true ✓
             │
             └──► serveFile("www/marigold/index.html")
                      │
                      ├──► std::ifstream file("www/marigold/index.html", ios::binary)
                      │        │
                      │        └──► open("www/marigold/index.html", O_RDONLY) → fd=6
                      │
                      ├──► file.is_open() → true ✓
                      │
                      ├──► Read file contents:
                      │        file.rdbuf() → contents
                      │        │
                      │        └──► read(6, buffer, ...) → file content
                      │
                      │    File content (example):
                      │    ─────────────────────────────────────────────────
                      │    <!DOCTYPE html>
                      │    <html>
                      │    <head><title>Welcome to Marigold Hotel</title></head>
                      │    <body>
                      │        <h1>🌼 Welcome to the Marigold Hotel! 🌼</h1>
                      │        <p>Your virtual home away from home.</p>
                      │    </body>
                      │    </html>
                      │    ─────────────────────────────────────────────────
                      │
                      ├──► file.close()
                      │        │
                      │        └──► close(6)
                      │
                      ├──► Determine Content-Type:
                      │        filepath = "www/marigold/index.html"
                      │        extension = ".html"
                      │        contentType = getMimeType(".html")
                      │        contentType = "text/html; charset=UTF-8"
                      │
                      ├──► Response response
                      │        response.setStatus(200, "OK")
                      │        response.setContentType("text/html; charset=UTF-8")
                      │        response.setBody(contents)
                      │        response.addStandardHeaders()
                      │            ├──► setHeader("Date", "Mon, 13 Jan 2026 10:00:00 GMT")
                      │            ├──► setHeader("Server", "webserv/1.0")
                      │            └──► setHeader("Connection", "keep-alive")
                      │
                      └──► Returns: response


┌─────────────────────────────────────────────────────────────────────────────┐
│                    SEND RESPONSE                                            │
└─────────────────────────────────────────────────────────────────────────────┘

conn.setResponse(response)
    │
    ├──► response.build()
    │        │
    │        └──► _builtResponse =
    │             "HTTP/1.1 200 OK\r\n"
    │             "Content-Type: text/html; charset=UTF-8\r\n"
    │             "Date: Mon, 13 Jan 2026 10:00:00 GMT\r\n"
    │             "Server: webserv/1.0\r\n"
    │             "Connection: keep-alive\r\n"
    │             "Content-Length: 245\r\n"
    │             "\r\n"
    │             "<!DOCTYPE html>\n"
    │             "<html>\n"
    │             "<head><title>Welcome to Marigold Hotel</title></head>\n"
    │             "<body>\n"
    │             "    <h1>🌼 Welcome to the Marigold Hotel! 🌼</h1>\n"
    │             "    <p>Your virtual home away from home.</p>\n"
    │             "</body>\n"
    │             "</html>\n"
    │
    ├──► _writeBuffer = response.build()
    ├──► _writeOffset = 0
    ├──► _state = CONN_WRITING
    └──► _keepAlive = true
    │
    ▼
modifyEpoll(5, EPOLLOUT)
    │
    └──► epoll_ctl(4, EPOLL_CTL_MOD, 5, {events=EPOLLOUT, data.fd=5})


┌─────────────────────────────────────────────────────────────────────────────┐
│                    WRITE RESPONSE                                           │
└─────────────────────────────────────────────────────────────────────────────┘

epoll_wait(4, events, 64, 1000) → 1
events[0] = {events=EPOLLOUT, data.fd=5}
    │
    ▼
handleClientEvent(clientFd=5, events=EPOLLOUT)
    │
    └──► conn.writeData()
             │
             ├──► send(5, response_data, ~400, MSG_NOSIGNAL)
             │        │
             │        └──► Returns: ~400 (all bytes sent)
             │
             └──► handleWriteComplete()
                      │
                      ├──► _keepAlive == true
                      │
                      └──► conn.reset()  // Ready for next request
    │
    ▼
modifyEpoll(5, EPOLLIN)
    │
    └──► epoll_ctl(4, EPOLL_CTL_MOD, 5, {events=EPOLLIN, data.fd=5})


┌─────────────────────────────────────────────────────────────────────────────┐
│                    CURL OUTPUT                                              │
└─────────────────────────────────────────────────────────────────────────────┘

$ curl -v --resolve marigold.hotel:8080:127.0.0.1 http://marigold.hotel:8080/
* Added marigold.hotel:8080:127.0.0.1 to DNS cache
* Hostname marigold.hotel was found in DNS cache
*   Trying 127.0.0.1:8080...
* Connected to marigold.hotel (127.0.0.1) port 8080 (#0)
> GET / HTTP/1.1
> Host: marigold.hotel:8080
> User-Agent: curl/7.68.0
> Accept: */ *
>
* Mark bundle as not supporting multiuse
< HTTP/1.1 200 OK
< Content-Type: text/html; charset=UTF-8
< Date: Mon, 13 Jan 2026 10:00:00 GMT
< Server: webserv/1.0
< Connection: keep-alive
< Content-Length: 245
<
<!DOCTYPE html>
<html>
<head><title>Welcome to Marigold Hotel</title></head>
<body>
    <h1>🌼 Welcome to the Marigold Hotel! 🌼</h1>
    <p>Your virtual home away from home.</p>
</body>
</html>
* Connection #0 to host marigold.hotel left intact
```

## Virtual Host Selection Visualization

```cpp
┌─────────────────────────────────────────────────────────────────────────────┐
│                    VIRTUAL HOST ROUTING DECISION TREE                       │
└─────────────────────────────────────────────────────────────────────────────┘

                        Request arrives on port 8080
                                    │
                                    ▼
                    ┌───────────────────────────────┐
                    │   Host header value?          │
                    └───────────────────────────────┘
                                    │
              ┌─────────────────────┼─────────────────────┐
              ▼                     ▼                     ▼
    "localhost:8080"      "marigold.hotel:8080"      "unknown:8080"
              │                     │                     │
              ▼                     ▼                     ▼
    ┌─────────────────┐   ┌─────────────────┐   ┌─────────────────┐
    │ Server Block 1  │   │ Server Block 2  │   │ Server Block 1  │
    │ server_name:    │   │ server_name:    │   │ (default for    │
    │   localhost     │   │ marigold.hotel  │   │  port 8080)     │
    │                 │   │                 │   │                 │
    │ root: www       │   │ root:           │   │ root: www       │
    └─────────────────┘   │   www/marigold  │   └─────────────────┘
                          └─────────────────┘
              │                     │                     │
              ▼                     ▼                     ▼
    Serve www/index.html  Serve www/marigold/   Serve www/index.html
                              index.html


┌─────────────────────────────────────────────────────────────────────────────┐
│                    FILESYSTEM STRUCTURE                                     │
└─────────────────────────────────────────────────────────────────────────────┘

www/                              ← Root for "localhost" server
├── index.html                    ← Served for curl http://localhost:8080/
├── files/
├── uploads/
├── cgi-bin/
├── errors/
│   ├── 404.html
│   └── 500.html
│
└── marigold/                     ← Root for "marigold.hotel" server
    └── index.html                ← Served for curl http://marigold.hotel:8080/
```


## Configuration Reference

```cpp
┌─────────────────────────────────────────────────────────────────────────────┐
│                    RELEVANT CONFIG SECTIONS                                 │
└─────────────────────────────────────────────────────────────────────────────┘

# Server Block 1: localhost (default for port 8080)
server {
    listen 8080;
    server_name localhost;              ← Matches "Host: localhost"

    location / {
        root www;                       ← Serves files from www/
        index index.html;
        allowed_methods GET POST;
    }
    # ... other locations ...
}

# Server Block 2: marigold.hotel (virtual host)
server {
    listen 8080;                        ← SAME PORT as server block 1!
    server_name marigold.hotel;         ← Matches "Host: marigold.hotel"

    location / {
        root www/marigold;              ← DIFFERENT ROOT! Serves from www/marigold/
        index index.html;
        allowed_methods GET;
    }
}
```


## Key Difference: Virtual Host vs Regular Request

```cpp
┌─────────────────────────────────────────────────────────────────────────────┐
│                    COMPARISON: localhost vs marigold.hotel                  │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────┬─────────────────────────────────────────────────────┐
│         Aspect          │                     Value                           │
├─────────────────────────┼──────────────────────┬──────────────────────────────┤
│                         │      localhost       │     marigold.hotel           │
├─────────────────────────┼──────────────────────┼──────────────────────────────┤
│ curl command            │ curl localhost:8080/ │ curl --resolve ...:8080/     │
│ Host header             │ localhost:8080       │ marigold.hotel:8080          │
│ Server block matched    │ servers[0]           │ servers[1]                   │
│ server_name directive   │ localhost            │ marigold.hotel               │
│ root directory          │ www                  │ www/marigold                 │
│ File served             │ www/index.html       │ www/marigold/index.html      │
│ Content                 │ Main webserv page    │ Marigold Hotel welcome       │
│ TCP connection          │ 127.0.0.1:8080       │ 127.0.0.1:8080 (same!)       │
└─────────────────────────┴──────────────────────┴──────────────────────────────┘

KEY INSIGHT:
Both requests connect to the SAME IP:PORT (127.0.0.1:8080)!
The ONLY difference is the "Host" HTTP header.
The server uses this header to select which server block handles the request.

This is how one server can host multiple websites on the same port!
```


## Summary: System Calls for Virtual Host Request

| Phase | System Call | Parameters | Returns |
|-------|-------------|------------|---------|
| **Accept** | `accept` | `(3, &addr, &len)` | 5 |
| | `fcntl` | `(5, F_SETFL, O_NONBLOCK)` | 0 |
| | `epoll_ctl` | `(4, ADD, 5, EPOLLIN)` | 0 |
| **Read** | `epoll_wait` | `(4, events, 64, 1000)` | 1 |
| | `recv` | `(5, buf, 8192, 0)` | ~95 |
| **Route** | **(No syscalls - pure config matching)** | Host header → server_name | |
| **Serve** | `stat` | `("www/marigold/", &st)` | 0 (directory) |
| | `stat` | `("www/marigold/index.html", &st)` | 0 (file exists) |
| | `open` | `("www/marigold/index.html", O_RDONLY)` | 6 |
| | `read` | `(6, buffer, ...)` | file content |
| | `close` | `(6)` | 0 |
| | `time` | `(NULL)` | timestamp |
| **Write** | `epoll_ctl` | `(4, MOD, 5, EPOLLOUT)` | 0 |
| | `epoll_wait` | `(4, events, 64, 1000)` | 1 |
| | `send` | `(5, response, ~400, MSG_NOSIGNAL)` | ~400 |
| | `epoll_ctl` | `(4, MOD, 5, EPOLLIN)` | 0 |

The key insight is that **virtual hosting happens entirely in application logic** -- the TCP connection is identical regardless of which virtual host is requested. The Host header in the HTTP request is the only differentiator.
