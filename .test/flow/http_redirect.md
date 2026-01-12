# Signal Flow: HTTP Redirect (301 Moved Permanently)

Here's the complete signal flow when requesting `curl -v http://localhost:8080/old-page`:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    CONNECTION & REQUEST READING                             │
└─────────────────────────────────────────────────────────────────────────────┘

epoll_wait(4, events, 64, 1000) → 1
    │
    ▼
accept(3, &clientAddr, &clientLen) → fd=5
fcntl(5, F_GETFL) → flags
fcntl(5, F_SETFL, flags | O_NONBLOCK) → 0
epoll_ctl(4, EPOLL_CTL_ADD, 5, {EPOLLIN, fd=5}) → 0
Connection conn(5, clientAddr, serverPort=8080)
    │
    ▼
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
                      └──► Returns: ~80 bytes


┌─────────────────────────────────────────────────────────────────────────────┐
│                    HTTP REQUEST RECEIVED                                    │
└─────────────────────────────────────────────────────────────────────────────┘

curl sends:
────────────────────────────────────────────────────────────────
GET /old-page HTTP/1.1
Host: localhost:8080
User-Agent: curl/7.68.0
Accept: */*

────────────────────────────────────────────────────────────────
    │
    ▼
_request->parse(_readBuffer)
    │
    ├──► parseRequestLine("GET /old-page HTTP/1.1")
    │        │
    │        ├──► _method = "GET"
    │        ├──► _uri = "/old-page"
    │        ├──► _path = "/old-page"
    │        ├──► _queryString = ""
    │        └──► _httpVersion = "HTTP/1.1"
    │
    ├──► Parse headers:
    │        _headers["host"] = "localhost:8080"
    │        _headers["user-agent"] = "curl/7.68.0"
    │        _headers["accept"] = "*/*"
    │
    ├──► Empty line found → end of headers
    │
    ├──► No Content-Length, no Transfer-Encoding
    │        → GET requests have no body
    │
    └──► _state = PARSE_COMPLETE
         Returns: true


┌─────────────────────────────────────────────────────────────────────────────┐
│                    REQUEST ROUTING                                          │
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
                  ├──► findServer(port=8080, hostname="localhost:8080")
                  │        │
                  │        └──► Returns: &servers[0] (localhost server)
                  │
                  ├──► requestPath = request.getPath()
                  │        └──► "/old-page"
                  │
                  ├──► method = request.getMethod()
                  │        └──► "GET"
                  │
                  ├──► findLocation(server, path="/old-page", method="GET")
                  │        │
                  │        ├──► Check location "/" :
                  │        │        "/old-page".compare(0, 1, "/") == 0 ✓
                  │        │        matchLength = 1
                  │        │
                  │        ├──► Check location "/old-page":
                  │        │        "/old-page" == "/old-page" ✓
                  │        │        matchLength = 9 ← BEST MATCH!
                  │        │
                  │        └──► Returns: &LocationConfig{
                  │                        path = "/old-page",
                  │                        redirect_code = 301,
                  │                        redirect_url = "/new-page"
                  │                      }
                  │
                  │  ┌─────────────────────────────────────────────────────────┐
                  │  │   REDIRECT DETECTED!                                    │
                  │  │                                                         │
                  │  │   Config:                                               │
                  │  │     location /old-page {                                │
                  │  │         return 301 /new-page;                           │
                  │  │     }                                                   │
                  │  └─────────────────────────────────────────────────────────┘
                  │
                  ├──► location->redirect_url.empty() → false ✓
                  │
                  └──► return Response::redirect(301, "/new-page")


┌─────────────────────────────────────────────────────────────────────────────┐
│                    REDIRECT RESPONSE CREATION                               │
└─────────────────────────────────────────────────────────────────────────────┘

Response::redirect(code=301, location="/new-page")
    │
    ├──► Response response
    │
    ├──► response.setStatus(301)
    │        │
    │        ├──► _statusCode = 301
    │        │
    │        └──► _reasonPhrase = getReasonPhrase(301)
    │                  │
    │                  └──► Returns: "Moved Permanently"
    │
    ├──► response.setHeader("Location", "/new-page")
    │        │
    │        └──► _headers["Location"] = "/new-page"
    │
    │             ┌─────────────────────────────────────────────────────┐
    │             │  The Location header is the KEY to redirects!       │
    │             │                                                     │
    │             │  Browser reads this header and automatically        │
    │             │  navigates to the new URL.                          │
    │             │                                                     │
    │             │  curl with -L flag would follow this redirect.      │
    │             └─────────────────────────────────────────────────────┘
    │
    ├──► response.setContentType("text/html; charset=UTF-8")
    │
    ├──► Build fallback HTML body:
    │        │
    │        └──► body =
    │             "<!DOCTYPE html>\n"
    │             "<html>\n"
    │             "<head>\n"
    │             "    <meta charset=\"UTF-8\">\n"
    │             "    <title>Redirect</title>\n"
    │             "    <meta http-equiv=\"refresh\" content=\"0; url=/new-page\">\n"
    │             "</head>\n"
    │             "<body>\n"
    │             "    <h1>Redirecting...</h1>\n"
    │             "    <p>If you are not redirected automatically, "
    │             "<a href=\"/new-page\">click here</a>.</p>\n"
    │             "</body>\n"
    │             "</html>\n"
    │
    │             ┌─────────────────────────────────────────────────────┐
    │             │  The HTML body serves multiple purposes:            │
    │             │                                                     │
    │             │  1. <meta http-equiv="refresh">                     │
    │             │     → Backup redirect for browsers that miss        │
    │             │       the Location header                           │
    │             │                                                     │
    │             │  2. Clickable link                                  │
    │             │     → For users with very old browsers              │
    │             │     → For command-line tools like curl              │
    │             │                                                     │
    │             │  3. Human-readable message                          │
    │             │     → Explains what's happening                     │
    │             └─────────────────────────────────────────────────────┘
    │
    ├──► response.setBody(body)
    │
    ├──► response.addStandardHeaders()
    │        │
    │        ├──► addDateHeader()
    │        │        └──► time(NULL) → timestamp
    │        │             setHeader("Date", "Mon, 13 Jan 2026 10:00:00 GMT")
    │        │
    │        ├──► addServerHeader()
    │        │        └──► setHeader("Server", "webserv/1.0")
    │        │
    │        └──► setConnection(true)
    │                 └──► setHeader("Connection", "keep-alive")
    │
    └──► Returns: Response


┌─────────────────────────────────────────────────────────────────────────────┐
│                    SEND REDIRECT RESPONSE                                   │
└─────────────────────────────────────────────────────────────────────────────┘

conn.setResponse(response)
    │
    ├──► response.build()
    │        │
    │        └──► buildIfNeeded()
    │                 │
    │                 ├──► ss << "HTTP/1.1 301 Moved Permanently\r\n"
    │                 │
    │                 ├──► For each header:
    │                 │        ss << "Location: /new-page\r\n"
    │                 │        ss << "Content-Type: text/html; charset=UTF-8\r\n"
    │                 │        ss << "Date: Mon, 13 Jan 2026 10:00:00 GMT\r\n"
    │                 │        ss << "Server: webserv/1.0\r\n"
    │                 │        ss << "Connection: keep-alive\r\n"
    │                 │        ss << "Content-Length: 280\r\n"
    │                 │
    │                 ├──► ss << "\r\n"  // End of headers
    │                 │
    │                 └──► ss << _body   // Fallback HTML
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
             ├──► send(5, response_data, ~450, MSG_NOSIGNAL)
             │        │
             │        └──► Returns: ~450 (all bytes sent)
             │
             │    Complete response sent:
             │    ─────────────────────────────────────────────────────
             │    HTTP/1.1 301 Moved Permanently\r\n
             │    Location: /new-page\r\n
             │    Content-Type: text/html; charset=UTF-8\r\n
             │    Date: Mon, 13 Jan 2026 10:00:00 GMT\r\n
             │    Server: webserv/1.0\r\n
             │    Connection: keep-alive\r\n
             │    Content-Length: 280\r\n
             │    \r\n
             │    <!DOCTYPE html>
             │    <html>
             │    <head>
             │        <meta charset="UTF-8">
             │        <title>Redirect</title>
             │        <meta http-equiv="refresh" content="0; url=/new-page">
             │    </head>
             │    <body>
             │        <h1>Redirecting...</h1>
             │        <p>If you are not redirected automatically,
             │        <a href="/new-page">click here</a>.</p>
             │    </body>
             │    </html>
             │    ─────────────────────────────────────────────────────
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
│                    CURL OUTPUT (Without -L flag)                            │
└─────────────────────────────────────────────────────────────────────────────┘

$ curl -v http://localhost:8080/old-page
*   Trying 127.0.0.1:8080...
* Connected to localhost (127.0.0.1) port 8080 (#0)
> GET /old-page HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/7.68.0
> Accept: */*
>
* Mark bundle as not supporting multiuse
< HTTP/1.1 301 Moved Permanently
< Location: /new-page
< Content-Type: text/html; charset=UTF-8
< Date: Mon, 13 Jan 2026 10:00:00 GMT
< Server: webserv/1.0
< Connection: keep-alive
< Content-Length: 280
<
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Redirect</title>
    <meta http-equiv="refresh" content="0; url=/new-page">
</head>
<body>
    <h1>Redirecting...</h1>
    <p>If you are not redirected automatically, <a href="/new-page">click here</a>.</p>
</body>
</html>
* Connection #0 to host localhost left intact


┌─────────────────────────────────────────────────────────────────────────────┐
│                    CURL OUTPUT (With -L flag - Follow Redirects)            │
└─────────────────────────────────────────────────────────────────────────────┘

$ curl -v -L http://localhost:8080/old-page
*   Trying 127.0.0.1:8080...
* Connected to localhost (127.0.0.1) port 8080 (#0)
> GET /old-page HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/7.68.0
> Accept: */*
>
* Mark bundle as not supporting multiuse
< HTTP/1.1 301 Moved Permanently
< Location: /new-page
< Content-Type: text/html; charset=UTF-8
< Date: Mon, 13 Jan 2026 10:00:00 GMT
< Server: webserv/1.0
< Connection: keep-alive
< Content-Length: 280
<
* Ignoring the response-body
* Connection #0 to host localhost left intact
* Issue another request to this URL: 'http://localhost:8080/new-page'
* Re-using existing connection! (#0) with host localhost      ← Keep-alive!
> GET /new-page HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/7.68.0
> Accept: */*
>
< HTTP/1.1 404 Not Found           ← /new-page doesn't exist in this example
< Content-Type: text/html
< ...
```

## Redirect Flow Visualization

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    REDIRECT PROCESS DIAGRAM                                 │
└─────────────────────────────────────────────────────────────────────────────┘

Browser/Client                          webserv                     Config
     │                                     │                           │
     │  GET /old-page HTTP/1.1             │                           │
     │────────────────────────────────────►│                           │
     │                                     │                           │
     │                                     │ findLocation("/old-page") │
     │                                     │──────────────────────────►│
     │                                     │                           │
     │                                     │ location {                │
     │                                     │   path: "/old-page"       │
     │                                     │   redirect_code: 301      │
     │                                     │   redirect_url:"/new-page"│
     │                                     │◄──────────────────────────│
     │                                     │                           │
     │  HTTP/1.1 301 Moved Permanently     │                           │
     │  Location: /new-page                │                           │
     │◄────────────────────────────────────│                           │
     │                                     │                           │
     │                                     │                           │
     │  (Browser automatically follows)    │                           │
     │                                     │                           │
     │  GET /new-page HTTP/1.1             │                           │
     │────────────────────────────────────►│                           │
     │                                     │                           │
     │  HTTP/1.1 200 OK (or 404)           │                           │
     │◄────────────────────────────────────│                           │
     │                                     │                           │
```

## Key Routing Decision Point

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    ROUTING DECISION: REDIRECT CHECK                         │
└─────────────────────────────────────────────────────────────────────────────┘

Router::route(request, serverPort)
    │
    ├──► findServer() ✓
    │
    ├──► findLocation() ✓
    │        │
    │        └──► Returns: location with redirect_url = "/new-page"
    │
    │    ┌─────────────────────────────────────────────────────────────────┐
    │    │  REDIRECT CHECK (happens BEFORE method/CGI/file checks!)        │
    │    │                                                                 │
    │    │  if (!location->redirect_url.empty())                           │
    │    │  {                                                              │
    │    │      return Response::redirect(                                 │
    │    │          location->redirect_code,  // 301                       │
    │    │          location->redirect_url    // "/new-page"               │
    │    │      );                                                         │
    │    │  }                                                              │
    │    │                                                                 │
    │    │  This means:                                                    │
    │    │  - Method is NOT checked (redirect works for any method)        │
    │    │  - File existence is NOT checked (redirect doesn't need files)  │
    │    │  - CGI is NOT checked (redirect bypasses CGI)                   │
    │    │  - Redirects are the FIRST thing checked after finding location │
    │    └─────────────────────────────────────────────────────────────────┘
    │
    ├──► location->redirect_url.empty() ?
    │        │
    │        └──► false! We have a redirect URL
    │
    └──► return Response::redirect(301, "/new-page")
         │
         │    (Method check, CGI check, file handling - ALL SKIPPED!)
         │
         └──► Never reaches these:
              ├──► isMethodAllowed()  ← NOT called
              ├──► isCgiRequest()     ← NOT called
              ├──► handleGet()        ← NOT called
              ├──► handlePost()       ← NOT called
              └──► handleDelete()     ← NOT called
```

## Summary: System Calls for Redirect Request

| Phase | System Call | Parameters | Returns |
|-------|-------------|------------|---------|
| **Accept** | `accept` | `(3, &addr, &len)` | 5 |
| | `fcntl` | `(5, F_GETFL)` | flags |
| | `fcntl` | `(5, F_SETFL, O_NONBLOCK)` | 0 |
| | `epoll_ctl` | `(4, ADD, 5, EPOLLIN)` | 0 |
| **Read** | `epoll_wait` | `(4, events, 64, 1000)` | 1 |
| | `recv` | `(5, buf, 8192, 0)` | ~80 |
| **Route** | **(No file system calls!)** | | |
| | `time` | `(NULL)` | timestamp |
| **Write** | `epoll_ctl` | `(4, MOD, 5, EPOLLOUT)` | 0 |
| | `epoll_wait` | `(4, events, 64, 1000)` | 1 |
| | `send` | `(5, response, ~450, MSG_NOSIGNAL)` | ~450 |
| | `epoll_ctl` | `(4, MOD, 5, EPOLLIN)` | 0 |

## Key Difference: Redirect vs Normal Requests

| Aspect | Normal GET Request | Redirect Request |
|--------|-------------------|------------------|
| **Method Check** | ✓ Performed | ✗ Skipped |
| **File System Access** | `stat()`, `open()`, `read()` | **None!** |
| **CGI Check** | ✓ Performed | ✗ Skipped |
| **Response Status** | 200 OK | **301 Moved Permanently** |
| **Location Header** | Not present | **Required** (`/new-page`) |
| **Response Body** | File content | Fallback HTML with link |
| **Browser Behavior** | Displays content | **Navigates to new URL** |

The key insight is that **redirects are handled entirely in memory** — no file system access is needed. The router simply checks if the location has a `redirect_url` configured and immediately returns a redirect response.
