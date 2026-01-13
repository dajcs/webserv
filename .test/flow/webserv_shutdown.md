# Signal Flow After Pressing Ctrl+C

The complete signal flow after Ctrl-C is pressed:


```cpp
┌─────────────────────────────────────────────────────────────────────────────┐
│                           CTRL+C PRESSED                                    │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  KERNEL generates SIGINT (signal number 2)                                  │
│  Delivers to process                                                        │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  signalHandler(int signum)                     // signum = 2 (SIGINT)       │
│  ├── Prints: "[INFO] Received SIGINT (Ctrl+C), shutting down..."            │
│  └── Calls: g_server->stop()                   // g_server = Server*        │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  Server::stop()                                                             │
│  ├── Sets: _running = false                                                 │
│  ├── Collects client FDs from _connections map                              │
│  ├── For each clientFd:                                                     │
│  │   └── closeClientConnection(int clientFd)                                │
│  ├── closeEpoll()                                                           │
│  └── closeAllSockets()                                                      │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                    ┌───────────────┼───────────────┐
                    ▼               ▼               ▼
┌───────────────────────┐ ┌─────────────────┐ ┌─────────────────────────────┐
│ closeClientConnection │ │ closeEpoll()    │ │ closeAllSockets()           │
│ (int clientFd)        │ │                 │ │                             │
│ ├── removeFromEpoll   │ │ close(_epollFd) │ │ For each ListenSocket:      │
│ │   (clientFd)        │ │ _epollFd = -1   │ │ ├── close(fd)               │
│ ├── close(clientFd)   │ │                 │ │ └── fd = -1                 │
│ └── _connections      │ │                 │ │ _listenSockets.clear()      │
│     .erase(clientFd)  │ │                 │ │ _listenFds.clear()          │
└───────────────────────┘ └─────────────────┘ └─────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  removeFromEpoll(int fd)                                                    │
│  └── epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, &ev)                            │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  BACK TO Server::run() - epoll_wait() loop                                  │
│  ├── _running == false, exits while loop                                    │
│  └── Prints: "=== Event Loop Ended ==="                                     │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  BACK TO main()                                                             │
│  ├── g_server = NULL                                                        │
│  ├── Prints: "[INFO] Server stopped gracefully"                             │
│  ├── Prints: "[INFO] Goodbye!"                                              │
│  └── return EXIT_SUCCESS (0)                                                │
└─────────────────────────────────────────────────────────────────────────────┘
```

## Key Function Signatures

| Function | Parameters | Purpose |
|----------|------------|---------|
| `signalHandler` | `int signum` | Receives signal number (2 for SIGINT) |
| `Server::stop` | `void` | Initiates graceful shutdown |
| `closeClientConnection` | `int clientFd` | Closes one client socket |
| `removeFromEpoll` | `int fd` | Removes FD from epoll monitoring |
| `closeEpoll` | `void` | Closes the epoll instance |
| `closeAllSockets` | `void` | Closes all listening sockets |
