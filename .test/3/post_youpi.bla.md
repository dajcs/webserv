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
