# Signal Flow: File Upload (multipart/form-data POST)

Here's the complete signal flow when uploading a file with `curl -v -F "file=@Makefile" http://localhost:8080/uploads`:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    CONNECTION & INITIAL READ                                │
└─────────────────────────────────────────────────────────────────────────────┘

epoll_wait(4, events, 64, 1000) → 1
    │
    ▼
accept(3, &clientAddr, &clientLen) → fd=5
fcntl(5, F_GETFL) → flags
fcntl(5, F_SETFL, flags | O_NONBLOCK) → 0
epoll_ctl(4, EPOLL_CTL_ADD, 5, {EPOLLIN, fd=5}) → 0
Connection conn(5, clientAddr, serverPort=8080)


┌─────────────────────────────────────────────────────────────────────────────┐
│                    REQUEST READING (Multiple recv() calls)                  │
└─────────────────────────────────────────────────────────────────────────────┘

curl sends multipart POST request:
────────────────────────────────────────────────────────────────
POST /uploads HTTP/1.1
Host: localhost:8080
User-Agent: curl/7.68.0
Accept: */*
Content-Length: 1847
Content-Type: multipart/form-data; boundary=------------------------abc123xyz

--------------------------abc123xyz
Content-Disposition: form-data; name="file"; filename="Makefile"
Content-Type: application/octet-stream

# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::   #
#    Makefile                                           :+:      :+:    :+:   #
# ... (rest of Makefile content) ...
# **************************************************************************** #

--------------------------abc123xyz--
────────────────────────────────────────────────────────────────
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
             ├──► recv(5, buffer[8192], 8192, 0)
             │        │
             │        └──► Returns: 1847 bytes (or multiple calls if large)
             │
             ├──► _readBuffer.append(buffer, 1847)
             │
             └──► parseRequest()
                      │
                      └──► _request->parse(_readBuffer)


┌─────────────────────────────────────────────────────────────────────────────┐
│                    REQUEST PARSING (Headers + Body)                         │
└─────────────────────────────────────────────────────────────────────────────┘

_request->parse(_readBuffer)
    │
    │  ┌─────────────────────────────────────────────────────────────────┐
    │  │                  PARSE_REQUEST_LINE                             │
    │  └─────────────────────────────────────────────────────────────────┘
    │
    ├──► requestLine = "POST /uploads HTTP/1.1"
    │        │
    │        └──► parseRequestLine()
    │                 ├──► _method = "POST"
    │                 ├──► _uri = "/uploads"
    │                 ├──► _path = "/uploads"
    │                 ├──► _queryString = ""
    │                 └──► _httpVersion = "HTTP/1.1"
    │
    │  ┌─────────────────────────────────────────────────────────────────┐
    │  │                  PARSE_HEADERS                                  │
    │  └─────────────────────────────────────────────────────────────────┘
    │
    ├──► Parse headers:
    │        _headers["host"] = "localhost:8080"
    │        _headers["user-agent"] = "curl/7.68.0"
    │        _headers["accept"] = "*/*"
    │        _headers["content-length"] = "1847"
    │        _headers["content-type"] = "multipart/form-data; boundary=------------------------abc123xyz"
    │
    ├──► Empty line found → end of headers
    │
    ├──► Check Content-Length: "1847"
    │        _contentLength = 1847
    │
    ├──► _state = PARSE_BODY
    │
    │  ┌─────────────────────────────────────────────────────────────────┐
    │  │                  PARSE_BODY (Content-Length)                    │
    │  └─────────────────────────────────────────────────────────────────┘
    │
    ├──► bytesNeeded = 1847 - 0 = 1847
    ├──► bytesAvailable = 1600 (remaining in buffer after headers)
    ├──► _body.append(buffer, 1600)
    ├──► _bodyBytesRead = 1600
    │
    └──► Returns: false (need more data, 247 bytes remaining)


┌─────────────────────────────────────────────────────────────────────────────┐
│                    CONTINUE READING BODY                                    │
└─────────────────────────────────────────────────────────────────────────────┘

epoll_wait(4, events, 64, 1000) → 1
events[0] = {events=EPOLLIN, data.fd=5}
    │
    ▼
handleClientEvent(clientFd=5, events=EPOLLIN)
    │
    └──► conn.readData()
             │
             ├──► recv(5, buffer, 8192, 0) → 247 bytes
             │
             ├──► _readBuffer.append(buffer, 247)
             │
             └──► parseRequest()
                      │
                      └──► _request->parse(_readBuffer)
                               │
                               ├──► _state == PARSE_BODY
                               ├──► bytesNeeded = 1847 - 1600 = 247
                               ├──► _body.append(remaining, 247)
                               ├──► _bodyBytesRead = 1847 ✓ (complete!)
                               │
                               └──► _state = PARSE_COMPLETE
                                    Returns: true
    │
    ▼
conn.hasCompleteRequest() → true


┌─────────────────────────────────────────────────────────────────────────────┐
│                    UPDATE MAX BODY SIZE (Location-based)                    │
└─────────────────────────────────────────────────────────────────────────────┘

handleClientEvent continues:
    │
    ├──► conn.needsMaxBodySizeUpdate() → true
    │
    ├──► Router::findLocation(server, "/uploads", "POST")
    │        │
    │        └──► Returns: &LocationConfig{
    │                        path="/uploads",
    │                        root="www",
    │                        upload_path="www/uploads",
    │                        allowed_methods={GET, POST, DELETE},
    │                        client_max_body_size=10485760  // 10M
    │                      }
    │
    ├──► serverConf->getMaxBodySize(location) → 10485760
    │
    ├──► conn.setMaxBodySize(10485760)
    │        └──► request->setMaxBodySize(10485760)
    │
    └──► conn.markMaxBodySizeUpdated()


┌─────────────────────────────────────────────────────────────────────────────┐
│                    REQUEST PROCESSING (File Upload)                         │
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
                  │        └──► Returns: &servers[0]
                  │
                  ├──► requestPath = "/uploads"
                  ├──► method = "POST"
                  │
                  ├──► findLocation(server, "/uploads", "POST")
                  │        │
                  │        ├──► Check location "/" : matchLength = 1
                  │        ├──► Check location "/uploads":
                  │        │        "/uploads" == "/uploads" ✓
                  │        │        matchLength = 8 ← BEST MATCH
                  │        │
                  │        └──► Returns: &LocationConfig{
                  │                        path="/uploads",
                  │                        root="www",
                  │                        upload_path="www/uploads",
                  │                        allowed_methods={GET, POST, DELETE},
                  │                        autoindex=true,
                  │                        client_max_body_size=10485760
                  │                      }
                  │
                  ├──► location->redirect_url.empty() → true
                  │
                  ├──► isMethodAllowed("POST", location)
                  │        │
                  │        └──► location.allowed_methods.count("POST") > 0 → true ✓
                  │
                  ├──► resolvePath("/uploads", location) → "www/uploads"
                  │
                  ├──► isCgiRequest("www/uploads", location) → false
                  │
                  ├──► method == "POST" → true
                  │
                  └──► handlePost(request, location, server)


┌─────────────────────────────────────────────────────────────────────────────┐
│                    handlePost() - File Upload Processing                    │
└─────────────────────────────────────────────────────────────────────────────┘

handlePost(request, location, server)
    │
    │  ┌─────────────────────────────────────────────────────────────────┐
    │  │   Step 1: Check upload directory                                │
    │  └─────────────────────────────────────────────────────────────────┘
    │
    ├──► location.upload_path = "www/uploads" (not empty) ✓
    │
    ├──► Utils::directoryExists("www/uploads")
    │        │
    │        └──► stat("www/uploads", &st)
    │                 Returns: 0
    │                 S_ISDIR(st.st_mode) → true ✓
    │
    │  ┌─────────────────────────────────────────────────────────────────┐
    │  │   Step 2: Determine Content-Type                                │
    │  └─────────────────────────────────────────────────────────────────┘
    │
    ├──► contentType = request.getHeader("Content-Type")
    │        └──► "multipart/form-data; boundary=------------------------abc123xyz"
    │
    ├──► contentTypeLower = "multipart/form-data; boundary=..."
    │
    ├──► Utils::startsWith(contentTypeLower, "multipart/form-data") → true
    │
    │  ┌─────────────────────────────────────────────────────────────────┐
    │  │   Step 3: Extract boundary                                      │
    │  └─────────────────────────────────────────────────────────────────┘
    │
    ├──► Utils::extractBoundary(contentType)
    │        │
    │        ├──► Find "boundary=" in Content-Type
    │        ├──► Extract "------------------------abc123xyz"
    │        │
    │        └──► Returns: "------------------------abc123xyz"
    │
    │  ┌─────────────────────────────────────────────────────────────────┐
    │  │   Step 4: Parse multipart body                                  │
    │  └─────────────────────────────────────────────────────────────────┘
    │
    ├──► Utils::parseMultipart(request.getBody(), boundary)
    │        │
    │        │   Request body structure:
    │        │   ─────────────────────────────────────────────────────────
    │        │   --------------------------abc123xyz\r\n
    │        │   Content-Disposition: form-data; name="file"; filename="Makefile"\r\n
    │        │   Content-Type: application/octet-stream\r\n
    │        │   \r\n
    │        │   # **************************************************************************** #\n
    │        │   #    Makefile                                           :+:      :+:    :+:   #\n
    │        │   ... (file content) ...\n
    │        │   # **************************************************************************** #\n
    │        │   --------------------------abc123xyz--\r\n
    │        │   ─────────────────────────────────────────────────────────
    │        │
    │        ├──► Find boundary markers:
    │        │        "--" + boundary = "--------------------------abc123xyz"
    │        │
    │        ├──► For each part between boundaries:
    │        │        │
    │        │        ├──► Parse part headers:
    │        │        │        "Content-Disposition: form-data; name=\"file\"; filename=\"Makefile\""
    │        │        │        "Content-Type: application/octet-stream"
    │        │        │
    │        │        ├──► Extract Content-Disposition:
    │        │        │        name = "file"
    │        │        │        filename = "Makefile"
    │        │        │
    │        │        ├──► Find empty line (end of part headers)
    │        │        │
    │        │        └──► Extract part body (file content)
    │        │
    │        └──► Returns: std::vector<MultipartPart>{
    │                 MultipartPart{
    │                     name = "file",
    │                     filename = "Makefile",
    │                     contentType = "application/octet-stream",
    │                     data = "# ******************** ... (Makefile content)"
    │                 }
    │             }
    │
    │  ┌─────────────────────────────────────────────────────────────────┐
    │  │   Step 5: Process each file part                                │
    │  └─────────────────────────────────────────────────────────────────┘
    │
    ├──► parts.empty() → false ✓
    │
    ├──► For each part in parts:
    │        │
    │        ├──► part.filename = "Makefile" (not empty) → It's a file!
    │        │
    │        ├──► Utils::sanitizeFilename("Makefile")
    │        │        │
    │        │        ├──► Remove path separators (/, \)
    │        │        ├──► Remove dangerous characters
    │        │        ├──► Prevent directory traversal (..)
    │        │        │
    │        │        └──► Returns: "Makefile" (already safe)
    │        │
    │        ├──► Build save path:
    │        │        savePath = "www/uploads"
    │        │        savePath += "/" → "www/uploads/"
    │        │        savePath += "Makefile" → "www/uploads/Makefile"
    │        │
    │        ├──► std::ofstream outFile("www/uploads/Makefile", ios::binary)
    │        │        │
    │        │        └──► open("www/uploads/Makefile", O_WRONLY|O_CREAT|O_TRUNC, 0644)
    │        │                 Returns: fd=6
    │        │
    │        ├──► outFile.is_open() → true ✓
    │        │
    │        ├──► outFile.write(part.data.c_str(), part.data.length())
    │        │        │
    │        │        └──► write(6, fileContent, 1500)
    │        │                 Returns: 1500 (bytes written)
    │        │
    │        ├──► outFile.fail() → false ✓
    │        │
    │        ├──► outFile.close()
    │        │        │
    │        │        └──► close(6)
    │        │
    │        └──► savedFiles.push_back("www/uploads/Makefile")
    │
    │  ┌─────────────────────────────────────────────────────────────────┐
    │  │   Step 6: Build success response                                │
    │  └─────────────────────────────────────────────────────────────────┘
    │
    ├──► savedFiles.empty() → false ✓
    │
    ├──► Response response
    │        response.setStatus(201, "Created")
    │
    ├──► Set Location header:
    │        locationUrl = "www/uploads/Makefile"
    │        Utils::startsWith(locationUrl, "www") → true
    │        locationUrl = "/uploads/Makefile"  // Strip "www" prefix
    │        response.setHeader("Location", "/uploads/Makefile")
    │
    ├──► Build response body:
    │        body = "Upload successful!\n\n"
    │        body += "Files saved:\n"
    │        body += "  - www/uploads/Makefile\n"
    │
    ├──► response.setContentType("text/plain")
    ├──► response.setBody(body)
    ├──► response.addStandardHeaders()
    │        ├──► setHeader("Date", "Mon, 13 Jan 2026 10:00:00 GMT")
    │        ├──► setHeader("Server", "webserv/1.0")
    │        └──► setHeader("Connection", "keep-alive")
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
    │             "HTTP/1.1 201 Created\r\n"
    │             "Date: Mon, 13 Jan 2026 10:00:00 GMT\r\n"
    │             "Server: webserv/1.0\r\n"
    │             "Content-Type: text/plain\r\n"
    │             "Location: /uploads/Makefile\r\n"
    │             "Connection: keep-alive\r\n"
    │             "Content-Length: 58\r\n"
    │             "\r\n"
    │             "Upload successful!\n\n"
    │             "Files saved:\n"
    │             "  - www/uploads/Makefile\n"
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
    │
    ▼
epoll_wait(4, events, 64, 1000) → 1
events[0] = {events=EPOLLOUT, data.fd=5}
    │
    ▼
conn.writeData()
    │
    ├──► send(5, response_data, ~280, MSG_NOSIGNAL)
    │        │
    │        └──► Returns: ~280 (all bytes sent)
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

$ curl -v -F "file=@Makefile" http://localhost:8080/uploads
*   Trying 127.0.0.1:8080...
* Connected to localhost (127.0.0.1) port 8080 (#0)
> POST /uploads HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/7.68.0
> Accept: */*
> Content-Length: 1847
> Content-Type: multipart/form-data; boundary=------------------------abc123xyz
>
* We are completely uploaded and fine
* Mark bundle as not supporting multiuse
< HTTP/1.1 201 Created
< Date: Mon, 13 Jan 2026 10:00:00 GMT
< Server: webserv/1.0
< Content-Type: text/plain
< Location: /uploads/Makefile
< Connection: keep-alive
< Content-Length: 58
<
Upload successful!

Files saved:
  - www/uploads/Makefile
* Connection #0 to host localhost left intact


┌─────────────────────────────────────────────────────────────────────────────┐
│                    FILE SYSTEM STATE AFTER UPLOAD                           │
└─────────────────────────────────────────────────────────────────────────────┘

www/
├── uploads/
│   └── Makefile    ← NEW FILE CREATED!
├── index.html
├── cgi-bin/
│   └── py/
│       └── hello.py
└── errors/
    ├── 404.html
    └── 500.html
```

## Multipart Parsing Detail

```
┌─────────────────────────────────────────────────────────────────────────────┐
│               MULTIPART/FORM-DATA STRUCTURE                                 │
└─────────────────────────────────────────────────────────────────────────────┘

Raw body bytes sent by curl:
═══════════════════════════════════════════════════════════════════════════════

--------------------------abc123xyz\r\n                    ← Boundary start
Content-Disposition: form-data; name="file"; filename="Makefile"\r\n  ← Part header
Content-Type: application/octet-stream\r\n                 ← Part header
\r\n                                                       ← Empty line (end headers)
# **************************************************************************** #\n
#                                                                              #\n
#                                                         :::      ::::::::   #\n
#    Makefile                                           :+:      :+:    :+:   #\n
#                                                    +:+ +:+         +:+     #\n
# ... (actual Makefile content) ...
# **************************************************************************** #\n
--------------------------abc123xyz--\r\n                  ← Final boundary (-- suffix)

═══════════════════════════════════════════════════════════════════════════════

Parsing algorithm:

1. Extract boundary from Content-Type header:
   "multipart/form-data; boundary=------------------------abc123xyz"
                                  └─────────────────────────────────┘
                                          boundary string

2. Split body by "--" + boundary:
   "--" + "------------------------abc123xyz" = "--------------------------abc123xyz"

3. For each part:
   a. Find "\r\n\r\n" to separate headers from content
   b. Parse Content-Disposition to get name and filename
   c. Everything after "\r\n\r\n" until next boundary is the file data

4. Handle final boundary:
   Final boundary ends with "--" suffix: "--------------------------abc123xyz--"
```

## Summary: System Calls for File Upload

| Phase | System Call | Parameters | Returns |
|-------|-------------|------------|---------|
| **Accept** | `accept` | `(3, &addr, &len)` | 5 |
| | `fcntl` | `(5, F_SETFL, O_NONBLOCK)` | 0 |
| | `epoll_ctl` | `(4, ADD, 5, EPOLLIN)` | 0 |
| **Read Headers** | `recv` | `(5, buf, 8192, 0)` | ~500 |
| **Read Body** | `recv` | `(5, buf, 8192, 0)` | ~1347 |
| | `recv` | `(5, buf, 8192, 0)` | ~247 (if needed) |
| **Validate Dir** | `stat` | `("www/uploads", &st)` | 0 |
| **Write File** | `open` | `("www/uploads/Makefile", O_WRONLY|O_CREAT)` | 6 |
| | `write` | `(6, fileContent, 1500)` | 1500 |
| | `close` | `(6)` | 0 |
| | `time` | `(NULL)` | timestamp |
| **Write Response** | `epoll_ctl` | `(4, MOD, 5, EPOLLOUT)` | 0 |
| | `epoll_wait` | `(4, events, 64, 1000)` | 1 |
| | `send` | `(5, response, ~280, MSG_NOSIGNAL)` | ~280 |
| | `epoll_ctl` | `(4, MOD, 5, EPOLLIN)` | 0 |

## Key Differences from Static File Request

| Aspect | GET /index.html | POST /uploads (file upload) |
|--------|-----------------|----------------------------|
| **Method** | GET | POST |
| **Has Body** | No | Yes (multipart/form-data) |
| **Content-Type** | N/A | multipart/form-data; boundary=... |
| **Content-Length** | N/A | Size of multipart body |
| **Parsing** | Headers only | Headers + body parsing |
| **Body Handling** | None | Parse multipart, extract files |
| **File I/O** | Read file | Write file |
| **Response Status** | 200 OK | 201 Created |
| **Location Header** | None | Path to uploaded file |
