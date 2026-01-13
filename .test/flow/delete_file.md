# Signal Flow: DELETE Request

The complete signal flow when deleting a file with:

`curl -v -X DELETE http://localhost:8080/uploads/Makefile`


```cpp
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
                      └──► Returns: ~85 bytes


┌─────────────────────────────────────────────────────────────────────────────┐
│                    HTTP REQUEST RECEIVED                                    │
└─────────────────────────────────────────────────────────────────────────────┘

curl sends:
────────────────────────────────────────────────────────────────
DELETE /uploads/Makefile HTTP/1.1
Host: localhost:8080
User-Agent: curl/7.68.0
Accept: * / *

────────────────────────────────────────────────────────────────
    │
    ▼
_request->parse(_readBuffer)
    │
    │  ┌─────────────────────────────────────────────────────────────────┐
    │  │                  PARSE_REQUEST_LINE                             │
    │  └─────────────────────────────────────────────────────────────────┘
    │
    ├──► parseRequestLine("DELETE /uploads/Makefile HTTP/1.1")
    │        │
    │        ├──► std::stringstream ss(line)
    │        ├──► ss >> _method >> _uri >> _httpVersion
    │        │        _method = "DELETE"
    │        │        _uri = "/uploads/Makefile"
    │        │        _httpVersion = "HTTP/1.1"
    │        │
    │        ├──► Validate method:
    │        │        _method != "GET" ✓
    │        │        _method != "POST" ✓
    │        │        _method == "DELETE" ✓  (allowed)
    │        │
    │        ├──► Parse URI into path and query:
    │        │        _uri.find('?') → npos (no query string)
    │        │        _path = "/uploads/Makefile"
    │        │        _queryString = ""
    │        │
    │        └──► Returns: true
    │
    │  ┌─────────────────────────────────────────────────────────────────┐
    │  │                  PARSE_HEADERS                                  │
    │  └─────────────────────────────────────────────────────────────────┘
    │
    ├──► Parse headers:
    │        _headers["host"] = "localhost:8080"
    │        _headers["user-agent"] = "curl/7.68.0"
    │        _headers["accept"] = "*/*"
    │
    ├──► Empty line found → end of headers
    │
    ├──► Check Content-Length: not present
    ├──► Check Transfer-Encoding: not present
    │
    │    ┌─────────────────────────────────────────────────────────────┐
    │    │  DELETE requests typically have NO BODY                     │
    │    │  Request is complete after headers!                         │
    │    └─────────────────────────────────────────────────────────────┘
    │
    └──► _state = PARSE_COMPLETE
         Returns: true


┌─────────────────────────────────────────────────────────────────────────────┐
│                    REQUEST PROCESSING                                       │
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
    ├──► request->hasError() → false (parsing succeeded)
    │
    └──► Router router(*_config)
         │
         └──► router.route(request, serverPort=8080)
                  │
                  ├──► findServer(port=8080, hostname="localhost:8080")
                  │        │
                  │        ├──► host = "localhost" (strip port)
                  │        │
                  │        └──► Returns: &servers[0] (localhost server)
                  │
                  ├──► requestPath = request.getPath()
                  │        └──► "/uploads/Makefile"
                  │
                  ├──► method = request.getMethod()
                  │        └──► "DELETE"
                  │
                  ├──► findLocation(server, path="/uploads/Makefile", method="DELETE")
                  │        │
                  │        ├──► Check location "/" :
                  │        │        "/uploads/Makefile".compare(0, 1, "/") == 0 ✓
                  │        │        matchLength = 1
                  │        │
                  │        ├──► Check location "/uploads":
                  │        │        "/uploads/Makefile".compare(0, 8, "/uploads") == 0 ✓
                  │        │        path[8] == '/' ✓ (path boundary)
                  │        │        matchLength = 8 ← BEST MATCH!
                  │        │
                  │        └──► Returns: &LocationConfig{
                  │                        path = "/uploads",
                  │                        root = "www",
                  │                        upload_path = "www/uploads",
                  │                        allowed_methods = {GET, POST, DELETE},
                  │                        autoindex = true,
                  │                        client_max_body_size = 10485760
                  │                      }
                  │
                  ├──► location->redirect_url.empty() → true (no redirect)
                  │
                  ├──► isMethodAllowed("DELETE", location)
                  │        │
                  │        └──► location.allowed_methods.count("DELETE") > 0 → true ✓
                  │
                  ├──► resolvePath("/uploads/Makefile", location)
                  │        │
                  │        ├──► root = "www"
                  │        ├──► locationPath = "/uploads"
                  │        ├──► remainder = "/Makefile"
                  │        │
                  │        └──► Returns: "www/uploads/Makefile"
                  │
                  ├──► isCgiRequest("www/uploads/Makefile", location)
                  │        │
                  │        └──► location.cgi_path.empty() → true
                  │             Returns: false (not CGI)
                  │
                  ├──► method == "GET" → false
                  ├──► method == "POST" → false
                  ├──► method == "DELETE" → true ✓
                  │
                  └──► handleDelete(request, location, server)


┌─────────────────────────────────────────────────────────────────────────────┐
│                    handleDelete() - File Deletion                           │
└─────────────────────────────────────────────────────────────────────────────┘

handleDelete(request, location, server)
    │
    │  ┌─────────────────────────────────────────────────────────────────┐
    │  │   Step 1: Resolve filesystem path                               │
    │  └─────────────────────────────────────────────────────────────────┘
    │
    ├──► path = resolvePath("/uploads/Makefile", location)
    │        │
    │        └──► Returns: "www/uploads/Makefile"
    │
    │  ┌─────────────────────────────────────────────────────────────────┐
    │  │   Step 2: Check if file exists                                  │
    │  └─────────────────────────────────────────────────────────────────┘
    │
    ├──► stat("www/uploads/Makefile", &pathStat)
    │        │
    │        └──► Returns: 0 (success - file exists!)
    │             pathStat.st_mode = S_IFREG | 0644  (regular file)
    │             pathStat.st_size = 1500  (file size in bytes)
    │
    │  ┌─────────────────────────────────────────────────────────────────┐
    │  │   Step 3: Verify it is NOT a directory                           │
    │  └─────────────────────────────────────────────────────────────────┘
    │
    ├──► S_ISDIR(pathStat.st_mode)
    │        │
    │        └──► false ✓ (it is a regular file, not a directory)
    │
    │  ┌─────────────────────────────────────────────────────────────────┐
    │  │   Step 4: Delete the file                                       │
    │  └─────────────────────────────────────────────────────────────────┘
    │
    ├──► unlink("www/uploads/Makefile")
    │        │
    │        │   unlink() removes the directory entry for the file.
    │        │   The file data is freed when no process has it open.
    │        │
    │        │   ┌───────────────────────────────────────────────────────┐
    │        │   │  BEFORE unlink():                                     │
    │        │   │                                                       │
    │        │   │  www/uploads/                                         │
    │        │   │  ├── Makefile    ← file exists                        │
    │        │   │  └── other_files...                                   │
    │        │   │                                                       │
    │        │   │  AFTER unlink():                                      │
    │        │   │                                                       │
    │        │   │  www/uploads/                                         │
    │        │   │  └── other_files...                                   │
    │        │   │                    ← Makefile is GONE!                │
    │        │   └───────────────────────────────────────────────────────┘
    │        │
    │        └──► Returns: 0 (success - file deleted!)
    │
    │  ┌─────────────────────────────────────────────────────────────────┐
    │  │   Step 5: Build success response (204 No Content)               │
    │  └─────────────────────────────────────────────────────────────────┘
    │
    └──► return Response::noContent()
             │
             ├──► Response response
             │
             ├──► response.setStatus(204, "No Content")
             │        │
             │        ├──► _statusCode = 204
             │        └──► _reasonPhrase = "No Content"
             │
             ├──► response.addStandardHeaders()
             │        │
             │        ├──► addDateHeader()
             │        │        └──► setHeader("Date", "Mon, 13 Jan 2026 10:00:00 GMT")
             │        │
             │        ├──► addServerHeader()
             │        │        └──► setHeader("Server", "webserv/1.0")
             │        │
             │        └──► setConnection(true)
             │                 └──► setHeader("Connection", "keep-alive")
             │
             │    ┌─────────────────────────────────────────────────────┐
             │    │  204 No Content:                                    │
             │    │  - MUST NOT have a message body                     │
             │    │  - MUST NOT have Content-Length header              │
             │    │  - Indicates successful deletion                    │
             │    └─────────────────────────────────────────────────────┘
             │
             └──► Returns: Response


┌─────────────────────────────────────────────────────────────────────────────┐
│                    SEND RESPONSE                                            │
└─────────────────────────────────────────────────────────────────────────────┘

conn.setResponse(response)
    │
    ├──► response.build()
    │        │
    │        └──► buildIfNeeded()
    │                 │
    │                 ├──► ss << "HTTP/1.1 204 No Content\r\n"
    │                 │
    │                 ├──► For each header:
    │                 │        ss << "Date: Mon, 13 Jan 2026 10:00:00 GMT\r\n"
    │                 │        ss << "Server: webserv/1.0\r\n"
    │                 │        ss << "Connection: keep-alive\r\n"
    │                 │
    │                 ├──► ss << "\r\n"  // End of headers
    │                 │
    │                 │    ┌─────────────────────────────────────────────┐
    │                 │    │  NO BODY for 204 No Content!                │
    │                 │    │  (response.setBody() was never called)      │
    │                 │    └─────────────────────────────────────────────┘
    │                 │
    │                 └──► _builtResponse = ss.str()
    │
    ├──► _writeBuffer = response.build()
    │        │
    │        └──► Complete response:
    │             "HTTP/1.1 204 No Content\r\n"
    │             "Date: Mon, 13 Jan 2026 10:00:00 GMT\r\n"
    │             "Server: webserv/1.0\r\n"
    │             "Connection: keep-alive\r\n"
    │             "\r\n"
    │
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
    ├──► conn.hasDataToWrite() → true
    │
    └──► conn.writeData()
             │
             ├──► remaining = _writeBuffer.size() - _writeOffset
             │        └──► ~110 bytes
             │
             ├──► send(5, response_data, ~110, MSG_NOSIGNAL)
             │        │
             │        └──► Returns: ~110 (all bytes sent)
             │
             ├──► _writeOffset += 110
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
                               └──► updateActivity()
    │
    ▼
modifyEpoll(5, EPOLLIN)
    │
    └──► epoll_ctl(4, EPOLL_CTL_MOD, 5, {events=EPOLLIN, data.fd=5})


┌─────────────────────────────────────────────────────────────────────────────┐
│                    CURL OUTPUT                                              │
└─────────────────────────────────────────────────────────────────────────────┘

$ curl -v -X DELETE http://localhost:8080/uploads/Makefile
*   Trying 127.0.0.1:8080...
* Connected to localhost (127.0.0.1) port 8080 (#0)
> DELETE /uploads/Makefile HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/7.68.0
> Accept: * / *
>
* Mark bundle as not supporting multiuse
< HTTP/1.1 204 No Content
< Date: Mon, 13 Jan 2026 10:00:00 GMT
< Server: webserv/1.0
< Connection: keep-alive
<
* Connection #0 to host localhost left intact
```

## Error Cases

```cpp
┌─────────────────────────────────────────────────────────────────────────────┐
│                    ERROR CASE 1: File Not Found (404)                       │
└─────────────────────────────────────────────────────────────────────────────┘

handleDelete(request, location, server)
    │
    ├──► path = resolvePath("/uploads/Makefile", location)
    │        └──► Returns: "www/uploads/Makefile"
    │
    ├──► stat("www/uploads/Makefile", &pathStat)
    │        │
    │        └──► Returns: -1 (file does not exist!)
    │             errno = ENOENT (No such file or directory)
    │
    └──► return errorResponse(404, server)
             │
             └──► Response with:
                  HTTP/1.1 404 Not Found
                  Content-Type: text/html

                  <html>...<h1>404</h1><p>Not Found</p>...</html>


┌─────────────────────────────────────────────────────────────────────────────┐
│                    ERROR CASE 2: Is a Directory (409 Conflict)              │
└─────────────────────────────────────────────────────────────────────────────┘

Request: DELETE /uploads HTTP/1.1
(Attempting to delete the uploads directory itself)

handleDelete(request, location, server)
    │
    ├──► path = resolvePath("/uploads", location)
    │        └──► Returns: "www/uploads"
    │
    ├──► stat("www/uploads", &pathStat)
    │        │
    │        └──► Returns: 0 (exists)
    │             pathStat.st_mode = S_IFDIR | 0755  (it is a directory!)
    │
    ├──► S_ISDIR(pathStat.st_mode)
    │        │
    │        └──► true ✓ (it IS a directory)
    │
    └──► return errorResponse(409)  // Conflict
             │
             └──► Response with:
                  HTTP/1.1 409 Conflict
                  Content-Type: text/html

                  <html>...<h1>409</h1><p>Conflict</p>...</html>


┌─────────────────────────────────────────────────────────────────────────────┐
│                    ERROR CASE 3: Permission Denied (403 Forbidden)          │
└─────────────────────────────────────────────────────────────────────────────┘

handleDelete(request, location, server)
    │
    ├──► path = resolvePath("/uploads/protected_file", location)
    │        └──► Returns: "www/uploads/protected_file"
    │
    ├──► stat("www/uploads/protected_file", &pathStat)
    │        └──► Returns: 0 (exists)
    │
    ├──► S_ISDIR(pathStat.st_mode) → false (regular file)
    │
    ├──► unlink("www/uploads/protected_file")
    │        │
    │        └──► Returns: -1 (failed!)
    │             errno = EACCES (Permission denied)
    │             (File or directory is read-only or owned by another user)
    │
    └──► return errorResponse(403)  // Forbidden
             │
             └──► Response with:
                  HTTP/1.1 403 Forbidden
                  Content-Type: text/html

                  <html>...<h1>403</h1><p>Forbidden</p>...</html>


┌─────────────────────────────────────────────────────────────────────────────┐
│                    ERROR CASE 4: Method Not Allowed (405)                   │
└─────────────────────────────────────────────────────────────────────────────┘

Request: DELETE /files/document.txt HTTP/1.1
(Location /files does not allow DELETE method)

router.route(request, serverPort=8080)
    │
    ├──► findLocation(server, "/files/document.txt", "DELETE")
    │        │
    │        └──► Returns: &LocationConfig{
    │                        path = "/files",
    │                        allowed_methods = {GET}  ← Only GET allowed!
    │                      }
    │
    ├──► isMethodAllowed("DELETE", location)
    │        │
    │        └──► location.allowed_methods.count("DELETE") > 0 → false ✗
    │
    └──► return errorResponse(405, server)  // Method Not Allowed
             │
             └──► Response with:
                  HTTP/1.1 405 Method Not Allowed
                  Content-Type: text/html

                  <html>...<h1>405</h1><p>Method Not Allowed</p>...</html>
```

## Filesystem State Before/After DELETE

```cpp
┌─────────────────────────────────────────────────────────────────────────────┐
│                    FILESYSTEM STATE CHANGES                                 │
└─────────────────────────────────────────────────────────────────────────────┘

BEFORE DELETE:
══════════════════════════════════════════════════
www/
├── index.html
├── uploads/
│   ├── Makefile     ← File exists (1500 bytes)
│   ├── image.png
│   └── document.pdf
├── files/
│   └── ...
└── errors/
    ├── 404.html
    └── 500.html

$ ls -la www/uploads/
-rw-r--r--  1 user user  1500 Jan 13 09:00 Makefile
-rw-r--r--  1 user user 25000 Jan 12 14:30 image.png
-rw-r--r--  1 user user 50000 Jan 11 10:00 document.pdf


AFTER DELETE:
══════════════════════════════════════════════════
www/
├── index.html
├── uploads/
│   ├── image.png
│   └── document.pdf
├── files/
│   └── ...
└── errors/
    ├── 404.html
    └── 500.html

$ ls -la www/uploads/
-rw-r--r--  1 user user 25000 Jan 12 14:30 image.png
-rw-r--r--  1 user user 50000 Jan 11 10:00 document.pdf

Makefile is DELETED! ✓
```cpp

## Summary: System Calls for DELETE Request

| Phase | System Call | Parameters | Returns |
|-------|-------------|------------|---------|
| **Accept** | `accept` | `(3, &addr, &len)` | 5 |
| | `fcntl` | `(5, F_GETFL)` | flags |
| | `fcntl` | `(5, F_SETFL, O_NONBLOCK)` | 0 |
| | `epoll_ctl` | `(4, ADD, 5, EPOLLIN)` | 0 |
| **Read** | `epoll_wait` | `(4, events, 64, 1000)` | 1 |
| | `recv` | `(5, buf, 8192, 0)` | ~85 |
| **Route** | `stat` | `("www/uploads/Makefile", &st)` | 0 |
| **Delete** | `unlink` | `("www/uploads/Makefile")` | **0 (success!)** |
| | `time` | `(NULL)` | timestamp |
| **Write** | `epoll_ctl` | `(4, MOD, 5, EPOLLOUT)` | 0 |
| | `epoll_wait` | `(4, events, 64, 1000)` | 1 |
| | `send` | `(5, response, ~110, MSG_NOSIGNAL)` | ~110 |
| | `epoll_ctl` | `(4, MOD, 5, EPOLLIN)` | 0 |

## Comparison: DELETE vs Other Methods

| Aspect | GET | POST (upload) | DELETE |
|--------|-----|---------------|--------|
| **Has Request Body** | No | Yes (file data) | No |
| **File I/O** | `open`, `read`, `close` | `open`, `write`, `close` | `stat`, `unlink` |
| **Key System Call** | `read()` file | `write()` file | **`unlink()`** file |
| **Response Status** | 200 OK | 201 Created | **204 No Content** |
| **Response Body** | File content | Success message | **None** |
| **Modifies Filesystem** | No | Yes (creates file) | **Yes (removes file)** |

The key system call for DELETE is **`unlink()`**, which removes the directory entry for the file. The 204 No Content response indicates success without returning any content.
