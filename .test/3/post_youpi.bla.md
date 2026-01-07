### webserv log

```
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLOUT
  [Connection fd=5] Sent 629 bytes (629/629)
  [Connection fd=5] Response complete!
  [Connection fd=5] Keep-alive: waiting for next request
  [Connection fd=5] Reset for next request
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 26 bytes (buffer now: 26 bytes)
  [Request] Buffer size: 26, State: 0
  [Request] Buffer: [GET /directory/Yeah/not_ha]
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 94 bytes (buffer now: 94 bytes)
  [Request] Buffer size: 120, State: 0
  [Request] Buffer: [GET /directory/Yeah/not_happy.bad_extension HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: Go-http-client/1.1\r\nAccept-Encod]
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 7 bytes (buffer now: 7 bytes)
  [Request] Buffer size: 19, State: 1
  [Request] Buffer: [Accept-Encoding: gz]
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Request] Buffer size: 20, State: 1
  [Request] Buffer: [Accept-Encoding: gzi]
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Request] Buffer size: 21, State: 1
  [Request] Buffer: [Accept-Encoding: gzip]
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 3 bytes (buffer now: 3 bytes)
  [Request] Buffer size: 24, State: 1
  [Request] Buffer: [Accept-Encoding: gzip\r\n\r]
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Request] Buffer size: 2, State: 1
  [Request] Buffer: [\r\n]
  [Connection fd=5] Keep-alive: yes (HTTP version: HTTP/1.1)
  [Connection fd=5] Request complete!
  Processing: GET /directory/Yeah/not_happy.bad_extension
  [resolvePath] location: /directory/, root: ./YoupiBanane, request: /directory/Yeah/not_happy.bad_extension, remainder: /Yeah/not_happy.bad_extension, result: ./YoupiBanane/Yeah/not_happy.bad_extension
  [resolvePath] location: /directory/, root: ./YoupiBanane, request: /directory/Yeah/not_happy.bad_extension, remainder: /Yeah/not_happy.bad_extension, result: ./YoupiBanane/Yeah/not_happy.bad_extension
  [Connection fd=5] Response queued (187 bytes)
  [Response fd=5] Sending response:
  ----------------------------------------
HTTP/1.1 200 OK\r\n
Connection: keep-alive\r\n
Content-Length: 40\r\n
Content-Type: text/plain\r\n
Date: Mon, 05 Jan 2026 13:27:25 GMT\r\n
Server: webserv/1.0\r\n
\r\n
content of Yeah/not_happy.bad_extension\n

  ----------------------------------------
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLOUT
  [Connection fd=5] Sent 187 bytes (187/187)
  [Connection fd=5] Response complete!
  [Connection fd=5] Keep-alive: waiting for next request
  [Connection fd=5] Reset for next request
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 130 bytes (buffer now: 130 bytes)
  [Request] Buffer size: 130, State: 0
  [Request] Buffer: [POST /directory/youpi.bla HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: Go-http-client/1.1\r\nTransfer-Encoding: chunked\r\nContent-Type]
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 18 bytes (buffer now: 18 bytes)
  [Request] Buffer size: 30, State: 1
  [Request] Buffer: [Content-Type: test/file\r\nAccep]
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Request] Buffer size: 6, State: 1
  [Request] Buffer: [Accept]
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 12 bytes (buffer now: 12 bytes)
  [Request] Buffer size: 18, State: 1
  [Request] Buffer: [Accept-Encoding: g]
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 4 bytes (buffer now: 4 bytes)
  [Request] Buffer size: 22, State: 1
  [Request] Buffer: [Accept-Encoding: gzip\r]
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 2 bytes (buffer now: 2 bytes)
  [Request] Buffer size: 24, State: 1
  [Request] Buffer: [Accept-Encoding: gzip\r\n\r]
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Request] Buffer size: 2, State: 1
  [Request] Buffer: [\r\n]
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1837 bytes (buffer now: 1837 bytes)
  [Request] Buffer size: 1837, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1778 bytes (buffer now: 1778 bytes)
  [Request] Buffer size: 3609, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 76 bytes (buffer now: 76 bytes)
  [Request] Buffer size: 3685, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 166 bytes (buffer now: 166 bytes)
  [Request] Buffer size: 3851, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 115 bytes (buffer now: 115 bytes)
  [Request] Buffer size: 3966, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 68 bytes (buffer now: 68 bytes)
  [Request] Buffer size: 4034, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 43 bytes (buffer now: 43 bytes)
  [Request] Buffer size: 4077, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Request] Buffer size: 4078, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 6 bytes (buffer now: 6 bytes)
  [Request] Buffer size: 4084, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 2 bytes (buffer now: 2 bytes)
  [Request] Buffer size: 4086, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Request] Buffer size: 4087, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Request] Buffer size: 4088, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Request] Buffer size: 4089, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Request] Buffer size: 4090, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 8192 bytes (buffer now: 8192 bytes)
  [Request] Buffer size: 12282, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 8192 bytes (buffer now: 8192 bytes)
  [Request] Buffer size: 20474, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 2728 bytes (buffer now: 2728 bytes)
  [Request] Buffer size: 23202, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1921 bytes (buffer now: 1921 bytes)
  [Request] Buffer size: 25123, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 898 bytes (buffer now: 898 bytes)
  [Request] Buffer size: 26021, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1311 bytes (buffer now: 1311 bytes)
  [Request] Buffer size: 27332, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 5133 bytes (buffer now: 5133 bytes)
  [Request] Buffer size: 32465, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 258 bytes (buffer now: 258 bytes)
  [Request] Buffer size: 32723, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 8 bytes (buffer now: 8 bytes)
  [Request] Buffer size: 32731, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 27 bytes (buffer now: 27 bytes)
  [Request] Buffer size: 32758, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 4 bytes (buffer now: 4 bytes)
  [Request] Buffer size: 32762, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 3 bytes (buffer now: 3 bytes)
  [Request] Buffer size: 32765, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 2 bytes (buffer now: 2 bytes)
  [Request] Buffer size: 32767, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Request] Buffer size: 32768, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Request] Buffer size: 32769, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Request] Buffer size: 32770, State: 3


...


[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 172 bytes (buffer now: 172 bytes)
  [Request] Buffer size: 23453, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1290 bytes (buffer now: 1290 bytes)
  [Request] Buffer size: 24743, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 3 bytes (buffer now: 3 bytes)
  [Request] Buffer size: 24746, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 54 bytes (buffer now: 54 bytes)
  [Request] Buffer size: 24800, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 31 bytes (buffer now: 31 bytes)
  [Request] Buffer size: 24831, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Request] Buffer size: 24832, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Request] Buffer size: 24833, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Request] Buffer size: 24834, State: 3
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 3 bytes (buffer now: 3 bytes)
  [Request] Buffer size: 3, State: 3
  [Request] Buffer: [0\r\n]
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Request] Buffer size: 1, State: 3
  [Request] Buffer: [\r]
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Request] Buffer size: 2, State: 3
  [Request] Buffer: [\r\n]
  [Connection fd=5] Keep-alive: yes (HTTP version: HTTP/1.1)
  [Connection fd=5] Request complete!
  Processing: POST /directory/youpi.bla
  [Connection fd=5] Response queued (656 bytes)
  [Response fd=5] Sending response:
  ----------------------------------------
HTTP/1.1 405 Method Not Allowed\r\n
Connection: keep-alive\r\n
Content-Type: text/html; charset=UTF-8\r\n
Date: Mon, 05 Jan 2026 13:28:02 GMT\r\n
Server: webserv/1.0\r\n
Content-Length: 478\r\n
\r\n
<!DOCTYPE html>\n
<html>\n
<head>\n
    <meta charset="UTF-8">\n
    <title>405 Method Not Allowed</title>\n
    <style>\n
        body { font-family: Arial, sans-serif; text-align: center; padding: 50px; }\n
        h1 { font-size: 50px; color: #333; }\n
        p { color: #666; }\n
        hr { border: none; border-top: 1px solid #ddd; margin: 20px 0; }\n
    </style>\n
</head>\n
<body>\n
    <h1>4
  ----------------------------------------
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLOUT
  [Connection fd=5] Sent 656 bytes (656/656)
  [Connection fd=5] Response complete!
  [Connection fd=5] Keep-alive: waiting for next request
  [Connection fd=5] Reset for next request
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Client disconnected (EOF)
  Closing client fd=5
  Client closed (remaining: 0)
```





### ubuntu_tester output

```console

./ubuntu_tester http://localhost:8080
Welcome in this little webserver tester.
Passing the test here is the minimum before going to an evaluation.

press enter to continue

THIS TEST IS NOT MEANT TO BE THE ONLY TEST IN THE EVALUATION!!!

press enter to continue

Before starting please follow the next few steps (files content can be anything and will be shown to you by the test):
- Download the cgi_test executable on the host
- Create a directory YoupiBanane with:
	-a file name youpi.bad_extension
	-a file name youpi.bla
	-a sub directory called nop
		-a file name youpi.bad_extension in nop
		-a file name other.pouic in nop
	-a sub directory called Yeah
		-a file name not_happy.bad_extension in Yeah
press enter to continue

Setup the configuration file as follow:
- / must answer to GET request ONLY
- /put_test/* must answer to PUT request and save files to a directory of your choice
- any file with .bla as extension must answer to POST request by calling the cgi_test executable
- /post_body must answer anything to POST request with a maxBody of 100
- /directory/ must answer to GET request and the root of it would be the repository YoupiBanane and if no file are requested, it should search for youpi.bad_extension files

press enter to continue

Before starting please verify that the server is launched
press enter to continue


...


Test GET http://localhost:8080/directory/Yeah/not_happy.bad_extension
content returned: content of Yeah/not_happy.bad_extension


Test POST http://localhost:8080/directory/youpi.bla with a size of 100000000
FATAL ERROR ON LAST TEST: bad status code
```






## The `curl` commands to reproduce the issue.

### 1. Simple POST Request
This sends a small POST request to the specific URL. If you get a **405 Method Not Allowed**, it confirms the server is matching the `/directory/` location (GET only) instead of the `*.bla` location (POST allowed).

```bash
curl -v -X POST -d "test data" http://localhost:8080/directory/youpi.bla
```

### 2. Chunked POST Request (Closer to Tester)
The tester uses `Transfer-Encoding: chunked`. Use this to match the tester's behavior more closely:

```bash
curl -v -X POST -H "Transfer-Encoding: chunked" -d "test data" http://localhost:8080/directory/youpi.bla
```

### Debugging Tip
If these commands return **405**, the `findLocation` logic is failing to match `*.bla`. You can add a temporary debug print




## Some Tests similar to those required for the webserv project

Test GET http://localhost:1234/
Test POST http://localhost:1234/ with a size of 0
Test HEAD http://localhost:1234/
Test GET http://localhost:1234/directory
Test GET http://localhost:1234/directory/youpi.bad_extension
Test GET http://localhost:1234/directory/youpi.bla
Test GET Expected 404 on http://localhost:1234/directory/oulalala
Test GET http://localhost:1234/directory/nop
Test GET http://localhost:1234/directory/nop/
Test GET http://localhost:1234/directory/nop/other.pouic
Test GET Expected 404 on http://localhost:1234/directory/nop/other.pouac
Test GET Expected 404 on http://localhost:1234/directory/Yeah
Test GET http://localhost:1234/directory/Yeah/not_happy.bad_extension
Test Put http://localhost:1234/put_test/file_should_exist_after with a size of 1000
Test Put http://localhost:1234/put_test/file_should_exist_after with a size of 10000000
Test POST http://localhost:1234/directory/youpi.bla with a size of 100000000
Test POST http://localhost:1234/directory/youpla.bla with a size of 100000000
Test POST http://localhost:1234/directory/youpi.bla with a size of 100000 with special headers
Test POST http://localhost:1234/post_body with a size of 0
Test POST http://localhost:1234/post_body with a size of 100
Test POST http://localhost:1234/post_body with a size of 200
Test POST http://localhost:1234/post_body with a size of 101
Test multiple workers(5) doing multiple times(15): GET on /
Test multiple workers(20) doing multiple times(5000): GET on /
Test multiple workers(128) doing multiple times(50): GET on /directory/nop
Test multiple workers(20) doing multiple times(5): Put on /put_test/multiple_same with size 1000000
Test multiple workers(20) doing multiple times(5): Post on /directory/youpi.bla with size 100000000






## TODO

```
Test POST http://localhost:8080/directory/youpi.bla with a size of 100000000
FATAL ERROR ON LAST TEST: bad status code
```


[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Received 1 bytes (buffer now: 1 bytes)
  [Request] Buffer size: 2, State: 3
  [Request] Buffer: [\r\n]
  [Connection fd=5] Keep-alive: yes (HTTP version: HTTP/1.1)
  [Connection fd=5] Request complete!
  Processing: POST /directory/youpi.bla
  [resolvePath] location: *.bla, root: , request: /directory/youpi.bla, remainder: /ctory/youpi.bla, result: /ctory/youpi.bla
  [Connection fd=5] Response queued (629 bytes)
  [Response fd=5] Sending response:
  ----------------------------------------
HTTP/1.1 403 Forbidden\r\n
Connection: keep-alive\r\n
Content-Type: text/html; charset=UTF-8\r\n
Date: Mon, 05 Jan 2026 16:34:12 GMT\r\n
Server: webserv/1.0\r\n
Content-Length: 460\r\n
\r\n
<!DOCTYPE html>\n
<html>\n
<head>\n
    <meta charset="UTF-8">\n
    <title>403 Forbidden</title>\n
    <style>\n
        body { font-family: Arial, sans-serif; text-align: center; padding: 50px; }\n
        h1 { font-size: 50px; color: #333; }\n
        p { color: #666; }\n
        hr { border: none; border-top: 1px solid #ddd; margin: 20px 0; }\n
    </style>\n
</head>\n
<body>\n
    <h1>403</h1>\n
    <p>For
  ----------------------------------------
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLOUT
  [Connection fd=5] Sent 629 bytes (629/629)
  [Connection fd=5] Response complete!
  [Connection fd=5] Keep-alive: waiting for next request
  [Connection fd=5] Reset for next request
[epoll] 1 event(s) ready
  Event on fd=5: EPOLLIN
  [Connection fd=5] Client disconnected (EOF)
  Closing client fd=5
  Client closed (remaining: 0)


---


# TIMEOUT

I see the problem. The timeout mechanism is broken because the `waitpid()` call with `WNOHANG` in the read loop doesn't properly handle the case when the child is still running. Also, the `time()` comparison is using `>=` which means it checks for `timeout` seconds exactly, but the loop continues blocking on `usleep()` calls.

The main issue is that your read loop with non-blocking I/O and `usleep(10000)` is essentially busy-waiting, but the timeout check `time(NULL) - startTime >= timeout` uses seconds, which is too coarse-grained.

Here's a fixed version using `poll()` for proper timeout handling:

````cpp
// ...existing code...

#include <poll.h>  // Add this include at the top with other includes

// ...existing code...

CGI::CGIResult CGI::execute(int timeout)
{
	CGIResult result;

	// Initialize pipe FDs to -1 (invalid) for safe cleanup
	int stdin_pipe[2] = {-1, -1};
	int stdout_pipe[2] = {-1, -1};

	// =========================================
	//  Pre-flight Checks
	// =========================================
	if (!_ready)
	{
		result.success = false;
		result.statusCode = 500;
		result.errorMessage = "CGI not ready - setup() was not called or failed";
		return result;
	}

	// =========================================
	//  Step 1: Create Pipes for Communication
	// =========================================
	if (pipe(stdin_pipe) == -1)
	{
		result.success = false;
		result.statusCode = 500;
		result.errorMessage = "Failed to create stdin pipe: ";
		result.errorMessage += strerror(errno);
		return result;
	}

	if (pipe(stdout_pipe) == -1)
	{
		closePipes(stdin_pipe, NULL);
		result.success = false;
		result.statusCode = 500;
		result.errorMessage = "Failed to create stdout pipe: ";
		result.errorMessage += strerror(errno);
		return result;
	}

	// =========================================
	//  Step 2: Fork Child Process
	// =========================================
	pid_t pid = fork();

	if (pid == -1)
	{
		closePipes(stdin_pipe, stdout_pipe);
		result.success = false;
		result.statusCode = 500;
		result.errorMessage = "Failed to fork process: ";
		result.errorMessage += strerror(errno);
		return result;
	}

	// =========================================
	//  Child Process (pid == 0)
	// =========================================
	if (pid == 0)
	{
		// Redirect stdin to read from pipe
		if (dup2(stdin_pipe[0], STDIN_FILENO) == -1)
		{
			_exit(1);
		}

		// Redirect stdout to write to pipe
		if (dup2(stdout_pipe[1], STDOUT_FILENO) == -1)
		{
			_exit(1);
		}

		// Close all pipe FDs (we have copies at 0 and 1 now)
		close(stdin_pipe[0]);
		close(stdin_pipe[1]);
		close(stdout_pipe[0]);
		close(stdout_pipe[1]);

		// Change to script directory
		if (!_workingDirectory.empty())
		{
			if (chdir(_workingDirectory.c_str()) == -1)
			{
				// Non-fatal: script might work anyway with absolute paths
			}
		}

		// Prepare argv and envp
		char** argv = getArgv();
		char** envp = getEnvArray();

		if (!argv || !envp)
		{
			if (argv) freeArgv(argv);
			if (envp) freeEnvArray(envp);
			_exit(1);
		}

		// Execute the CGI interpreter
		execve(_interpreterPath.c_str(), argv, envp);

		// If we get here, execve failed
		freeArgv(argv);
		freeEnvArray(envp);
		_exit(2);
	}

	// =========================================
	//  Parent Process (pid > 0)
	// =========================================

	// Close pipe ends we don't need
	close(stdin_pipe[0]);
	stdin_pipe[0] = -1;
	close(stdout_pipe[1]);
	stdout_pipe[1] = -1;

	// =========================================
	//  Step 3: Write Request Body to Child
	// =========================================
	const std::string& requestBody = getRequestBody();
	if (!requestBody.empty())
	{
		// Set write end to non-blocking to avoid hanging on large bodies
		setNonBlocking(stdin_pipe[1]);

		size_t totalWritten = 0;
		size_t bodySize = requestBody.size();
		const char* bodyData = requestBody.c_str();

		while (totalWritten < bodySize)
		{
			struct pollfd pfd;
			pfd.fd = stdin_pipe[1];
			pfd.events = POLLOUT;
			pfd.revents = 0;

			int pollResult = poll(&pfd, 1, timeout * 1000);

			if (pollResult == 0)
			{
				// Timeout writing to CGI
				close(stdin_pipe[1]);
				stdin_pipe[1] = -1;
				close(stdout_pipe[0]);
				stdout_pipe[0] = -1;
				cleanupChild(pid);

				result.success = false;
				result.statusCode = 504;
				result.errorMessage = "Timeout writing request body to CGI";
				return result;
			}
			else if (pollResult < 0)
			{
				break;  // Error, stop writing
			}

			ssize_t written = write(stdin_pipe[1], bodyData + totalWritten,
									bodySize - totalWritten);
			if (written > 0)
			{
				totalWritten += written;
			}
			else if (written == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
			{
				break;  // Write error
			}
		}
	}

	// Close write end to signal EOF to child
	close(stdin_pipe[1]);
	stdin_pipe[1] = -1;

	// =========================================
	//  Step 4: Read CGI Output with Timeout
	// =========================================
	setNonBlocking(stdout_pipe[0]);

	std::string cgiOutput;
	char buffer[4096];
	int timeoutMs = timeout * 1000;  // Convert to milliseconds
	time_t startTime = time(NULL);

	while (true)
	{
		// Calculate remaining timeout
		time_t elapsed = time(NULL) - startTime;
		int remainingMs = timeoutMs - (elapsed * 1000);

		if (remainingMs <= 0)
		{
			// Timeout reached
			close(stdout_pipe[0]);
			stdout_pipe[0] = -1;
			cleanupChild(pid);

			result.success = false;
			result.statusCode = 504;
			std::ostringstream oss;
			oss << "CGI script execution timed out after " << timeout << " seconds";
			result.errorMessage = oss.str();
			return result;
		}

		// Use poll() for proper timeout handling
		struct pollfd pfd;
		pfd.fd = stdout_pipe[0];
		pfd.events = POLLIN;
		pfd.revents = 0;

		int pollResult = poll(&pfd, 1, remainingMs > 1000 ? 1000 : remainingMs);

		if (pollResult == 0)
		{
			// poll timeout - check if child has exited
			int status;
			pid_t waitResult = waitpid(pid, &status, WNOHANG);

			if (waitResult == pid)
			{
				// Child exited, read any remaining data
				while (true)
				{
					ssize_t bytesRead = read(stdout_pipe[0], buffer, sizeof(buffer) - 1);
					if (bytesRead > 0)
					{
						cgiOutput.append(buffer, bytesRead);
					}
					else
					{
						break;
					}
				}

				close(stdout_pipe[0]);
				stdout_pipe[0] = -1;

				// Check exit status
				if (!WIFEXITED(status))
				{
					result.success = false;
					result.statusCode = 500;

					if (WIFSIGNALED(status))
					{
						int sig = WTERMSIG(status);
						std::ostringstream oss;
						oss << "CGI script killed by signal " << sig;
						result.errorMessage = oss.str();
					}
					else
					{
						result.errorMessage = "CGI script terminated abnormally";
					}
					return result;
				}

				int exitCode = WEXITSTATUS(status);
				if (exitCode == 2 && cgiOutput.empty())
				{
					result.success = false;
					result.statusCode = 500;
					result.errorMessage = "Failed to execute CGI interpreter";
					return result;
				}

				break;  // Exit read loop, we have all output
			}

			// Child still running, continue polling
			continue;
		}
		else if (pollResult < 0)
		{
			if (errno == EINTR)
			{
				continue;  // Interrupted by signal, retry
			}
			// Poll error
			close(stdout_pipe[0]);
			stdout_pipe[0] = -1;
			cleanupChild(pid);

			result.success = false;
			result.statusCode = 500;
			result.errorMessage = "poll() error reading CGI output: ";
			result.errorMessage += strerror(errno);
			return result;
		}

		// Data available or pipe closed
		if (pfd.revents & (POLLIN | POLLHUP))
		{
			ssize_t bytesRead = read(stdout_pipe[0], buffer, sizeof(buffer) - 1);

			if (bytesRead > 0)
			{
				cgiOutput.append(buffer, bytesRead);
			}
			else if (bytesRead == 0)
			{
				// EOF - child closed stdout
				close(stdout_pipe[0]);
				stdout_pipe[0] = -1;

				// Wait for child to fully exit
				int status;
				waitpid(pid, &status, 0);

				if (!WIFEXITED(status))
				{
					result.success = false;
					result.statusCode = 500;

					if (WIFSIGNALED(status))
					{
						int sig = WTERMSIG(status);
						std::ostringstream oss;
						oss << "CGI script killed by signal " << sig;
						result.errorMessage = oss.str();
					}
					else
					{
						result.errorMessage = "CGI script terminated abnormally";
					}
					return result;
				}

				break;  // Exit read loop
			}
			else
			{
				// bytesRead < 0
				if (errno == EAGAIN || errno == EWOULDBLOCK)
				{
					continue;
				}
				// Read error
				close(stdout_pipe[0]);
				stdout_pipe[0] = -1;
				cleanupChild(pid);

				result.success = false;
				result.statusCode = 500;
				result.errorMessage = "Error reading CGI output: ";
				result.errorMessage += strerror(errno);
				return result;
			}
		}

		if (pfd.revents & POLLERR)
		{
			// Error on pipe
			close(stdout_pipe[0]);
			stdout_pipe[0] = -1;
			cleanupChild(pid);

			result.success = false;
			result.statusCode = 500;
			result.errorMessage = "Pipe error while reading CGI output";
			return result;
		}
	}

	// =========================================
	//  Step 5: Validate Output
	// =========================================
	if (cgiOutput.empty())
	{
		result.success = false;
		result.statusCode = 500;
		result.errorMessage = "CGI script produced no output";
		return result;
	}

	// =========================================
	//  Step 6: Parse CGI Output
	// =========================================
	if (!parseCgiOutput(cgiOutput, result))
	{
		return result;
	}

	// =========================================
	//  Success!
	// =========================================
	result.success = true;
	return result;
}

// ...existing code...
````

Key changes:

1. **Uses `poll()` with timeout** - Instead of `usleep()` busy-waiting, we use `poll()` which properly blocks until data is available OR timeout expires.

2. **Handles writing large request bodies** - For POST requests with large bodies (like your 100MB test), we now use `poll()` on the write side too with timeout.

3. **Proper timeout calculation** - We track remaining time and pass it to `poll()`.

4. **Handles `POLLHUP`** - When the child closes its stdout, we get `POLLHUP` and handle it properly.

5. **Added `#include <poll.h>`** - Make sure this is included at the top of the file.

The timeout should now work correctly and the server won't block indefinitely.








# Second Attempt


I see the problem! The issue is that writing 100MB to the CGI's stdin is blocking while the CGI is trying to write to stdout. This creates a **deadlock**:

1. Parent tries to write 100MB to child's stdin
2. Child reads some data, processes it, tries to write to stdout
3. Stdout pipe buffer fills up (~64KB on Linux)
4. Child blocks waiting for parent to read from stdout
5. Parent is still trying to write to stdin → **DEADLOCK**

The pipe buffers are only ~64KB, so neither side can make progress.

## The Fix: Read and Write Simultaneously

We need to use `poll()` to monitor **both** the stdin write and stdout read simultaneously:

````cpp
// ...existing code for execute() until Step 3...

	// =========================================
	//  Step 3 & 4: Write Body AND Read Output Simultaneously
	// =========================================
	/*
		CRITICAL: We must read and write at the same time!

		Why? Pipe buffers are limited (~64KB on Linux).
		If the CGI produces output while we're still writing input,
		and we don't read the output, the CGI will block on write(),
		and we'll block on write() too = DEADLOCK.

		Solution: Use poll() to monitor both pipes and handle
		whichever one is ready.
	*/

	setNonBlocking(stdin_pipe[1]);
	setNonBlocking(stdout_pipe[0]);

	const std::string& requestBody = getRequestBody();
	size_t totalWritten = 0;
	size_t bodySize = requestBody.size();
	const char* bodyData = requestBody.c_str();
	bool doneWriting = requestBody.empty();

	std::string cgiOutput;
	char buffer[65536];  // Larger buffer for efficiency
	time_t startTime = time(NULL);

	#if DEBUG >= 1
	std::cerr << "  [CGI] Request body size: " << requestBody.size() << " bytes" << std::endl;
	if (requestBody.size() > 0 && requestBody.size() <= 1000)
	{
		std::cerr << "  [CGI] Request body (first 500 chars): ["
				<< requestBody.substr(0, 500) << "]" << std::endl;
	}
	else if (requestBody.size() > 1000)
	{
		std::cerr << "  [CGI] Request body preview (first 200 chars): ["
				<< requestBody.substr(0, 200) << "]..." << std::endl;
		std::cerr << "  [CGI] Request body preview (last 200 chars): [..."
				<< requestBody.substr(requestBody.size() - 200) << "]" << std::endl;
	}
	#endif

	while (true)
	{
		// Check timeout
		time_t elapsed = time(NULL) - startTime;
		if (elapsed >= timeout)
		{
			if (stdin_pipe[1] >= 0)
			{
				close(stdin_pipe[1]);
				stdin_pipe[1] = -1;
			}
			close(stdout_pipe[0]);
			stdout_pipe[0] = -1;
			cleanupChild(pid);

			result.success = false;
			result.statusCode = 504;
			std::ostringstream oss;
			oss << "CGI script execution timed out after " << timeout << " seconds";
			result.errorMessage = oss.str();
			return result;
		}

		// Setup poll for both pipes
		struct pollfd pfds[2];
		int nfds = 0;
		int stdinIndex = -1;
		int stdoutIndex = -1;

		// Add stdout pipe (always monitor for reading)
		stdoutIndex = nfds;
		pfds[nfds].fd = stdout_pipe[0];
		pfds[nfds].events = POLLIN;
		pfds[nfds].revents = 0;
		nfds++;

		// Add stdin pipe only if we still have data to write
		if (!doneWriting && stdin_pipe[1] >= 0)
		{
			stdinIndex = nfds;
			pfds[nfds].fd = stdin_pipe[1];
			pfds[nfds].events = POLLOUT;
			pfds[nfds].revents = 0;
			nfds++;
		}

		// Poll with 1 second timeout (to check overall timeout)
		int pollResult = poll(pfds, nfds, 1000);

		if (pollResult < 0)
		{
			if (errno == EINTR)
			{
				continue;  // Interrupted by signal, retry
			}
			// Poll error
			if (stdin_pipe[1] >= 0)
			{
				close(stdin_pipe[1]);
				stdin_pipe[1] = -1;
			}
			close(stdout_pipe[0]);
			stdout_pipe[0] = -1;
			cleanupChild(pid);

			result.success = false;
			result.statusCode = 500;
			result.errorMessage = "poll() error: ";
			result.errorMessage += strerror(errno);
			return result;
		}

		// Handle stdin (writing to CGI)
		if (stdinIndex >= 0 && (pfds[stdinIndex].revents & POLLOUT))
		{
			ssize_t written = write(stdin_pipe[1], bodyData + totalWritten,
									bodySize - totalWritten);
			if (written > 0)
			{
				totalWritten += written;
				if (totalWritten >= bodySize)
				{
					// Done writing, close stdin to signal EOF
					close(stdin_pipe[1]);
					stdin_pipe[1] = -1;
					doneWriting = true;
					#if DEBUG >= 1
					std::cerr << "  [CGI] Finished writing " << totalWritten << " bytes to stdin" << std::endl;
					#endif
				}
			}
			else if (written == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
			{
				// Write error - close stdin and stop writing
				close(stdin_pipe[1]);
				stdin_pipe[1] = -1;
				doneWriting = true;
			}
		}

		// Check for stdin errors
		if (stdinIndex >= 0 && (pfds[stdinIndex].revents & (POLLERR | POLLHUP)))
		{
			// Pipe closed or error - stop writing
			if (stdin_pipe[1] >= 0)
			{
				close(stdin_pipe[1]);
				stdin_pipe[1] = -1;
			}
			doneWriting = true;
		}

		// Handle stdout (reading from CGI)
		if (pfds[stdoutIndex].revents & POLLIN)
		{
			ssize_t bytesRead = read(stdout_pipe[0], buffer, sizeof(buffer));
			if (bytesRead > 0)
			{
				cgiOutput.append(buffer, bytesRead);
			}
			else if (bytesRead == 0)
			{
				// EOF - CGI closed stdout
				close(stdout_pipe[0]);
				stdout_pipe[0] = -1;

				// Close stdin if still open
				if (stdin_pipe[1] >= 0)
				{
					close(stdin_pipe[1]);
					stdin_pipe[1] = -1;
				}

				// Wait for child to exit
				int status;
				waitpid(pid, &status, 0);

				if (!WIFEXITED(status))
				{
					result.success = false;
					result.statusCode = 500;

					if (WIFSIGNALED(status))
					{
						int sig = WTERMSIG(status);
						std::ostringstream oss;
						oss << "CGI script killed by signal " << sig;
						result.errorMessage = oss.str();
					}
					else
					{
						result.errorMessage = "CGI script terminated abnormally";
					}
					return result;
				}

				int exitCode = WEXITSTATUS(status);
				if (exitCode == 2 && cgiOutput.empty())
				{
					result.success = false;
					result.statusCode = 500;
					result.errorMessage = "Failed to execute CGI interpreter";
					return result;
				}

				#if DEBUG >= 1
				std::cerr << "  [CGI] Read " << cgiOutput.size() << " bytes from stdout" << std::endl;
				#endif

				break;  // Exit loop - we're done
			}
			// bytesRead < 0 with EAGAIN is normal, just continue
		}

		// Check for stdout hangup (CGI exited)
		if (pfds[stdoutIndex].revents & POLLHUP)
		{
			// Read any remaining data
			while (true)
			{
				ssize_t bytesRead = read(stdout_pipe[0], buffer, sizeof(buffer));
				if (bytesRead > 0)
				{
					cgiOutput.append(buffer, bytesRead);
				}
				else
				{
					break;
				}
			}

			close(stdout_pipe[0]);
			stdout_pipe[0] = -1;

			if (stdin_pipe[1] >= 0)
			{
				close(stdin_pipe[1]);
				stdin_pipe[1] = -1;
			}

			// Wait for child
			int status;
			waitpid(pid, &status, 0);

			if (!WIFEXITED(status))
			{
				result.success = false;
				result.statusCode = 500;

				if (WIFSIGNALED(status))
				{
					int sig = WTERMSIG(status);
					std::ostringstream oss;
					oss << "CGI script killed by signal " << sig;
					result.errorMessage = oss.str();
				}
				else
				{
					result.errorMessage = "CGI script terminated abnormally";
				}
				return result;
			}

			break;
		}

		// Check for stdout errors
		if (pfds[stdoutIndex].revents & POLLERR)
		{
			close(stdout_pipe[0]);
			stdout_pipe[0] = -1;

			if (stdin_pipe[1] >= 0)
			{
				close(stdin_pipe[1]);
				stdin_pipe[1] = -1;
			}

			cleanupChild(pid);

			result.success = false;
			result.statusCode = 500;
			result.errorMessage = "Pipe error while reading CGI output";
			return result;
		}

		// If poll returned 0 (timeout), check if child exited
		if (pollResult == 0)
		{
			int status;
			pid_t waitResult = waitpid(pid, &status, WNOHANG);

			if (waitResult == pid)
			{
				// Child exited, drain remaining output
				while (true)
				{
					ssize_t bytesRead = read(stdout_pipe[0], buffer, sizeof(buffer));
					if (bytesRead > 0)
					{
						cgiOutput.append(buffer, bytesRead);
					}
					else
					{
						break;
					}
				}

				close(stdout_pipe[0]);
				stdout_pipe[0] = -1;

				if (stdin_pipe[1] >= 0)
				{
					close(stdin_pipe[1]);
					stdin_pipe[1] = -1;
				}

				if (!WIFEXITED(status))
				{
					result.success = false;
					result.statusCode = 500;

					if (WIFSIGNALED(status))
					{
						int sig = WTERMSIG(status);
						std::ostringstream oss;
						oss << "CGI script killed by signal " << sig;
						result.errorMessage = oss.str();
					}
					else
					{
						result.errorMessage = "CGI script terminated abnormally";
					}
					return result;
				}

				break;
			}
		}
	}

	// =========================================
	//  Step 5: Validate Output
	// =========================================
	// ...existing code...
````

## Key Changes:

1. **Single `poll()` loop** monitors both stdin (write) and stdout (read) simultaneously
2. **No more deadlock** - we read CGI output while still writing input
3. **Larger buffer** (64KB) for more efficient reads
4. **Proper handling of `POLLHUP`** - drain remaining data when CGI closes stdout

## Also Important: Non-blocking Server

Your current design has another issue - the CGI execution blocks the entire server's event loop. For a production server, you'd want to:

1. Make CGI execution non-blocking (fork and return immediately)
2. Add the stdout pipe FD to the main epoll loop
3. Handle CGI output in the event loop like any other I/O

But for now, this fix should get the 100MB test passing by preventing the deadlock.












# client_max_body_size


I'll help you fix the config parsing to support `client_max_body_size` in location blocks and then update the Request class to use it properly.

## Step 1: Update `LocationConfig` to include `client_max_body_size`

First, we need to modify the `Config.hpp` file to add the field to `LocationConfig`:

````cpp
// ...existing code...

struct LocationConfig
{
    std::string path;
    std::string root;
    std::string index;
    std::string upload_path;
    std::string cgi_extension;
    std::string cgi_path;
    std::string redirect_url;
    int redirect_code;
    bool autoindex;
    std::set<std::string> allowed_methods;
    size_t client_max_body_size;  // Add this field

    LocationConfig();
};

// ...existing code...
````

## Step 2: Update `LocationConfig` constructor

````cpp
// ...existing code...

LocationConfig::LocationConfig() :
	path("/"),
	root(""),
	index("index.html"),
	upload_path(""),
	cgi_extension(""),
	cgi_path(""),
	redirect_url(""),
	redirect_code(0),
	autoindex(false),
	client_max_body_size(0)  // 0 means "inherit from server"
{
	// By default, only allow GET, the safest HTTP method
	// POST and DELETE must be explicitly enabled in config
	allowed_methods.insert("GET");
}

// ...existing code...
````

## Step 3: Parse `client_max_body_size` in location blocks

````cpp
// ...existing code... (inside parseLocationBlock, after the cgi_extension directive handling)

		else if (directive == "cgi_extension")
		{
			/*
				cgi_extension directive: file extension to trigger CGI
				Example: cgi_extension .php
				Files ending in .php will be executed via CGI
			*/
			if (tokens.size() < 2)
			{
				throw ConfigException("cgi_extension directive requires an extension");
			}
			location.cgi_extension = tokens[1];
		}
		else if (directive == "client_max_body_size")
		{
			/*
				client_max_body_size directive in location block
				Overrides the server-level setting for this specific location
				Example: client_max_body_size 100M;
			*/
			if (tokens.size() < 2)
			{
				throw ConfigException("client_max_body_size directive requires a value");
			}
			location.client_max_body_size = parseSize(tokens[1]);
		}
		else if (directive == "return")
		{
// ...existing code...
````

## Step 4: Add a helper method to `ServerConfig` to get the effective max body size

Add this to your `Config.hpp`:

````cpp
// ...existing code...

struct ServerConfig
{
    std::string host;
    int port;
    std::vector<std::string> server_names;
    std::map<int, std::string> error_pages;
    size_t client_max_body_size;
    std::vector<LocationConfig> locations;

    ServerConfig();

    // Get effective max body size for a location (location value if set, otherwise server value)
    size_t getMaxBodySize(const LocationConfig* loc) const;

    // Find the best matching location for a URI
    const LocationConfig* findLocation(const std::string& uri) const;
};

// ...existing code...
````

## Step 5: Implement the helper methods in Config.cpp

````cpp
// ...existing code... (after ServerConfig constructor)

/*
	getMaxBodySize: Get the effective client_max_body_size for a location

	Priority:
		1. If location has client_max_body_size set (> 0), use it
		2. Otherwise, use the server's client_max_body_size

	This allows location-specific overrides like:
		location /upload { client_max_body_size 100M; }
		location /api { client_max_body_size 1K; }
*/
size_t ServerConfig::getMaxBodySize(const LocationConfig* loc) const
{
	if (loc && loc->client_max_body_size > 0)
	{
		return loc->client_max_body_size;
	}
	return client_max_body_size;
}

/*
	findLocation: Find the best matching location for a URI

	HTTP servers use "longest prefix match" to find the right location.
	For URI "/images/photo.jpg":
		- location "/" matches (prefix "/" is in URI)
		- location "/images" matches better (longer prefix)
		- location "/images/photo.jpg" would match best if it existed
*/
const LocationConfig* ServerConfig::findLocation(const std::string& uri) const
{
	const LocationConfig* bestMatch = NULL;
	size_t longestMatch = 0;

	for (size_t i = 0; i < locations.size(); ++i)
	{
		const std::string& locPath = locations[i].path;

		// Handle extension matching (e.g. *.bla)
		if (locPath.length() > 1 && locPath[0] == '*')
		{
			std::string suffix = locPath.substr(1);
			if (uri.length() >= suffix.length() &&
				uri.compare(uri.length() - suffix.length(), suffix.length(), suffix) == 0)
			{
				return &locations[i];
			}
			continue;
		}

		// Check if this location's path is a prefix of the requested URI
		if (uri.compare(0, locPath.length(), locPath) == 0)
		{
			// For non-root locations, ensure we match at a path boundary
			if (locPath != "/" && uri.length() > locPath.length()
				&& uri[locPath.length()] != '/')
			{
				continue;
			}

			// Keep track of the longest (most specific) match
			if (locPath.length() > longestMatch)
			{
				longestMatch = locPath.length();
				bestMatch = &locations[i];
			}
		}
	}
	return bestMatch;
}

// ...existing code...
````

## Step 6: Update `Request` class to accept max body size

Now we need to update `Request.hpp` and Request.cpp to use the configured max body size:

````cpp
// ...existing code...

class Request
{
public:
    // ...existing code...

    // Set the maximum allowed body size (from config)
    void setMaxBodySize(size_t maxSize);

    // ...existing code...

private:
    // ...existing code...
    size_t _maxBodySize;  // Add this field
    // ...existing code...
};

// ...existing code...
````

## Step 7: Update Request.cpp constructor and methods

````cpp
// ...existing code...

// Default Constructor
Request::Request() :
	_method(""),
	_uri(""),
	_path(""),
	_queryString(""),
	_httpVersion(""),
	_body(""),
	_clientIP(""),
	_state(PARSE_REQUEST_LINE),
	_errorCode(0),
	_buffer(""),
	_contentLength(0),
	_bodyBytesRead(0),
	_expectedChunkSize(0),
	_maxBodySize(1048576)  // Default 1MB
{}

// ...existing code...

// Copy constructor
Request::Request(const Request& other) :
	_method(other._method),
	_uri(other._uri),
	_path(other._path),
	_queryString(other._queryString),
	_httpVersion(other._httpVersion),
	_body(other._body),
	_clientIP(other._clientIP),
	_state(other._state),
	_errorCode(other._errorCode),
	_buffer(other._buffer),
	_contentLength(other._contentLength),
	_bodyBytesRead(other._bodyBytesRead),
	_expectedChunkSize(other._expectedChunkSize),
	_maxBodySize(other._maxBodySize)
{}

// copy assignment operator
Request& Request::operator=(const Request& other)
{
	if (this != &other)
	{
		_method = other._method;
		_uri = other._uri;
		_path = other._path;
		_queryString = other._queryString;
		_httpVersion = other._httpVersion;
		_body = other._body;
		_clientIP = other._clientIP;
		_state = other._state;
		_errorCode = other._errorCode;
		_buffer = other._buffer;
		_contentLength = other._contentLength;
		_bodyBytesRead = other._bodyBytesRead;
		_expectedChunkSize = other._expectedChunkSize;
		_maxBodySize = other._maxBodySize;
	}
	return *this;
}

// ...existing code...

void Request::reset()
{
	_method.clear();
	_uri.clear();
	_path.clear();
	_queryString.clear();
	_httpVersion.clear();
	_body.clear();
	_clientIP.clear();
	_headers.clear();
	_buffer.clear();

	_state = PARSE_REQUEST_LINE;
	_errorCode = 0;
	_contentLength = 0;
	_bodyBytesRead = 0;
	_expectedChunkSize = 0;
	// Note: Don't reset _maxBodySize here - it should persist for the connection
}

// ...existing code...
````

## Step 8: Update the body size check in `Request::parse()`

````cpp
// ...existing code... (in the PARSE_HEADERS state, where Content-Length is checked)

				if (!contentLength.empty())
				{
					// Content-Length body
					_contentLength = std::atol(contentLength.c_str());

					// Check against client_max_body_size from config
					if (_contentLength > _maxBodySize)
					{
						_state = PARSE_ERROR;
						_errorCode = 413; // Payload Too Large
						return true;
					}

					if (_contentLength > 0)
					{
						// Request has a body, move to body parsing
						_state = PARSE_BODY;
					}
					else
					{
						// No body (Content-Length: 0)
						_state = PARSE_COMPLETE;
					}
				}

// ...existing code...
````

## Step 9: Update chunked body size check

````cpp
// ...existing code... (in parseChunkedBody, Step 4: Enforce Body Size Limit)

		// ===================================
		//  Step 4: Enforce Body Size Limit
		// ===================================
		/*
			Even with chunked encoding, we must limit total body size
			Prevent DoS attacks with infinite chunks
		*/
		if (_body.size() + chunkSize > _maxBodySize)
		{
			_state = PARSE_ERROR;
			_errorCode = 413; // Payload Too Large
			return false;
		}

// ...existing code...
````

## Step 10: Add the setter method

````cpp
// ...existing code... (at the end with other setters/getters)

void Request::setMaxBodySize(size_t maxSize)
{
	_maxBodySize = maxSize;
}

// ...existing code...
````

## Step 11: Usage in your connection handler

In your server code where you handle connections, you'll need to set the max body size after you know which location the request is for. This typically happens after parsing the request line:

````cpp
// Example usage in your connection/client handler:
// After parsing request line but before parsing body

// Get the server config for this connection
const ServerConfig* serverConfig = config.getServerByHostPort(host, port);
if (serverConfig)
{
    // Find the matching location
    const LocationConfig* location = serverConfig->findLocation(request.getPath());

    // Set the max body size based on config
    size_t maxBodySize = serverConfig->getMaxBodySize(location);
    request.setMaxBodySize(maxBodySize);
}
````

This solution:
1. Adds `client_max_body_size` to `LocationConfig` with a default of 0 (meaning inherit from server)
2. Parses the directive in location blocks
3. Provides `getMaxBodySize()` helper that returns location value if set, otherwise server value
4. Updates `Request` to use a configurable max body size instead of hardcoded values
5. Works with your existing config file format (like `client_max_body_size 100M;` in the `*.bla` location)





















# request.setMaxBodySize(maxBodySize) during parsing

You're right! We need to set the max body size during parsing, specifically after we have the headers (so we know the path and can do location matching). This requires passing the server config to the Request or having a callback mechanism.

Here's the cleanest approach - add a method to set max body size based on config after headers are parsed:

## Step 1: Add a method to update max body size in Request

````cpp
// ...existing code...

class Request
{
public:
    // ...existing code...

    // Update max body size based on config (call after headers are parsed)
    void updateMaxBodySizeFromConfig(const ServerConfig& server);

    // ...existing code...
};

// ...existing code...
````

## Step 2: Implement the method in Request.cpp

````cpp
// ...existing code... (add after setMaxBodySize)

/*
    updateMaxBodySizeFromConfig() - Update max body size based on server config

    This should be called after headers are parsed, when we know the request path.
    It performs location matching and sets the appropriate max body size.

    Priority:
        1. Location's client_max_body_size (if > 0)
        2. Server's client_max_body_size
*/
void Request::updateMaxBodySizeFromConfig(const ServerConfig& server)
{
    // Find the matching location for this request path
    const LocationConfig* location = server.findLocation(_path);

    // Get the effective max body size
    _maxBodySize = server.getMaxBodySize(location);

    #if DEBUG >= 1
    std::cout << "  [Request] Updated max body size to " << _maxBodySize
              << " for path: " << _path << std::endl;
    #endif
}

// ...existing code...
````

## Step 3: Update the parse() method to accept an optional server config

The tricky part is that during parsing, we need to update the max body size **after headers are complete but before checking Content-Length**. Here's how to modify the `parse()` method:

````cpp
// ...existing code...

/*
    parse() - Main parsing function, feeds incoming data to the request parser

    ...existing comments...

    The optional serverConfig parameter allows updating max body size
    based on location matching after headers are parsed.
*/
bool Request::parse(const std::string& data, const ServerConfig* serverConfig)
{
    // Add incoming data to the buffer
    _buffer += data;

    // ...existing DEBUG code...

    // ==============================
    //  PARSE_REQUEST_LINE State
    // ==============================
    // ...existing PARSE_REQUEST_LINE code...

    // ==============================
    //  PARSE_HEADERS State
    // ==============================
    if (_state == PARSE_HEADERS)
    {
        // Process headers line by line until we find the empty line
        while (true)
        {
            // Look for next line ending
            size_t pos = _buffer.find("\r\n");
            if (pos == std::string::npos)
            {
                // No complete line yet, need more data
                if (_buffer.size() > 8192)
                {
                    _state = PARSE_ERROR;
                    _errorCode = 431;
                    return true;
                }
                return false;
            }

            // Extract the line (without \r\n)
            std::string line = _buffer.substr(0, pos);

            // Remove processed line from buffer (including \r\n)
            _buffer.erase(0, pos + 2);

            // Empty line marks end of headers
            if (line.empty())
            {
                // Headers are complete!

                // Check if HTTP/1.1 requires Host header
                if (_httpVersion == "HTTP/1.1")
                {
                    if (getHeader("Host").empty())
                    {
                        _state = PARSE_ERROR;
                        _errorCode = 400;
                        return true;
                    }
                }

                // ================================================
                // UPDATE MAX BODY SIZE BASED ON CONFIG
                // ================================================
                // Now that we have the path and headers, we can do
                // location matching and get the correct max body size
                if (serverConfig)
                {
                    updateMaxBodySizeFromConfig(*serverConfig);
                }

                // Determine if request has a body
                std::string contentLength = getHeader("Content-Length");
                std::string transferEncoding = getHeader("Transfer-Encoding");

                if (!contentLength.empty())
                {
                    _contentLength = std::atol(contentLength.c_str());

                    // Check against client_max_body_size from config
                    // Now uses the correctly updated _maxBodySize
                    if (_contentLength > _maxBodySize)
                    {
                        _state = PARSE_ERROR;
                        _errorCode = 413; // Payload Too Large
                        return true;
                    }

                    if (_contentLength > 0)
                    {
                        _state = PARSE_BODY;
                    }
                    else
                    {
                        _state = PARSE_COMPLETE;
                    }
                }
                else if (!transferEncoding.empty() &&
                         transferEncoding.find("chunked") != std::string::npos)
                {
                    _state = PARSE_CHUNKED_BODY;
                }
                else
                {
                    _state = PARSE_COMPLETE;
                    return true;
                }

                break;
            }

            // Parse this header line
            if (!parseHeader(line))
            {
                _state = PARSE_ERROR;
                return true;
            }
        }
    }

    // ...existing PARSE_BODY and PARSE_CHUNKED_BODY code...
}

// ...existing code...
````

## Step 4: Update the header file with the new signature

````cpp
// ...existing code...

#include "Config.hpp"  // Add this include

class Request
{
public:
    // ...existing code...

    // Parse incoming data - returns true when request is complete
    // serverConfig is optional, used to get correct max body size
    bool parse(const std::string& data, const ServerConfig* serverConfig = NULL);

    // ...existing code...
};

// ...existing code...
````

## Step 5: Update your connection handler to pass the server config

In your `Server.cpp` or wherever you handle client connections, update the parse call:

````cpp
// Example in your connection handler (Server.cpp or similar)
// ...existing code...

// When reading data from client
ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);
if (bytesRead > 0)
{
    std::string data(buffer, bytesRead);

    // Get the server config for this connection
    // (You likely already have this from when the connection was accepted)
    const ServerConfig* serverConfig = _config->getServerByHostPort(host, port);

    // Pass server config to parse for proper max body size handling
    if (client.request.parse(data, serverConfig))
    {
        // Request complete, route it
        Response response = _router.route(client.request, port);
        // ...send response...
    }
}

// ...existing code...
````

## Summary

This approach:

1. **Passes server config to `parse()`** - The config is passed as an optional parameter (NULL for backward compatibility)

2. **Updates max body size after headers** - Once we have the path from the request line and all headers are parsed, we call `updateMaxBodySizeFromConfig()` which does location matching

3. **Checks Content-Length against correct limit** - The `_maxBodySize` is now properly set before we validate the Content-Length header

4. **Works with chunked encoding too** - The `parseChunkedBody()` also uses `_maxBodySize` which is now correctly set

5. **Maintains backward compatibility** - If no server config is passed, it uses the default 1MB limit


















---



# Dynamic Body Size Limit implementation by AI Studio - first attempt
============================ =========================================


Here are the steps to implement the dynamic body size limit.

### Step 1: Update `Request.cpp`
We need to replace the hardcoded values with the class member `_maxBodySize`.

**In `Request::parse` (Inside `PARSE_HEADERS` block):**

Find the section handling `Content-Length` and replace the hardcoded `1048576`.

```cpp
// ... inside PARSE_HEADERS state ...

				if (!contentLength.empty())
				{
					// Content-Length body
					_contentLength = std::atol(contentLength.c_str());

					// REPLACEMENT: Use _maxBodySize instead of hardcoded 1MB
					if(_contentLength > _maxBodySize)
					{
						_state = PARSE_ERROR;
						_errorCode = 413; // Payload Too Large
						return true;
					}

					if (_contentLength > 0)
// ...
```

**In `Request::parse` (Inside `PARSE_CHUNKED_BODY` block):**

Find the safety check for chunked data and replace the hardcoded `104857600`.

```cpp
// ... inside PARSE_CHUNKED_BODY ...

		// ===================================
		//  Step 4: Enforce Body Size Limit
		// ===================================
		/*
			Even with chunked encoding, we must limit total body size
		*/
		// REPLACEMENT: Use _maxBodySize instead of hardcoded 100MB
		if (_body.size() + chunkSize > _maxBodySize)
		{
			_state = PARSE_ERROR;
			_errorCode = 413; // Payload Too Large
			return false;
		}
// ...
```

---

### Step 2: Implement the Helper in `Router.cpp`

You need a way to extract the correct size limit based on the request's **Host** (to find the Server) and **URI** (to find the Location).

Add this method to **`Router.hpp`**:

```cpp
// Router.hpp

public:
    // ... existing methods ...

    // Helper to determine max body size based on request headers
    size_t getClientMaxBodySize(const Request& request, int serverPort) const;
```

Add the implementation to **`Router.cpp`**:

```cpp
// Router.cpp

/*
    getClientMaxBodySize()

    Helper function to determine the effective client_max_body_size
    without performing a full route/response generation.

    Used by the Connection/Server class after headers are parsed
    to update the Request's limit before reading the body.
*/
size_t Router::getClientMaxBodySize(const Request& request, int serverPort) const
{
    if (!_config)
        return 1048576; // Default 1MB if no config

    // 1. Find the server block
    std::string hostHeader = request.getHeader("Host");
    const ServerConfig* server = findServer(serverPort, hostHeader);

    if (!server)
        return 1048576; // Default fallback

    // 2. Find the location block
    std::string requestPath = request.getPath();
    std::string method = request.getMethod();
    const LocationConfig* location = findLocation(*server, requestPath, method);

    // 3. Get the size
    return server->getMaxBodySize(location);
}
```

---

### Step 3: How to wire it together (Integration)

This is the tricky part. `Request::parse` might process headers and start processing the body in a single function call if the buffer contains both.

To support **Location-specific** limits that are higher than the Server default, you have to ensure the limit is updated *exactly* when the headers are finished but before the body size is checked.

Since `Request::parse` does the body size check immediately after headers (inside the same loop), you have two options. Ideally, you should perform this check in your main Server/Connection loop.

**Logic for your `Connection::handleRead()` or similar function:**

```cpp
// Assuming you have:
// Router router;
// Request request;
// int port;

// 1. Before parsing, set the SERVER level limit (fallback)
// This prevents attacks before we even know the URI
const ServerConfig* defaultServer = config.getServerByHostPort("0.0.0.0", port);
if (defaultServer) {
    request.setMaxBodySize(defaultServer->client_max_body_size);
}

// 2. Parse the incoming data
bool finished = request.parse(buffer);

// 3. CRITICAL: Check if we just finished parsing headers
// If the parser is now in BODY state, we should verify the Location limit.
if (request.getState() == Request::PARSE_BODY ||
    request.getState() == Request::PARSE_CHUNKED_BODY)
{
    // Retrieve the specific limit for this URI/Location
    size_t preciseLimit = router.getClientMaxBodySize(request, port);

    // Update the request
    request.setMaxBodySize(preciseLimit);

    // Re-verify Content-Length against new limit immediately
    // (Because Request::parse might have checked against the old limit and passed,
    //  or we want to catch it if the new limit is actually smaller).
    if (request.getContentLength() > preciseLimit) {
        // Handle 413 Payload Too Large manually or set error state
    }
}

// ... proceed with processing
```

**Note on the "Chicken and Egg" problem:**
If `Request::parse` receives the whole request (Header + Body) in one chunk, it will trigger the `413` error internally using the *old* limit before returning `true`.
*   **Safe approach:** Ensure your `ServerConfig` limit is high enough, or modify `Request.cpp` to **not** check `Content-Length` inside `PARSE_HEADERS`, but only check it at the start of `PARSE_BODY`.

To implement the **Safe Approach** inside `Request.cpp` (Recommended):

1.  In `PARSE_HEADERS`: Calculate `_contentLength`, but **remove** the `if(_contentLength > _maxBodySize)` check there.
2.  In `PARSE_BODY`: Add the check at the very top.

```cpp
// Request.cpp modification

// ... inside PARSE_BODY ...
if (_state == PARSE_BODY)
{
    // Check limit HERE, at the start of body processing
    // This allows the caller to update _maxBodySize after headers are parsed
    // but before body is consumed.
    if (_contentLength > _maxBodySize)
    {
        _state = PARSE_ERROR;
        _errorCode = 413;
        return true;
    }

    // Calculate how many bytes we still need...
    // ... rest of logic
```



















