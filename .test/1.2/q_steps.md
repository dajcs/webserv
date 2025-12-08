Can you help me coding `### Step 2.1: Socket Creation & Binding` section of the `webserv` project at school 42, described below?


# Webserv

- Program name: webserv
- Files to submit: *.hpp, *.cpp, *.tpp, *.ipp, config files
- Makefile: NAME, all, clean, fclean, re
- Arguments: ./webserv <config_file>
- Allowed C functions: `execve`, `pipe`, `strerror`, `gai_strerror`, `errno`, `dup`,
  `dup2`, `fork`, `socketpair`, `htons`, `htonl`, `ntohs`, `ntohl`,`select`, `poll`,
  `epoll` (`epoll_create`, `epoll_ctl`, `epoll_wait`),
  `socket`, `accept`, `listen`, `send`, `recv`, `chdir`, `bind`, `connect`,
  `getaddrinfo`, `freeaddrinfo`, `setsockopt`, `getsockname`, `getprotobyname`,
  `fcntl`, `close`, `read`, `write`, `waitpid`, `kill`, `signal`, `access`, `stat`, `open`,
  `opendir`, `readdir`, `closedir`
- C++ Standard: C++98
- Description: An HTTP server in C++98

    **info**: Even though `poll()` is mentioned in the subject and evaluation sheet, any equivalent function can be used (e.g. epoll, kqueue, select).

## Mandatory part

- The program must use a configuration file, provided as an argument on the command line, or available in a default location (e.g. ./webserv.conf).
- the program can't `execve` another web server (e.g. nginx, apache, etc.)
- the server must remain non-blocking at all times and properly handle client disconnections when necessary.
- it must be non-blocking and use only **1** `poll()` (or equivalent) for all the I/O operations between the clients and the server (listen included).
- `poll()` (or equivalent) must monitor both reading and writing simultaneously.
- all read and write operations should go through the `poll()` (or equivalent) mechanism.
- checking the value of `errno` to adjust the server behaviour is strictly forbidden after a read or write operation.
- using `poll()` (or equivalent) for regular disk files is not required; `read()` and `write()` on them do not require readiness notifications.

  **Note**: I/O can wait for data (sockets, pipes/FIFOs, etc.) must be non-blocking and driven by a single `poll()` (or equivalent).
  Calling read/recv or write/send on these descriptors without prior readiness will result in a grade of 0. Regular disk files are exempt from this requirement.

- When using `poll()` (or equivalent), every associated macro or helper function (e.g., `FD_SET` for `select()`) can be used.
- A request to the server should never hang indefinitely.
- The server must be compatible with standard web browsers.
- NGINX may be used to compare headers and answer behaviours (pay attention to differences between HTTP/1.0 and HTTP/1.1).
- The HTTP response status codes must be accurate.
- The server must have **default error pages** if none are provided.
- `fork()` can be used only for CGI (like PHP, or Python, and so on) execution.
- it should be possible to **serve a fully static website**.
- Clients must be able to **upload files**
- At least the GET, POST, and DELETE methods must be implemented.
- Stress test the server to ensure it remains available at all times
- The server must be able to listen on multiple ports to deliver different content (see *Configuration file* section).

  **Note**: We deliberately chose to offer only a subset of the HTTP RFC. In this context, the virtual host feature is considered out of scope.
  But if you want to implement it anyway, you are free to do so.


## Configuration file

  **hint**: You can take inspiration from the `server` section of the NGINX configuration file.

The configuration file must allow at least the following settings:
- Define all the interface:port pairs on which the server will listen to (defining multiple websites served by the program)
- set up default error pages
- set the maximum allowed size for client request bodies
- specify rules or configurations on a URL/route (no regex required here), for a website, among the following:
  - List of accepted HTTP methods for the route
  - HTTP redirection
  - Directory where the requested file should be located (e.g., if URL /kapouet is rooted to /tmp/www, URL /capouet/pouic/tot/pouet will search for /tmp/www/pouic/toto/pouet)
  - Enable/disable directory listing
  - Default file to serve when the requested resource is a directory.
  - Uploading files from the clients to the server is authorized, and storage location is provided.
  - Execution of CGI, based on file extension (for example .php).
    Here are somespecific remarks regarding CGIs:
    - Do you wonder what a CGI is? Read [this](https://en.wikipedia.org/wiki/Common_Gateway_Interface)
    - Have a careful look at the environment variables involved in the web server-CGI communication. The full request and arguments provided by the client must be available to the CGI.
    - Remember that for chunked requests, the server needs to un-chunk them, the CGI will expect EOF as the endo of the body.
    - The same applies to the output of the CGI. If no `content_length` is returned from the CGI, EOF will mark the end of the returned data.
    - The CGI should be run in the correct directory for relative path file access.
    - The server should support at least one CGI (php-CGI, Python, and so on).

You must provide configuration files and default files to test and demonstrate that every feature works during the evaluation.

You can have other rules or configuration information in your file (e.g. a server name for a website if you plan to implement virtual hosts).

  **info**: If you have a question about a specific behaviour, you can compare your program's vehaviour wih NGINX's.
  We have provide a small tester. Using it is not mandatory if everything works fine with browser andd tests, but it can help you find and fix bugs.

  **attention**: Resilience is key. Your server must remain operational at all times.

  **attention**: Do not test with only one program. Write your tests in a more suitable language, such as Python, among others, even in C or C++ if you prefer.



## Bonus part

Additional features that can be implemented:

- Support cookies and session management (provide simple examples)
- Handle multiple CGI types




# Webserv Implementation Workflow

## Phase 1: Project Setup & Basic Infrastructure

### Step 1.1: Project Structure Setup
**Goal**: Create the basic project structure

**Tasks**:
- Create directory structure as shown below:

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

