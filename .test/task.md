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
