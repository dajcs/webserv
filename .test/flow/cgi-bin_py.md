# Signal Flow: CGI Request Processing

The complete signal flow when a client requests:

`curl -v http://localhost:8080/cgi-bin/py/hello.py`


```cpp
┌─────────────────────────────────────────────────────────────────────────────┐
│                    CONNECTION & REQUEST READING                             │
│                     (Same as static file request)                           │
└─────────────────────────────────────────────────────────────────────────────┘

epoll_wait(4, events, 64, 1000) → 1
    │
    ▼
accept(3, &clientAddr, &clientLen) → fd=5
fcntl(5, F_GETFL) → flags
fcntl(5, F_SETFL, flags | O_NONBLOCK) → 0
epoll_ctl(4, EPOLL_CTL_ADD, 5, {EPOLLIN, fd=5}) → 0
    │
    ▼
epoll_wait(4, events, 64, 1000) → 1
events[0] = {EPOLLIN, fd=5}
    │
    ▼
recv(5, buffer, 8192, 0) → ~100 bytes
    │
    Request received:
    │   "GET /cgi-bin/py/hello.py HTTP/1.1\r\n"
    │   "Host: localhost:8080\r\n"
    │   "User-Agent: curl/7.68.0\r\n"
    │   "Accept: */*\r\n"
    │   "\r\n"
    │
    ▼
Request::parse(_readBuffer)
    │
    ├──► _method = "GET"
    ├──► _uri = "/cgi-bin/py/hello.py"
    ├──► _path = "/cgi-bin/py/hello.py"
    ├──► _queryString = ""
    ├──► _httpVersion = "HTTP/1.1"
    ├──► _headers["host"] = "localhost:8080"
    ├──► _headers["user-agent"] = "curl/7.68.0"
    ├──► _headers["accept"] = "*/*"
    └──► _state = PARSE_COMPLETE


┌─────────────────────────────────────────────────────────────────────────────┐
│                         REQUEST ROUTING                                     │
└─────────────────────────────────────────────────────────────────────────────┘

processRequest(conn)
    │
    ▼
Router::route(request, serverPort=8080)
    │
    ├──► findServer(port=8080, hostname="localhost:8080")
    │        └──► Returns: &servers[0] (localhost server)
    │
    ├──► requestPath = "/cgi-bin/py/hello.py"
    ├──► method = "GET"
    │
    ├──► findLocation(server, path="/cgi-bin/py/hello.py", method="GET")
    │        │
    │        ├──► Check location "/" :
    │        │        matchLength = 1
    │        │
    │        ├──► Check location "/cgi-bin/py":
    │        │        "/cgi-bin/py/hello.py".compare(0, 12, "/cgi-bin/py") == 0 ✓
    │        │        "/cgi-bin/py/hello.py"[12] == '/' ✓ (path boundary)
    │        │        matchLength = 12  ← BEST MATCH!
    │        │
    │        └──► Returns: &LocationConfig{
    │                        path = "/cgi-bin/py",
    │                        root = "www",
    │                        allowed_methods = {GET, POST},
    │                        autoindex = true,
    │                        cgi_extension = ".py",
    │                        cgi_path = "/usr/bin/python3"
    │                      }
    │
    ├──► isMethodAllowed("GET", location) → true ✓
    │
    ├──► resolvePath("/cgi-bin/py/hello.py", location)
    │        │
    │        ├──► root = "www"
    │        ├──► locationPath = "/cgi-bin/py"
    │        ├──► remainder = "/hello.py"
    │        │
    │        └──► Returns: "www/cgi-bin/py/hello.py"
    │
    ├──► isCgiRequest("www/cgi-bin/py/hello.py", location)
    │        │
    │        └──► CGI::isCgiRequest(path, location)
    │                 │
    │                 ├──► location.cgi_path = "/usr/bin/python3" (not empty) ✓
    │                 ├──► location.cgi_extension = ".py"
    │                 ├──► path.compare(pathLen - 3, 3, ".py") == 0 ✓
    │                 │
    │                 └──► Returns: true  ← IT IS A CGI REQUEST!
    │
    └──► handleCgi(request, "www/cgi-bin/py/hello.py", location)


┌─────────────────────────────────────────────────────────────────────────────┐
│                      CGI SETUP (Step 8.1)                                   │
└─────────────────────────────────────────────────────────────────────────────┘

handleCgi(request, scriptPath="www/cgi-bin/py/hello.py", location)
    │
    ├──► CGI cgi(request, location)
    │        │
    │        ├──► _request = &request
    │        ├──► _location = &location
    │        └──► _ready = false
    │
    └──► cgi.setup("www/cgi-bin/py/hello.py")
             │
             │  ┌─────────────────────────────────────────────────────────┐
             │  │            VALIDATE SCRIPT (Step 1)                     │
             │  └─────────────────────────────────────────────────────────┘
             │
             ├──► validateScript("www/cgi-bin/py/hello.py")
             │        │
             │        ├──► stat("www/cgi-bin/py/hello.py", &st)
             │        │        └──► Returns: 0 (file exists)
             │        │             st.st_mode = S_IFREG | 0755
             │        │
             │        ├──► S_ISREG(st.st_mode) → true ✓
             │        │
             │        ├──► st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)
             │        │        └──► true ✓ (executable)
             │        │
             │        └──► Returns: true
             │
             ├──► _scriptPath = "www/cgi-bin/py/hello.py"
             │
             │  ┌─────────────────────────────────────────────────────────┐
             │  │            VALIDATE INTERPRETER (Step 2)                │
             │  └─────────────────────────────────────────────────────────┘
             │
             ├──► _interpreterPath = "/usr/bin/python3"
             │
             ├──► validateInterpreter("/usr/bin/python3")
             │        │
             │        ├──► access("/usr/bin/python3", X_OK)
             │        │        └──► Returns: 0 (executable) ✓
             │        │
             │        └──► Returns: true
             │
             │  ┌─────────────────────────────────────────────────────────┐
             │  │            DETERMINE WORKING DIRECTORY (Step 3)         │
             │  └─────────────────────────────────────────────────────────┘
             │
             ├──► _scriptPath.rfind('/') → 18
             ├──► _workingDirectory = "www/cgi-bin/py"
             │
             │  ┌─────────────────────────────────────────────────────────┐
             │  │            EXTRACT PATH_INFO (Step 4)                   │
             │  └─────────────────────────────────────────────────────────┘
             │
             ├──► extractPathInfo()
             │        │
             │        ├──► requestPath = "/cgi-bin/py/hello.py"
             │        ├──► ext = ".py"
             │        ├──► extPos = requestPath.find(".py") → 19
             │        ├──► pathInfoStart = 19 + 3 = 22
             │        ├──► 22 >= 22 (requestPath.length())
             │        │
             │        └──► Returns: "" (no PATH_INFO)
             │
             ├──► _pathInfo = ""
             │
             │  ┌─────────────────────────────────────────────────────────┐
             │  │            BUILD ENVIRONMENT (Step 5)                   │
             │  └─────────────────────────────────────────────────────────┘
             │
             └──► buildEnvironment()
                      │
                      ├──► _envVars["REQUEST_METHOD"] = "GET"
                      ├──► _envVars["QUERY_STRING"] = ""
                      ├──► _envVars["SCRIPT_NAME"] = "/cgi-bin/py/hello.py"
                      ├──► _envVars["SCRIPT_FILENAME"] = "www/cgi-bin/py/hello.py"
                      ├──► _envVars["PATH_INFO"] = ""
                      ├──► _envVars["SERVER_PROTOCOL"] = "HTTP/1.1"
                      ├──► _envVars["GATEWAY_INTERFACE"] = "CGI/1.1"
                      ├──► _envVars["REQUEST_URI"] = "/cgi-bin/py/hello.py"
                      ├──► _envVars["SERVER_SOFTWARE"] = "webserv/1.0"
                      ├──► _envVars["SERVER_NAME"] = "localhost"
                      ├──► _envVars["SERVER_PORT"] = "8080"
                      ├──► _envVars["REMOTE_ADDR"] = "127.0.0.1"
                      ├──► _envVars["REMOTE_HOST"] = "127.0.0.1"
                      ├──► _envVars["DOCUMENT_ROOT"] = "www"
                      ├──► _envVars["REDIRECT_STATUS"] = "200"
                      │
                      └──► addHttpHeaders()
                               │
                               ├──► _envVars["HTTP_HOST"] = "localhost:8080"
                               ├──► _envVars["HTTP_USER_AGENT"] = "curl/7.68.0"
                               └──► _envVars["HTTP_ACCEPT"] = "*/*"
             │
             ├──► _ready = true
             └──► Returns: true


┌─────────────────────────────────────────────────────────────────────────────┐
│                      CGI EXECUTION (Step 8.2)                               │
└─────────────────────────────────────────────────────────────────────────────┘

cgi.execute(timeout=30)
    │
    │  ┌─────────────────────────────────────────────────────────────────────┐
    │  │                    CREATE PIPES                                      │
    │  └─────────────────────────────────────────────────────────────────────┘
    │
    ├──► pipe(stdin_pipe)
    │        │
    │        └──► Returns: 0
    │             stdin_pipe[0] = 6  (read end)
    │             stdin_pipe[1] = 7  (write end)
    │
    ├──► pipe(stdout_pipe)
    │        │
    │        └──► Returns: 0
    │             stdout_pipe[0] = 8  (read end)
    │             stdout_pipe[1] = 9  (write end)
    │
    │  ┌─────────────────────────────────────────────────────────────────────┐
    │  │                       FORK                                          │
    │  └─────────────────────────────────────────────────────────────────────┘
    │
    ├──► fork()
    │        │
    │        └──► Returns: pid (in parent), 0 (in child)


    ════════════════════════════════════════════════════════════════════════
                              CHILD PROCESS (pid == 0)
    ════════════════════════════════════════════════════════════════════════

    ├──► dup2(stdin_pipe[0], STDIN_FILENO)
    │        │
    │        └──► Returns: 0 (stdin now reads from pipe)
    │
    ├──► dup2(stdout_pipe[1], STDOUT_FILENO)
    │        │
    │        └──► Returns: 1 (stdout now writes to pipe)
    │
    ├──► close(stdin_pipe[0])   → 0
    ├──► close(stdin_pipe[1])   → 0
    ├──► close(stdout_pipe[0])  → 0
    ├──► close(stdout_pipe[1])  → 0
    │
    ├──► chdir("www/cgi-bin/py")
    │        │
    │        └──► Returns: 0
    │
    ├──► getArgv()
    │        │
    │        └──► Returns: char** argv = {
    │                 "/usr/bin/python3",
    │                 "hello.py",
    │                 NULL
    │             }
    │
    ├──► getEnvArray()
    │        │
    │        └──► Returns: char** envp = {
    │                 "REQUEST_METHOD=GET",
    │                 "QUERY_STRING=",
    │                 "SCRIPT_NAME=/cgi-bin/py/hello.py",
    │                 "SCRIPT_FILENAME=www/cgi-bin/py/hello.py",
    │                 "PATH_INFO=",
    │                 "SERVER_PROTOCOL=HTTP/1.1",
    │                 "GATEWAY_INTERFACE=CGI/1.1",
    │                 "REQUEST_URI=/cgi-bin/py/hello.py",
    │                 "SERVER_SOFTWARE=webserv/1.0",
    │                 "SERVER_NAME=localhost",
    │                 "SERVER_PORT=8080",
    │                 "REMOTE_ADDR=127.0.0.1",
    │                 "REMOTE_HOST=127.0.0.1",
    │                 "DOCUMENT_ROOT=www",
    │                 "REDIRECT_STATUS=200",
    │                 "HTTP_HOST=localhost:8080",
    │                 "HTTP_USER_AGENT=curl/7.68.0",
    │                 "HTTP_ACCEPT=*/*",
    │                 NULL
    │             }
    │
    └──► execve("/usr/bin/python3", argv, envp)
             │
             │   ┌─────────────────────────────────────────────────────────┐
             │   │    PYTHON3 EXECUTES hello.py                            │
             │   │                                                         │
             │   │    Script reads environment variables, generates output │
             │   │    and writes to stdout (which is the pipe)             │
             │   └─────────────────────────────────────────────────────────┘
             │
             │   hello.py:
             │   ─────────────────────────────────────────────────────
             │   #!/usr/bin/env python3
             │   print("Content-Type: text/html")
             │   print()
             │   print("<html><body>")
             │   print("<h1>Hello from Python CGI!</h1>")
             │   print("</body></html>")
             │   ─────────────────────────────────────────────────────
             │
             │   Script writes to stdout (pipe):
             │   "Content-Type: text/html\n\n<html><body>\n<h1>Hello from Python CGI!</h1>\n</body></html>\n"
             │
             └──► Process exits with status 0


    ════════════════════════════════════════════════════════════════════════
                              PARENT PROCESS (pid > 0)
    ════════════════════════════════════════════════════════════════════════

    │
    ├──► close(stdin_pipe[0])   → 0  (child's read end)
    │        stdin_pipe[0] = -1
    │
    ├──► close(stdout_pipe[1])  → 0  (child's write end)
    │        stdout_pipe[1] = -1
    │
    ├──► setNonBlocking(stdin_pipe[1])   // fd 7
    │        ├──► fcntl(7, F_GETFL) → flags
    │        └──► fcntl(7, F_SETFL, flags | O_NONBLOCK) → 0
    │
    ├──► setNonBlocking(stdout_pipe[0])  // fd 8
    │        ├──► fcntl(8, F_GETFL) → flags
    │        └──► fcntl(8, F_SETFL, flags | O_NONBLOCK) → 0
    │
    ├──► requestBody = getRequestBody() → "" (empty for GET)
    ├──► doneWriting = true (no body to write)
    │
    │  ┌─────────────────────────────────────────────────────────────────────┐
    │  │                 POLL LOOP (Read CGI Output)                         │
    │  └─────────────────────────────────────────────────────────────────────┘
    │
    ├──► while (true)
    │        │
    │        ├──► time(NULL) - startTime < 30 ✓ (not timed out)
    │        │
    │        ├──► Setup poll:
    │        │        pfds[0] = {fd=8, events=POLLIN, revents=0}  // stdout
    │        │        nfds = 1  (no stdin since doneWriting=true)
    │        │
    │        ├──► poll(pfds, 1, 1000)
    │        │        │
    │        │        └──► Returns: 1
    │        │             pfds[0].revents = POLLIN
    │        │
    │        ├──► pfds[0].revents & POLLIN → true
    │        │        │
    │        │        └──► read(8, buffer, 65536)
    │        │                 │
    │        │                 └──► Returns: 85 bytes
    │        │                      buffer = "Content-Type: text/html\n\n
    │        │                               <html><body>\n
    │        │                               <h1>Hello from Python CGI!</h1>\n
    │        │                               </body></html>\n"
    │        │
    │        ├──► cgiOutput.append(buffer, 85)
    │        │
    │        ├──► (next poll iteration)
    │        │
    │        ├──► poll(pfds, 1, 1000)
    │        │        │
    │        │        └──► Returns: 1
    │        │             pfds[0].revents = POLLHUP  (child exited)
    │        │
    │        ├──► pfds[0].revents & POLLHUP → true
    │        │        │
    │        │        ├──► Read remaining data: read(8, buffer, 65536) → 0
    │        │        │
    │        │        ├──► close(stdout_pipe[0])  // fd 8
    │        │        │        stdout_pipe[0] = -1
    │        │        │
    │        │        └──► close(stdin_pipe[1]) if still open
    │        │
    │        └──► waitpid(pid, &status, 0)
    │                 │
    │                 └──► Returns: pid
    │                      status: WIFEXITED(status) = true
    │                              WEXITSTATUS(status) = 0
    │
    │  ┌─────────────────────────────────────────────────────────────────────┐
    │  │                 PARSE CGI OUTPUT                                    │
    │  └─────────────────────────────────────────────────────────────────────┘
    │
    └──► parseCgiOutput(cgiOutput, result)
             │
             ├──► cgiOutput = "Content-Type: text/html\n\n<html><body>..."
             │
             ├──► Find separator: output.find("\r\n\r\n") → npos
             │                    output.find("\n\n") → 24
             │
             ├──► separator = "\n\n"
             ├──► separatorPos = 24
             │
             ├──► headerSection = "Content-Type: text/html"
             ├──► result.body = "<html><body>\n<h1>Hello from Python CGI!</h1>\n</body></html>\n"
             │
             ├──► Parse headers:
             │        │
             │        └──► Line: "Content-Type: text/html"
             │                 colonPos = 12
             │                 name = "Content-Type"
             │                 value = "text/html"
             │                 result.headers["Content-Type"] = "text/html"
             │
             ├──► Check for Status header: not found
             │        result.statusCode = 200 (default)
             │
             ├──► result.success = true
             │
             └──► Returns: true


┌─────────────────────────────────────────────────────────────────────────────┐
│                      BUILD HTTP RESPONSE                                    │
└─────────────────────────────────────────────────────────────────────────────┘

handleCgi() continues:
    │
    ├──► cgiResult.success == true ✓
    │
    └──► Build Response from CGI result
             │
             ├──► Response response
             │
             ├──► response.setStatus(200)
             │        _statusCode = 200
             │        _reasonPhrase = "OK"
             │
             ├──► Copy CGI headers:
             │        response.setHeader("Content-Type", "text/html")
             │
             ├──► response.setBody("<html><body>\n<h1>Hello from Python CGI!</h1>\n</body></html>\n")
             │
             ├──► response.addStandardHeaders()
             │        ├──► setHeader("Date", "Sun, 12 Jan 2026 10:00:00 GMT")
             │        ├──► setHeader("Server", "webserv/1.0")
             │        └──► setHeader("Connection", "keep-alive")
             │
             └──► Returns: response


┌─────────────────────────────────────────────────────────────────────────────┐
│                      SEND RESPONSE TO CLIENT                                │
└─────────────────────────────────────────────────────────────────────────────┘

conn.setResponse(response)
    │
    ├──► response.build()
    │        │
    │        └──► _builtResponse =
    │             "HTTP/1.1 200 OK\r\n"
    │             "Content-Type: text/html\r\n"
    │             "Date: Sun, 12 Jan 2026 10:00:00 GMT\r\n"
    │             "Server: webserv/1.0\r\n"
    │             "Connection: keep-alive\r\n"
    │             "Content-Length: 62\r\n"
    │             "\r\n"
    │             "<html><body>\n"
    │             "<h1>Hello from Python CGI!</h1>\n"
    │             "</body></html>\n"
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
    ├──► send(5, response_data, ~250, MSG_NOSIGNAL)
    │        │
    │        └──► Returns: ~250 (all bytes sent)
    │
    └──► handleWriteComplete()
             │
             └──► conn.reset() (keep-alive)
    │
    ▼
modifyEpoll(5, EPOLLIN)
    │
    └──► epoll_ctl(4, EPOLL_CTL_MOD, 5, {events=EPOLLIN, data.fd=5})


┌─────────────────────────────────────────────────────────────────────────────┐
│                         CURL OUTPUT                                         │
└─────────────────────────────────────────────────────────────────────────────┘

$ curl -v http://localhost:8080/cgi-bin/py/hello.py
*   Trying 127.0.0.1:8080...
* Connected to localhost (127.0.0.1) port 8080 (#0)
> GET /cgi-bin/py/hello.py HTTP/1.1
> Host: localhost:8080
> User-Agent: curl/7.68.0
> Accept: * / *
>
* Mark bundle as not supporting multiuse
< HTTP/1.1 200 OK
< Content-Type: text/html
< Date: Sun, 12 Jan 2026 10:00:00 GMT
< Server: webserv/1.0
< Connection: keep-alive
< Content-Length: 62
<
<html><body>
<h1>Hello from Python CGI!</h1>
</body></html>
* Connection #0 to host localhost left intact
```

## File Descriptor Summary During CGI Execution

```cpp
┌─────────────────────────────────────────────────────────────────────────────┐
│                    FILE DESCRIPTORS TIMELINE                                │
└─────────────────────────────────────────────────────────────────────────────┘

BEFORE fork():
┌────────┬─────────────────────────────────────────┐
│   FD   │           Purpose                       │
├────────┼─────────────────────────────────────────┤
│   0    │ stdin                                   │
│   1    │ stdout                                  │
│   2    │ stderr                                  │
│   3    │ Listening socket (port 8080)            │
│   4    │ Epoll instance                          │
│   5    │ Client socket (curl connection)         │
│   6    │ stdin_pipe[0]  (pipe read end)          │
│   7    │ stdin_pipe[1]  (pipe write end)         │
│   8    │ stdout_pipe[0] (pipe read end)          │
│   9    │ stdout_pipe[1] (pipe write end)         │
└────────┴─────────────────────────────────────────┘

CHILD PROCESS (after dup2, before execve):
┌────────┬─────────────────────────────────────────┐
│   FD   │           Purpose                       │
├────────┼─────────────────────────────────────────┤
│   0    │ stdin ← reads from stdin_pipe           │
│   1    │ stdout → writes to stdout_pipe          │
│   2    │ stderr                                  │
└────────┴─────────────────────────────────────────┘
(All other FDs closed)

PARENT PROCESS (during CGI execution):
┌────────┬─────────────────────────────────────────┐
│   FD   │           Purpose                       │
├────────┼─────────────────────────────────────────┤
│   0    │ stdin                                   │
│   1    │ stdout                                  │
│   2    │ stderr                                  │
│   3    │ Listening socket (port 8080)            │
│   4    │ Epoll instance                          │
│   5    │ Client socket (curl connection)         │
│   7    │ stdin_pipe[1] → write POST body to CGI  │
│   8    │ stdout_pipe[0] ← read output from CGI   │
└────────┴─────────────────────────────────────────┘
(FDs 6 and 9 closed - the child end of pipes)

AFTER CGI EXECUTION (cleanup):
┌────────┬─────────────────────────────────────────┐
│   FD   │           Purpose                       │
├────────┼─────────────────────────────────────────┤
│   0    │ stdin                                   │
│   1    │ stdout                                  │
│   2    │ stderr                                  │
│   3    │ Listening socket (port 8080)            │
│   4    │ Epoll instance                          │
│   5    │ Client socket (curl connection)         │
└────────┴─────────────────────────────────────────┘
(All pipe FDs closed, child process reaped)
```


## Summary: System Calls for CGI Request

| Phase | System Call | Parameters | Returns |
|-------|-------------|------------|---------|
| **Accept** | `accept` | `(3, &addr, &len)` | 5 |
| | `fcntl` | `(5, F_SETFL, O_NONBLOCK)` | 0 |
| | `epoll_ctl` | `(4, ADD, 5, EPOLLIN)` | 0 |
| **Read** | `recv` | `(5, buf, 8192, 0)` | ~100 |
| **CGI Setup** | `stat` | `("www/cgi-bin/py/hello.py", &st)` | 0 |
| | `access` | `("/usr/bin/python3", X_OK)` | 0 |
| **CGI Exec** | `pipe` | `(stdin_pipe)` | 0 (fds 6,7) |
| | `pipe` | `(stdout_pipe)` | 0 (fds 8,9) |
| | `fork` | `()` | pid / 0 |
| *Child* | `dup2` | `(6, 0)` | 0 |
| | `dup2` | `(9, 1)` | 1 |
| | `close` | `(6,7,8,9)` | 0 |
| | `chdir` | `("www/cgi-bin/py")` | 0 |
| | `execve` | `("/usr/bin/python3", argv, envp)` | (no return) |
| *Parent* | `close` | `(6)` | 0 |
| | `close` | `(9)` | 0 |
| | `fcntl` | `(7, F_SETFL, O_NONBLOCK)` | 0 |
| | `fcntl` | `(8, F_SETFL, O_NONBLOCK)` | 0 |
| | `poll` | `(pfds, 1, 1000)` | 1 |
| | `read` | `(8, buffer, 65536)` | 85 |
| | `poll` | `(pfds, 1, 1000)` | 1 (POLLHUP) |
| | `close` | `(8)` | 0 |
| | `close` | `(7)` | 0 |
| | `waitpid` | `(pid, &status, 0)` | pid |
| **Write** | `epoll_ctl` | `(4, MOD, 5, EPOLLOUT)` | 0 |
| | `send` | `(5, response, ~250, MSG_NOSIGNAL)` | ~250 |
| | `epoll_ctl` | `(4, MOD, 5, EPOLLIN)` | 0 |
