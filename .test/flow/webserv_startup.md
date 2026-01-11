# Signal Flow: webserv Startup and Wait for Requests

Here's the complete signal flow from program start to waiting for connections:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              STARTUP PHASE                                  │
└─────────────────────────────────────────────────────────────────────────────┘

main(argc=2, argv=["./webserv", "config/default.conf"])
    │
    ├──► setupSignalHandlers()
    │        │
    │        ├──► sigaction(SIGINT, &sa, NULL)     // register Ctrl+C handler
    │        ├──► sigaction(SIGTERM, &sa, NULL)    // register Kill signal handler
    │        └──► sigaction(SIGPIPE, &sa{SIG_IGN}) // Ignore broken pipe to prevent
                                                   //    crashes during socket writes
    │
    ├──► Config::parseFile("config/default.conf")
    │        │
    │        ├──► ifstream::open("config/default.conf")
    │        ├──► parseServerBlock(file, line)     // For each "server {"
    │        │        └──► parseLocationBlock(file, line, server)  // For each "location {"
    │        └──► validateConfig()
    │
    ├──► Server::Server(config)
    │        │
    │        └──► _config = &config
    │            _running = false
    │            _epollFd = -1
    │
    └──► Server::init()
             │
             ├──► _config->getServers()  // Returns vector<ServerConfig>
             │
             │    ┌─── For each ServerConfig (port 8080) ───┐
             │    │                                          │
             │    └──► createListenSocket("0.0.0.0", 8080)
             │              │
             │              ├──► socket(AF_INET, SOCK_STREAM, 0)
             │              │        Returns: fd=3 (first available after 0,1,2)
             │              │
             │              ├──► setsockopt(3, SOL_SOCKET, SO_REUSEADDR, &1, 4)
             │              │        Returns: 0 (success)
             │              │
             │              ├──► setsockopt(3, SOL_SOCKET, SO_REUSEPORT, &1, 4)
             │              │        Returns: 0 (success, optional)
             │              │
             │              ├──► bind(3, {AF_INET, htons(8080), INADDR_ANY}, 16)
             │              │        Returns: 0 (success)
             │              │
             │              ├──► setNonBlocking(3)
             │              │        │
             │              │        ├──► fcntl(3, F_GETFL, 0)
             │              │        │        Returns: current flags (e.g., 2)
             │              │        │
             │              │        └──► fcntl(3, F_SETFL, flags | O_NONBLOCK)
             │              │                 Returns: 0 (success)
             │              │
             │              └──► listen(3, 128)
             │                       Returns: 0 (success)
             │
             ├──► _listenSockets.push_back({fd=3, host="0.0.0.0", port=8080})
             ├──► _listenFds.insert(3)
             │
             └──► initEpoll()
                      │
                      ├──► epoll_create1(0)
                      │        Returns: fd=4 (epoll instance)
                      │
                      └──► addToEpoll(3, EPOLLIN)
                               │
                               └──► epoll_ctl(4, EPOLL_CTL_ADD, 3,
                                              {events=EPOLLIN, data.fd=3})
                                        Returns: 0 (success)


┌─────────────────────────────────────────────────────────────────────────────┐
│                           MAIN EVENT LOOP                                   │
└─────────────────────────────────────────────────────────────────────────────┘

Server::run()
    │
    ├──► _running = true
    │
    └──► while (_running)
             │
             └──► epoll_wait(4, events[64], 64, 1000)
                      │
                      │   Parameters:
                      │     epfd = 4          (epoll instance)
                      │     events = [64]     (array to receive events)
                      │     maxevents = 64    (max events to return)
                      │     timeout = 1000    (1 second timeout in ms)
                      │
                      └──► BLOCKS HERE waiting for:
                           - Connection on fd=3 (listening socket)
                           - Timeout (1 second)
                           - Signal (SIGINT/SIGTERM)

                      Returns:
                        > 0: Number of ready file descriptors
                          0: Timeout (no events)
                         -1: Error or interrupted (errno=EINTR for signals)


┌─────────────────────────────────────────────────────────────────────────────┐
│                    WAITING STATE (IDLE SERVER)                              │
└─────────────────────────────────────────────────────────────────────────────┘

File Descriptors in use:
┌────────┬─────────────────────────────────┐
│   FD   │           Purpose               │
├────────┼─────────────────────────────────┤
│   0    │ stdin                           │
│   1    │ stdout                          │
│   2    │ stderr                          │
│   3    │ Listening socket (port 8080)    │
│   4    │ Epoll instance                  │
└────────┴─────────────────────────────────┘

Epoll Interest List:
┌────────┬─────────────────────────────────┐
│   FD   │       Monitored Events          │
├────────┼─────────────────────────────────┤
│   3    │ EPOLLIN (new connections)       │
└────────┴─────────────────────────────────┘

Data Structures:
┌─────────────────────────────────────────────────────────────────────────┐
│ Server                                                                  │
├─────────────────────────────────────────────────────────────────────────┤
│ _config          → Config* (parsed from default.conf)                   │
│ _running         → true                                                 │
│ _epollFd         → 4                                                    │
│ _listenSockets   → [{fd=3, host="0.0.0.0", port=8080, serverConfig*}]   │
│ _listenFds       → {3}                                                  │
│ _connections     → {} (empty map - no clients yet)                      │
└─────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────┐
│ Config                                                                  │
├─────────────────────────────────────────────────────────────────────────┤
│ _servers[0]:                                                            │
│   host = "0.0.0.0"                                                      │
│   port = 8080                                                           │
│   server_names = ["localhost"]                                          │
│   client_max_body_size = 1048576 (1MB)                                  │
│   error_pages = {404: "/errors/404.html", 500: "/errors/500.html"}      │
│   locations = [                                                         │
│     {path="/", root="www", index="index.html", methods={GET,POST}},     │
│     {path="/files", root="www", autoindex=true, methods={GET}},         │
│     {path="/uploads", root="www", upload_path="www/uploads", ...},      │
│     {path="/cgi-bin/py", cgi_extension=".py", cgi_path="/usr/bin/...}, │
│     ...                                                                 │
│   ]                                                                     │
│                                                                         │
│ _servers[1]:                                                            │
│   host = "0.0.0.0"                                                      │
│   port = 8080                                                           │
│   server_names = ["marigold.hotel"]                                     │
│   ...                                                                   │
└─────────────────────────────────────────────────────────────────────────┘


┌─────────────────────────────────────────────────────────────────────────┐
│                    PERIODIC TIMEOUT CYCLE                               │
└─────────────────────────────────────────────────────────────────────────┘

Every 1 second (EPOLL_TIMEOUT_MS = 1000):

    epoll_wait(4, events, 64, 1000)
         │
         └──► Returns 0 (timeout, no events)

    numEvents = 0
         │
         └──► Skip event processing loop (no events)

    time(NULL) - lastCleanup >= 10 ?  (CLEANUP_INTERVAL_SEC = 10)
         │
         ├──► NO:  Continue to next epoll_wait()
         │
         └──► YES: cleanupTimedOutConnections()
                       │
                       └──► Iterate _connections (empty)
                            └──► Nothing to clean up

    └──► Loop back to epoll_wait()
```

## Summary of System Calls During Startup

| Order | System Call | Parameters | Returns |
|-------|-------------|------------|---------|
| 1 | `sigaction` | `SIGINT, handler` | 0 |
| 2 | `sigaction` | `SIGTERM, handler` | 0 |
| 3 | `sigaction` | `SIGPIPE, SIG_IGN` | 0 |
| 4 | `open` | `"config/default.conf"` | fd (file) |
| 5 | `read` | file contents | bytes |
| 6 | `close` | config file fd | 0 |
| 7 | `socket` | `AF_INET, SOCK_STREAM, 0` | 3 |
| 8 | `setsockopt` | `3, SO_REUSEADDR` | 0 |
| 9 | `setsockopt` | `3, SO_REUSEPORT` | 0 |
| 10 | `bind` | `3, 0.0.0.0:8080` | 0 |
| 11 | `fcntl` | `3, F_GETFL` | flags |
| 12 | `fcntl` | `3, F_SETFL, O_NONBLOCK` | 0 |
| 13 | `listen` | `3, 128` | 0 |
| 14 | `epoll_create1` | `0` | 4 |
| 15 | `epoll_ctl` | `4, ADD, 3, EPOLLIN` | 0 |
| 16+ | `epoll_wait` | `4, events, 64, 1000` | 0 (loops) |
