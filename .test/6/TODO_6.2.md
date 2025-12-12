
### Step 6.2: Non-blocking Write
**Goal**: Write responses to clients non-blockingly

**Tasks**:
- Monitor client sockets for POLLOUT events
- Use `send()` to write response data
- Handle partial writes (EAGAIN/EWOULDBLOCK)
- Buffer unsent data for next POLLOUT event
- Track bytes sent vs total response size
- Close connection when response is complete (if Connection: close)
- Support keep-alive connections

**Testing**:
```bash
# Test large file serving
dd if=/dev/zero of=www/large.bin bs=1M count=50
curl http://localhost:8080/large.bin > /dev/null

# Test keep-alive
curl -v http://localhost:8080/ http://localhost:8080/index.html
```


**we need the network part (Phase 2 and Phase 3) to implement this**:
