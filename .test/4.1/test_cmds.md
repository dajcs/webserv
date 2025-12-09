# Make sure you're in the webserv directory
cd /home/anemet/webserv

# Compile the test (standalone, no network dependencies)
c++ -std=c++98 -Wall -Wextra -Werror -I inc src/Request.cpp test_request_step4_1.cpp -o test_step4_1

# Run the tests
./test_step4_1


What this test covers:

✅ Valid requests - All HTTP methods (GET, POST, DELETE, HEAD, PUT)
✅ Query strings - Parsing URI paths vs query parameters
✅ HTTP versions - Both 1.0 and 1.1
✅ Error handling - 400, 414, 501, 505 responses
✅ Incremental parsing - Simulates non-blocking network I/O
✅ Buffer overflow protection - Security against malicious requests
✅ Keep-alive simulation - Request reuse via reset()
Why this works without network code:

The Request::parse() method accepts a std::string parameter
In production, this string comes from recv() on a socket
For testing, we just pass strings directly
This tests the parsing logic independently of network I/O
Next steps:

After this test passes, you'll be ready to:

Implement Step 4.2 (header parsing)
Implement Step 4.3 (body parsing)
Eventually integrate with the network layer (Steps 2-3)
The beauty of this design is that Student 2 can develop and test all HTTP parsing without waiting for Student 1's network code!




