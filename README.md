# The Quest: Webserv

Build a **minimal HTTP/1.1 web server** in **C++98**, using **non-blocking I/O** with a **single** `poll`/`select`/`epoll` loop, able to:
1. Accept TCP connections on one or more ports
2. Parse HTTP requests (at least GET, POST, DELETE methods)
3. Serve static files from the filesystem
4. Generate directory listings
5. Perform redirections
6. Run CGI scripts (e.g., PHP, Python)
7. Handle file uploads via POST requests
8. Produce correct HTTP responses and error pages
9. Stay responsive under high load, without blocking on any client


We are going to building a simplified version of **NGINX** or **Apache**.
The program is a **Web Server**. It runs in the background and listens for incoming connections. When a Web Browser (Chrome, Firefox) tries to access a page hosted by the server, we must:
1.  Receive the request.
2.  Parse it (understand what the browser wants).
3.  Find the file (or run a script).
4.  Send the result back to the browser.

The server must be launched with a config file (e.g., `./webserv myconfig.conf`),
or it falls back to a default one.


## The Core Concepts

### 1. Sockets, Bind, Listen, Accept

1.  **`socket()`**: endpoint for communication.
2.  **`bind()`**: attach socket to IP:port (e.g., `127.0.0.1:8080`).
3.  **`listen()`**: mark socket as passive (ready to accept connections).
4.  **`accept()`**: accept an incoming client connection. **Crucial:** `accept` returns a *new* socket (File Descriptor) specifically for *that* conversation. We keep the original socket to listen for *more* calls.
5.  **`recv()` / `send()`**: read from / write to the connected socket. **Critical restriction**: We CANNOT have blocking calls like `read()/recv()` or `write()/send()` on sockets/pipes without `poll()` first telling us they're ready. (This will result in a grade of 0.)
6.  **`Select/Poll/Epoll/Kqueue`**: These are I/O multiplexing functions. They monitor multiple file descriptors (sockets, pipes) at once for readiness (e.g., "is there data to read?").
    - `select()`: Basic, but limited (handles up to ~1024 FDs inefficiently).
    - `poll()`: Improved over select, scales better.
    - `epoll()` (Linux): High-performance for many connections.
    - `kqueue()` (BSD/macOS): Similar to epoll.


### 2. The Event Loop: Non-blocking I/O with poll/select/epoll

The key restriction: we must use only ONE `poll()` call for all I/O operations.
Why non-blocking? If we have 100 clients connected and we would use blocking I/O:

Client 1 sends data → we read it (blocks until complete)
While reading from Client 1, Clients 2-100 are waiting
This is inefficient!

`poll()` solution:

We register all file descriptors (sockets) we're interested in
`poll()` monitors them ALL at once
It tells us which ones are ready to read/write
We only read/write when data is actually ready

```cpp

// Simplified concept:

struct pollfd fds[MAX_CLIENTS];
// Add listening socket and all client sockets to fds[]

while (true) {
    poll(fds, num_fds, timeout); // ONE call monitors everything

    // Check which sockets are ready
    for (each fd in fds) {
        if (ready for reading) → read data
        if (ready for writing) → write response
    }
}
```

### 3. **HTTP Protocol**

**HTTP/1.0 vs HTTP/1.1** (we need to support both):

**HTTP/1.0**:
- Each request opens a new connection
- Connection closes after response
- Simpler to implement

**HTTP/1.1**:
- **Keep-alive**: Connection stays open for multiple requests
- **Chunked transfer encoding**: Data sent in chunks
- **Host header**: Required (allows multiple sites on one IP)
- More efficient but more complex

We need to implement HTTP/1.1 support, with fallback possibility to HTTP/1.0.


### 4. **HTTP Methods**

**GET**: Retrieve a resource (file, webpage)
```
GET /index.html HTTP/1.1
Host: localhost:8080
```

**POST**: Send data to server (form submissions, file uploads)
```
POST /upload HTTP/1.1
Content-Length: 1234
Content-Type: multipart/form-data

[binary file data]
```

**DELETE**: Remove a resource
```
DELETE /files/document.txt HTTP/1.1
```


### 5. **CGI (Common Gateway Interface)**

CGI lets us execute external programs (PHP, Python) to generate dynamic content.

**How it works**:
1. Client requests `/script.php`
2. Server uses `fork()` to create child process
3. Server sets **environment variables** (REQUEST_METHOD, QUERY_STRING, etc.)
4. Server uses `execve()` to run PHP interpreter
5. Server sends request body to CGI via pipe (stdin)
6. CGI writes response to pipe (stdout)
7. Server reads CGI output and sends to client

**Important**: For chunked requests, we must un-chunk them before sending to CGI. CGI expects EOF to mark end of input.


### 6. Configuration File

Similar to NGINX config, our file might look like:

```javascript

server {
    listen 8080;
    server_name localhost;

    error_page 404 /errors/404.html;
    client_max_body_size 10M;

    location / {
        root /var/www;
        index index.html;
        allow_methods GET POST;
    }

    location /upload {
        allow_methods POST DELETE;
        upload_dir /var/www/uploads;
    }

    location /cgi-bin {
        cgi_extension .php;
        cgi_path /usr/bin/php-cgi;
    }
}
```

We need to parse this file and set up our server accordingly.

---


## Recommended File Structure

```bash

webserv/
    Makefile
    config/
        default.conf
        example.conf
    includes/
        Server.hpp
        Config.hpp
        Connection.hpp
        Request.hpp
        Response.hpp
        Router.hpp
        CGI.hpp
        Utils.hpp
    src/
        main.cpp
        Server.cpp
        Config.cpp
        Connection.cpp
        Request.cpp
        Response.cpp
        Router.cpp
        CGI.cpp
        Utils.cpp
    www/
        index.html
        errors/
        uploads/
        cgi-bin/
```

---


# Webserv Implementation Workflow

## Phase 1: Project Setup & Basic Infrastructure

### Step 1.1: Project Structure Setup
**Goal**: Create the basic project structure

**Tasks**:
- Create directory structure as shown in the subject
- Write a basic Makefile with rules: NAME, all, clean, fclean, re
- Set up C++98 compilation flags: `-Wall -Wextra -Werror -std=c++98`
- Create empty header and source files

**Testing**:
```bash
make all
make clean
make fclean
make re
```

### Step 1.2: Configuration File Parser
**Goal**: Parse and validate the configuration file

**Tasks**:
- Create `Config.hpp` and `Config.cpp`
- Implement configuration file parser (similar to NGINX format)
- Support basic directives:
  - Server blocks with `listen` (host:port)
  - `error_page` directives
  - `client_max_body_size`
  - Location blocks with routes
- Store parsed data in appropriate data structures
- Handle parsing errors gracefully

**Testing**:
```bash
# Create test config files with various formats
./webserv config/test_valid.conf    # Should parse successfully
./webserv config/test_invalid.conf  # Should show error and exit
./webserv                           # Should use default config
```

**Test Config Example**:
```nginx
server {
    listen 8080;
    server_name localhost;
    error_page 404 /errors/404.html;
    client_max_body_size 10M;

    location / {
        root www;
        index index.html;
    }
}
```

---

## Phase 2: Socket Setup & Basic Server

### Step 2.1: Socket Creation & Binding
**Goal**: Create listening sockets for all configured ports

**Tasks**:
- Create `Server.hpp` and `Server.cpp`
- Implement socket creation with `socket()`
- Set socket options with `setsockopt()` (SO_REUSEADDR, SO_REUSEPORT)
- Bind sockets to configured host:port pairs with `bind()`
- Set sockets to non-blocking mode with `fcntl()`
- Start listening with `listen()`
- Support multiple listening ports

**Testing**:
```bash
# Terminal 1
./webserv config/multi_port.conf

# Terminal 2
netstat -tuln | grep 8080  # Check if socket is listening
telnet localhost 8080      # Try to connect
```

### Step 2.2: Poll/Select Implementation
**Goal**: Set up the main event loop with poll/epoll

**Tasks**:
- Choose between `poll()`, `epoll`, or `select()` (recommend epoll for Linux)
- Create the main server loop
- Add listening sockets to the poll mechanism
- Implement basic event loop structure:
  - Monitor for POLLIN events on listening sockets
  - Accept new connections
  - Add client sockets to poll
  - Handle poll errors and timeouts

**Testing**:
```bash
# Terminal 1
./webserv

# Terminal 2
nc localhost 8080  # Multiple times to test multiple connections
# Server should accept connections without blocking
```

---

## Phase 3: Connection Management

### Step 3.1: Accept & Track Connections
**Goal**: Accept client connections and manage them

**Tasks**:
- Create `Connection.hpp` and `Connection.cpp`
- Implement `accept()` for new connections
- Set client sockets to non-blocking mode
- Store connection state (socket fd, remote address, timestamp)
- Implement connection timeout mechanism
- Handle connection closure properly

**Testing**:
```bash
# Terminal 1
./webserv

# Terminal 2 - Test multiple connections
for i in {1..10}; do nc localhost 8080 & done
# All should be accepted

# Test timeout
nc localhost 8080
# Wait without sending data - should timeout and disconnect
```

### Step 3.2: Read from Clients
**Goal**: Read HTTP requests from clients non-blockingly

**Tasks**:
- Monitor client sockets for POLLIN events
- Use `recv()` to read data when available
- Buffer incomplete requests (requests may arrive in chunks)
- Detect complete HTTP requests (check for "\r\n\r\n" separator)
- Handle partial reads and EAGAIN/EWOULDBLOCK
- Handle client disconnections (recv returns 0)

**Testing**:
```bash
# Send partial request
echo -ne "GET / HTTP/1.1\r\n" | nc localhost 8080
# Wait, then send rest
echo -ne "Host: localhost\r\n\r\n" | nc localhost 8080

# Send complete request
printf "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost 8080
```

---

## Phase 4: HTTP Request Parsing

### Step 4.1: Request Line Parser
**Goal**: Parse HTTP request line (method, URI, version)

**Tasks**:
- Create `Request.hpp` and `Request.cpp`
- Parse request line: `METHOD URI HTTP/VERSION`
- Extract and validate:
  - HTTP method (GET, POST, DELETE)
  - Request URI (path + query string)
  - HTTP version (HTTP/1.0, HTTP/1.1)
- Handle malformed request lines (return 400 Bad Request)

**Testing**:
```bash
# Valid requests
printf "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost 8080
printf "POST /upload HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost 8080

# Invalid requests (should get 400)
printf "INVALID /index.html HTTP/1.1\r\n\r\n" | nc localhost 8080
printf "GET\r\n\r\n" | nc localhost 8080
```

### Step 4.2: Headers Parser
**Goal**: Parse HTTP headers

**Tasks**:
- Parse headers line by line (format: `Key: Value`)
- Store headers in a map/dictionary structure
- Handle multi-line headers
- Case-insensitive header names
- Parse important headers:
  - Host (required in HTTP/1.1)
  - Content-Length
  - Content-Type
  - Transfer-Encoding (chunked)
  - Connection (keep-alive, close)

**Testing**:
```bash
cat << EOF | nc localhost 8080
GET / HTTP/1.1
Host: localhost
User-Agent: TestClient/1.0
Accept: */*

EOF
```

### Step 4.3: Request Body Handling
**Goal**: Read and process request bodies

**Tasks**:
- Implement Content-Length based body reading
- Implement chunked transfer encoding parsing
- Enforce `client_max_body_size` limit (413 Payload Too Large)
- Buffer body data properly
- Handle POST requests with body

**Testing**:
```bash
# Content-Length body
cat << EOF | nc localhost 8080
POST /upload HTTP/1.1
Host: localhost
Content-Length: 13
Content-Type: text/plain

Hello, World!
EOF

# Chunked encoding
cat << EOF | nc localhost 8080
POST /upload HTTP/1.1
Host: localhost
Transfer-Encoding: chunked

5
Hello
0

EOF

# Body too large (should get 413)
dd if=/dev/zero bs=1M count=100 | nc localhost 8080
```

---

## Phase 5: Routing & Response Building

### Step 5.1: Router Implementation
**Goal**: Match requests to location blocks

**Tasks**:
- Create `Router.hpp` and `Router.cpp`
- Implement URI to location matching algorithm
- Find the most specific matching location
- Apply location-specific configuration:
  - Allowed methods
  - Root directory
  - Redirections
  - Directory listing
  - Default index files
- Handle method not allowed (405)

**Testing**:
```bash
# Test different routes
printf "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost 8080
printf "GET /api/test HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost 8080
printf "POST / HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost 8080
```

### Step 5.2: Static File Serving (GET)
**Goal**: Serve static files

**Tasks**:
- Create `Response.hpp` and `Response.cpp`
- Resolve file paths (combine root + URI)
- Security: prevent directory traversal (../)
- Use `stat()` to check file existence and type
- Handle directories:
  - Serve index file if configured
  - Show directory listing if enabled
  - Return 403 Forbidden if disabled
- Read file content using `open()`, `read()`, `close()`
- Determine Content-Type from file extension
- Build HTTP response with proper headers

**Testing**:
```bash
# Create test files
mkdir -p www
echo "<h1>Hello World</h1>" > www/index.html
mkdir -p www/subdir
echo "test" > www/subdir/file.txt

# Test file serving
curl -v http://localhost:8080/
curl -v http://localhost:8080/index.html
curl -v http://localhost:8080/subdir/file.txt

# Test directory listing
curl -v http://localhost:8080/subdir/

# Test 404
curl -v http://localhost:8080/nonexistent.html
```

---

## Phase 6: HTTP Response & Writing

### Step 6.1: Response Generation
**Goal**: Build complete HTTP responses

**Tasks**:
- Generate status line: `HTTP/1.1 STATUS_CODE REASON_PHRASE`
- Add standard headers:
  - Content-Type
  - Content-Length
  - Date
  - Server
  - Connection (keep-alive or close)
- Format response: status line + headers + blank line + body
- Implement default error pages for all error codes

**Testing**:
```bash
# Check response format
curl -v http://localhost:8080/ 2>&1 | head -20

# Test various status codes
curl -I http://localhost:8080/              # 200
curl -I http://localhost:8080/notfound      # 404
curl -I -X POST http://localhost:8080/      # 405 if not allowed
```

### Step 6.2: Non-blocking Write
**Goal**: Write responses to clients non-blockingly

**Tasks**:
- Monitor client sockets for POLLOUT events
- Use `send()` to write response data
- Handle partial writes (EAGAIN/EWOULDBLOCK)
- Buffer unsent data for next POLLOUT event
- Track bytes sent vs total response size
- Close connection when response is complete (if Connection: close)
- Support keep-alive connections

**Testing**:
```bash
# Test large file serving
dd if=/dev/zero of=www/large.bin bs=1M count=50
curl http://localhost:8080/large.bin > /dev/null

# Test keep-alive
curl -v http://localhost:8080/ http://localhost:8080/index.html
```

---

## Phase 7: HTTP Methods Implementation

### Step 7.1: DELETE Method
**Goal**: Allow clients to delete files

**Tasks**:
- Check if DELETE is allowed for the route
- Resolve file path
- Security checks (prevent deleting outside allowed directories)
- Use `unlink()` to delete the file
- Return appropriate status:
  - 204 No Content (success)
  - 404 Not Found
  - 403 Forbidden (not allowed to delete)
  - 409 Conflict (if directory)

**Testing**:
```bash
# Create test file
echo "delete me" > www/uploads/test.txt

# Delete it
curl -X DELETE http://localhost:8080/uploads/test.txt

# Verify deletion
curl http://localhost:8080/uploads/test.txt  # Should get 404

# Try to delete non-existent file
curl -X DELETE http://localhost:8080/uploads/nonexistent.txt  # 404
```

### Step 7.2: POST Method & File Upload
**Goal**: Handle file uploads

**Tasks**:
- Parse multipart/form-data (for file uploads)
- Extract boundary from Content-Type header
- Parse multipart sections (headers + data)
- Save uploaded files to configured upload directory
- Generate unique filenames or use provided names
- Return 201 Created with Location header
- Alternative: support application/x-www-form-urlencoded

**Testing**:
```bash
# Create upload directory
mkdir -p www/uploads

# Test file upload
curl -X POST -F "file=@test.txt" http://localhost:8080/upload

# Verify file was saved
ls www/uploads/

# Test with large file
dd if=/dev/urandom of=large.bin bs=1M count=5
curl -X POST -F "file=@large.bin" http://localhost:8080/upload
```

---

## Phase 8: CGI Implementation

### Step 8.1: CGI Detection & Setup
**Goal**: Detect and prepare CGI execution

**Tasks**:
- Create `CGI.hpp` and `CGI.cpp`
- Detect CGI requests based on file extension (.php, .py, .pl)
- Check if CGI is enabled for the location
- Verify CGI executable exists and is executable
- Prepare CGI environment variables:
  - REQUEST_METHOD
  - QUERY_STRING
  - CONTENT_TYPE
  - CONTENT_LENGTH
  - SCRIPT_FILENAME
  - PATH_INFO
  - SERVER_PROTOCOL
  - GATEWAY_INTERFACE
  - All HTTP headers as HTTP_*

**Testing**:
```bash
# Create test CGI script
cat > www/cgi-bin/test.py << 'EOF'
#!/usr/bin/env python3
print("Content-Type: text/html\r")
print("\r")
print("<h1>CGI Works!</h1>")
EOF
chmod +x www/cgi-bin/test.py

# Test detection
curl http://localhost:8080/cgi-bin/test.py
```

### Step 8.2: CGI Execution
**Goal**: Execute CGI scripts and capture output

**Tasks**:
- Use `pipe()` to create pipes for stdin/stdout
- Use `fork()` to create child process
- In child:
  - Use `dup2()` to redirect stdin/stdout to pipes
  - Use `chdir()` to change to script directory
  - Use `execve()` to execute CGI interpreter
- In parent:
  - Write request body to CGI stdin (if POST)
  - Read CGI output from stdout
  - Use `waitpid()` to wait for child completion
  - Set timeout for CGI execution (504 Gateway Timeout)
- Parse CGI output (headers + body separated by blank line)
- Handle chunked request bodies (un-chunk before sending to CGI)

**Testing**:
```bash
# Test simple CGI
curl http://localhost:8080/cgi-bin/test.py

# Test CGI with query string
curl "http://localhost:8080/cgi-bin/test.py?name=World"

# Test POST to CGI
curl -X POST -d "name=Test" http://localhost:8080/cgi-bin/test.py

# Test PHP CGI (if php-cgi installed)
echo '<?php phpinfo(); ?>' > www/cgi-bin/info.php
curl http://localhost:8080/cgi-bin/info.php
```

### Step 8.3: CGI Error Handling
**Goal**: Handle CGI execution errors

**Tasks**:
- Handle CGI script not found (404)
- Handle CGI not executable (500)
- Handle CGI execution failure (500)
- Handle CGI timeout (504)
- Handle invalid CGI output (502)
- Clean up pipes and child processes properly
- Use `kill()` to terminate hung CGI processes

**Testing**:
```bash
# Test non-existent CGI
curl http://localhost:8080/cgi-bin/nonexistent.py  # 404

# Test non-executable CGI
echo "print('test')" > www/cgi-bin/noexec.py
chmod -x www/cgi-bin/noexec.py
curl http://localhost:8080/cgi-bin/noexec.py  # 500

# Test CGI timeout (create script that sleeps)
cat > www/cgi-bin/slow.py << 'EOF'
#!/usr/bin/env python3
import time
time.sleep(60)
print("Content-Type: text/plain\r\n\r\nDone")
EOF
chmod +x www/cgi-bin/slow.py
curl http://localhost:8080/cgi-bin/slow.py  # Should timeout
```

---

## Phase 9: Advanced Features

### Step 9.1: HTTP Redirections
**Goal**: Implement HTTP redirects

**Tasks**:
- Parse redirect directive from config (return 301/302 URL)
- Generate redirect response (301/302) with Location header
- Support both permanent (301) and temporary (302) redirects

**Testing**:
```bash
# Add to config: location /old { return 301 /new; }
curl -v http://localhost:8080/old
# Should see 301 redirect with Location: /new
```

### Step 9.2: Directory Listing
**Goal**: Show directory contents when enabled

**Tasks**:
- Use `opendir()`, `readdir()`, `closedir()` to read directory
- Generate HTML page with file/directory list
- Show file sizes and types
- Make entries clickable links
- Add parent directory link (..)

**Testing**:
```bash
# Enable directory listing in config
mkdir -p www/files
touch www/files/{file1.txt,file2.html,file3.jpg}
curl http://localhost:8080/files/
# Should see HTML list of files
```

### Step 9.3: Multiple Server Blocks
**Goal**: Support multiple websites on different ports

**Tasks**:
- Parse multiple server blocks from config
- Create listening socket for each unique port
- Route requests to correct server block based on port
- Apply server-specific configuration

**Testing**:
```bash
# Create config with multiple servers on ports 8080, 8081
./webserv config/multi_server.conf

# Terminal 2
curl http://localhost:8080/  # Server 1
curl http://localhost:8081/  # Server 2
```

---

## Phase 10: Error Handling & Robustness

### Step 10.1: Comprehensive Error Handling
**Goal**: Handle all error conditions gracefully

**Tasks**:
- Implement all HTTP error responses:
  - 400 Bad Request
  - 403 Forbidden
  - 404 Not Found
  - 405 Method Not Allowed
  - 413 Payload Too Large
  - 500 Internal Server Error
  - 502 Bad Gateway
  - 504 Gateway Timeout
  - 505 HTTP Version Not Supported
- Custom error pages from config
- Log errors appropriately
- Never crash, always recover

**Testing**:
```bash
# Test each error code
curl http://localhost:8080/nonexistent           # 404
curl -X PATCH http://localhost:8080/             # 405
printf "INVALID\r\n\r\n" | nc localhost 8080     # 400
curl -X POST --data-binary @huge_file ...        # 413
```

### Step 10.2: Connection Timeouts
**Goal**: Implement request/connection timeouts

**Tasks**:
- Track timestamp for each connection
- Close idle connections after timeout period
- Close connections that send data too slowly
- Set timeout for CGI execution
- Clean up resources properly

**Testing**:
```bash
# Connect but don't send anything
nc localhost 8080
# Should disconnect after timeout

# Send partial request slowly
printf "GET / HTTP/1.1\r\n" | nc localhost 8080
# Wait... should timeout
```

### Step 10.3: Resource Limits
**Goal**: Prevent resource exhaustion

**Tasks**:
- Limit maximum number of concurrent connections
- Limit request body size per config
- Limit request header size
- Limit URL length
- Prevent memory leaks (use valgrind)

**Testing**:
```bash
# Test many concurrent connections
for i in {1..1000}; do nc localhost 8080 & done

# Test with valgrind
valgrind --leak-check=full ./webserv config/default.conf
```

---

## Phase 11: Testing & Validation

### Step 11.1: Browser Compatibility
**Goal**: Ensure compatibility with web browsers

**Tasks**:
- Test with Chrome/Firefox/Safari
- Verify HTML rendering
- Test file downloads
- Test form submissions
- Test file uploads via HTML form

**Testing**:
```html
<!-- Create test HTML form -->
<!DOCTYPE html>
<html>
<body>
<form action="/upload" method="post" enctype="multipart/form-data">
    <input type="file" name="file">
    <input type="submit" value="Upload">
</form>
</body>
</html>
```

### Step 11.2: Stress Testing
**Goal**: Ensure server remains stable under load

**Tasks**:
- Use `siege`, `ab` (Apache Bench), or `wrk` for stress testing
- Test with many concurrent connections
- Test with large files
- Test with many requests per second
- Monitor for crashes, hangs, or memory leaks

**Testing**:
```bash
# Apache Bench
ab -n 10000 -c 100 http://localhost:8080/

# Siege
siege -c 100 -t 1M http://localhost:8080/

# Custom script
python3 stress_test.py
```

### Step 11.3: Comparison with NGINX
**Goal**: Verify behavior matches NGINX

**Tasks**:
- Set up NGINX with similar config
- Compare response headers
- Compare status codes
- Compare error handling
- Compare CGI behavior

**Testing**:
```bash
# Start NGINX on port 8082 with similar config
nginx -c /path/to/nginx.conf

# Compare responses
diff <(curl -I http://localhost:8080/) <(curl -I http://localhost:8082/)
```

---

## Phase 12: Final Polish

### Step 12.1: Configuration Validation
**Goal**: Robust config file handling

**Tasks**:
- Validate all config directives
- Provide helpful error messages
- Support comments in config
- Handle edge cases (missing values, duplicates)

### Step 12.2: Documentation
**Goal**: Document your implementation

**Tasks**:
- Write README with:
  - Compilation instructions
  - Usage examples
  - Configuration guide
  - Supported features
- Comment complex code sections
- Create example config files

### Step 12.3: Code Quality
**Goal**: Clean, maintainable code

**Tasks**:
- Follow C++98 standards strictly
- No memory leaks (check with valgrind)
- No undefined behavior (check with sanitizers)
- Consistent code style
- Remove debug code
- Handle all edge cases

**Testing**:
```bash
# Final checks
make re
make clean && make
valgrind --leak-check=full --track-fds=yes ./webserv
clang++ -std=c++98 -fsanitize=address ...
```

---

## Testing Checklist

After implementation, verify:

- ✅ Config file parsing works with various formats
- ✅ Server binds to all configured ports
- ✅ Non-blocking I/O throughout
- ✅ GET serves static files correctly
- ✅ POST uploads files successfully
- ✅ DELETE removes files properly
- ✅ CGI scripts execute (at least one type)
- ✅ Chunked transfer encoding handled
- ✅ Keep-alive connections work
- ✅ Redirections function properly
- ✅ Directory listing works when enabled
- ✅ Error pages display correctly
- ✅ Large files transfer without issues
- ✅ Many concurrent connections handled
- ✅ No memory leaks (valgrind clean)
- ✅ Works with standard browsers
- ✅ Matches NGINX behavior for common cases
- ✅ Server never crashes or hangs
- ✅ Timeouts work correctly
- ✅ Resource limits enforced

---

## Recommended Tools

- **netcat (nc)**: Raw TCP testing
- **curl**: HTTP testing
- **telnet**: Connection testing
- **valgrind**: Memory leak detection
- **gdb**: Debugging
- **strace**: System call tracing
- **ab/siege/wrk**: Load testing
- **tcpdump/wireshark**: Network analysis

## Time Estimates

- Phase 1: 2-3 days
- Phase 2-3: 3-4 days
- Phase 4: 2-3 days
- Phase 5-6: 3-4 days
- Phase 7: 2-3 days
- Phase 8: 4-5 days (CGI is complex!)
- Phase 9: 2-3 days
- Phase 10: 2-3 days
- Phase 11-12: 2-3 days

**Total**: ~25-35 days of focused work

