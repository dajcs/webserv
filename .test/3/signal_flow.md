# Webserv Signal Flow Analysis

## Summary of Function Call Flow for Test Cases

### 1. Basic GET Request (`curl -v http://localhost:8080/`)


```
main()
  └─> Server::init()
        └─> createListeningSocket() for each configured port
  └─> Server::run()  [main event loop]
        └─> epoll_wait() - blocks until event
        └─> acceptNewConnection(listenFd)
              └─> accept() - creates new client socket
              └─> Connection(fd, clientAddr, serverPort)
              └─> addToEpoll(clientFd, EPOLLIN)
        └─> epoll_wait() - client ready to read
        └─> handleClientEvent(clientFd, EPOLLIN)
              └─> Connection::readData()
                    └─> recv() - reads "GET / HTTP/1.1\r\n..."
                    └─> parseRequest()
                          └─> Request::parse()
                                └─> parseRequestLine()
                                └─> parseHeader() [for each header]
              └─> processRequest(conn)
                    └─> Router::route(request, serverPort)
                          └─> findServer(port, hostname)
                          └─> findLocation(server, path)
                          └─> handleGet(request, location)
                                └─> resolvePath(uri, location)
                                └─> serveFile(filepath) or serveDirectory()
                                      └─> open(), read(), close()
                                      └─> Response::ok(body, contentType)
                    └─> Connection::setResponse(response)
              └─> modifyEpoll(clientFd, EPOLLOUT)
        └─> epoll_wait() - client ready to write
        └─> handleClientEvent(clientFd, EPOLLOUT)
              └─> Connection::writeData()
                    └─> send() - writes HTTP response
              └─> handleWriteComplete()
                    └─> reset() [if keep-alive] or close
```


### 2. CGI Request (`curl -v http://localhost:8080/cgi-bin/hello.py`)


```
Router::route()
  └─> isCgiRequest(path, location) - returns true
  └─> CGI::setup(scriptPath)
        └─> validateScript()
        └─> validateInterpreter()
        └─> buildEnvironment()
  └─> CGI::execute()
        └─> pipe() - create stdin/stdout pipes
        └─> fork()
        └─> [Child Process]:
              └─> dup2() - redirect stdin/stdout
              └─> chdir() - change to script directory
              └─> execve(interpreter, argv, envp)
        └─> [Parent Process]:
              └─> write() - send request body to child stdin
              └─> epoll_wait() with timeout
              └─> read() - get CGI output
              └─> parseCgiOutput()
              └─> waitpid() - reap child
              └─> cleanupChild() if timeout
```


### 3. File Upload (`curl -v -F "file=@Makefile" http://localhost:8080/uploads`)


```
Connection::readData()
  └─> recv() - reads POST request with multipart body
  └─> Request::parse()
        └─> parseRequestLine() - "POST /uploads HTTP/1.1"
        └─> parseHeader() - Content-Type: multipart/form-data
        └─> [PARSE_BODY state] - accumulate body bytes

Router::handlePost()
  └─> Utils::extractBoundary(contentType)
  └─> Utils::parseMultipart(body, boundary)
        └─> parseContentDisposition() - get filename
  └─> Utils::sanitizeFilename()
  └─> Utils::generateUniqueFilename()
  └─> open(), write(), close() - save file
  └─> Response(201 Created)
```


---

## Debugging in VS Code

### The Timeout Problem

**Step-by-step debugging will cause issues** because:

1. **Client timeouts**: curl/browser will timeout waiting for response
2. **Server timeouts**: Your `cleanupTimedOutConnections()` may close the connection
3. **TCP keepalive**: OS may close idle connections

### Solutions for Debugging

**Option 1: Increase timeouts for debugging**

Add a debug configuration in your code:

```cpp
#ifdef DEBUG
    static const int CONNECTION_TIMEOUT = 3600; // 1 hour
#else
    static const int CONNECTION_TIMEOUT = 60;   // 1 minute
#endif
```


**Option 2: Use conditional breakpoints**

Set breakpoints that only trigger on specific conditions to minimize pauses.

**Option 3: Use logging instead of breakpoints**

Add detailed logging for the signal flow without stopping execution.

**Option 4: Debug with a patient client**

Use `nc` (netcat) which doesn't timeout:

```bash
nc localhost 8080
# Then manually type: GET / HTTP/1.1<Enter><Enter>
# Take your time stepping through
```


### Recommended VS Code Debug Configuration

Create or update `.vscode/launch.json`:


```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug webserv",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/webserv",
            "args": ["config/default.conf"],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                },
                {
                    "description": "Disable SIGPIPE",
                    "text": "handle SIGPIPE nostop noprint pass",
                    "ignoreFailures": true
                }
            ]
        }
    ]
}
```


### Key Breakpoint Locations

Set breakpoints at these functions to trace the flow:

| Function | File | Purpose |
|----------|------|---------|
| `Server::acceptNewConnection` | Server.cpp | New client connects |
| `Connection::readData` | Connection.cpp | Data received |
| `Request::parseRequestLine` | Request.cpp | Request parsing |
| `Router::route` | Router.cpp | Request routing |
| `Router::handleGet` | Router.cpp | GET handling |
| `CGI::execute` | CGI.cpp | CGI execution |
| `Connection::writeData` | Connection.cpp | Response sending |


---
