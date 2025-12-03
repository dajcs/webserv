# Webserv

- Program name: webserv
- Files to submit: *.hpp, *.cpp, *.tpp, *.ipp, config files
- Makefile: NAME, all, clean, fclean, re
- Arguments: ./webserv <config_file>
- Allowed C functions: execve, pipe, strerror, gai_strerror, errno, dup,
  dup2, fork, socketpair, htons, htonl, ntohs, ntohl,select, poll, 
  epoll (epoll_create, epoll_ctl, epoll_wait), kqueue (kqueue, kevent), 
  socket, accept, listen, send, recv, chdir, bind, connect, 
  getaddrinfo, freeaddrinfo, setsockopt, getsockname, getprotobyname, 
  fcntl, close, read, write, waitpid, kill, signal, access, stat, open, 
  opendir, readdir, closedir
- C++ Standard: C++98
- Description: An HTTP server in C++98

    **info**: Even though poll() is mentionaed in the subject and evaluation sheet, any equivalent function can be used (e.g. epoll, kqueue, select).

## Mandatory part

- The program must use a configuration file, provided as an argument on the command line, or available in a default location (e.g. ./webserv.conf).
- the program can't `execve` another web server (e.g. nginx, apache, etc.)
- the server must remain non-blocking at all times and properly handle client disconnections when necessary.
- it must be non-blocking and use only 1 poll() (or equivalent) for all the I/O operations between the clients and the server (listen included).
- `poll()` (or equivalent) must monitor both reading and writing simultaneously.
- all read and write operations should go through the `poll()` (or equivalent) mechanism.
- checking the value of `errno` to adjust the server behaviour is strictly forbidden after a read or write operation.
- using `poll()` (or equivalent) for regular disk files is not required; `read()` and `write()` on them do not require readiness notifications.

    **Note**: I/O can wait for data (sockets, pipes/FIFOs, etc.) must be non-blocking and driven by a single `poll()` (or equivalent). 
    Calling read/recv or write/send on these descriptors without prior readiness will result in a grade of 0. Regular disk files are exempt from this requirement.

- When using `poll()` (or equivalent), every associated macro or helper function (e.g., `FD_SET` for `select()`) can be used.
- A request to the server should never hang indefinitely.
- The server must be compatible with standard web browsers (e.g., Chrome, Firefox) and command-line tools (e.g., curl, wget).
- NGINX may be used to compare headers and answer behaviours (pay attention to differences between HTTP/1.0 and HTTP/1.1).
- The HTTP response status codes must be accurate.
- The server must have **default error pages** if none are provided.
- `fork()` can be used only for CGI (like PHP, or Python, and so on) execution.
- it should be possible to **serve a fully static website**.
- Clients must be able to **upload files**
- At least the GET, POST, and DELETE methods must be implemented.
- Stress test the server to ensure it remains available at all times
- The server must be able to listen on multiple ports to deliver different content (see *Configuratioin file* section).

    **Note**: We deliberately chose to offer only a subset of the HTTP RFC. In this context, the virtual host feature is considered out of scope.
    But if you want to implement it anyway, you are free to do so.

## Configuration file

    **hint**: You can take inspiration from the `server` section of the NGINX configuration file.




