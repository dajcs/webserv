# Signal Flow: Chunked File Upload

Here's how the signal flow differs when using chunked transfer encoding with `curl -v -H "Transfer-Encoding: chunked" -F "file=@Makefile" http://localhost:8080/uploads`:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    KEY DIFFERENCE: NO CONTENT-LENGTH                        │
└─────────────────────────────────────────────────────────────────────────────┘

REGULAR POST (Content-Length):          CHUNKED POST (Transfer-Encoding):
────────────────────────────────        ────────────────────────────────────
POST /uploads HTTP/1.1                  POST /uploads HTTP/1.1
Host: localhost:8080                    Host: localhost:8080
Content-Length: 1847        ← SIZE      Transfer-Encoding: chunked  ← NO SIZE
Content-Type: multipart/...             Content-Type: multipart/...

<entire body in one piece>              <body sent in chunks with hex sizes>


┌─────────────────────────────────────────────────────────────────────────────┐
│                    CHUNKED ENCODING FORMAT                                  │
└─────────────────────────────────────────────────────────────────────────────┘

curl sends data in this format:
═══════════════════════════════════════════════════════════════════════════════

POST /uploads HTTP/1.1\r\n
Host: localhost:8080\r\n
User-Agent: curl/7.68.0\r\n
Accept: */*\r\n
Transfer-Encoding: chunked\r\n                    ← KEY HEADER
Content-Type: multipart/form-data; boundary=------------------------abc123xyz\r\n
\r\n
                                                  ← End of headers, body starts
3ff\r\n                                           ← Chunk 1: hex size (1023 bytes)
--------------------------abc123xyz\r\n
Content-Disposition: form-data; name="file"; filename="Makefile"\r\n
Content-Type: application/octet-stream\r\n
\r\n
# **************************************************************************** #\r\n
#    Makefile                                           :+:      :+:    :+:   #\r\n
... (first 1023 bytes of content) ...
\r\n                                              ← Chunk terminator
2a0\r\n                                           ← Chunk 2: hex size (672 bytes)
... (next portion of Makefile) ...
\r\n                                              ← Chunk terminator
b5\r\n                                            ← Chunk 3: hex size (181 bytes)
# **************************************************************************** #\r\n
--------------------------abc123xyz--\r\n
\r\n                                              ← Chunk terminator
0\r\n                                             ← Final chunk: size 0 = END
\r\n                                              ← End of chunked body

═══════════════════════════════════════════════════════════════════════════════
```

## Detailed Signal Flow Differences

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    REQUEST PARSING - HEADER PHASE                           │
└─────────────────────────────────────────────────────────────────────────────┘

_request->parse(_readBuffer)
    │
    │  ┌─────────────────────────────────────────────────────────────────┐
    │  │                  PARSE_HEADERS                                  │
    │  └─────────────────────────────────────────────────────────────────┘
    │
    ├──► Parse headers:
    │        _headers["host"] = "localhost:8080"
    │        _headers["user-agent"] = "curl/7.68.0"
    │        _headers["accept"] = "*/*"
    │        _headers["transfer-encoding"] = "chunked"      ← KEY DIFFERENCE!
    │        _headers["content-type"] = "multipart/form-data; boundary=..."
    │
    │        ┌─────────────────────────────────────────────────────────────┐
    │        │  NO Content-Length header!                                  │
    │        │  Instead: Transfer-Encoding: chunked                        │
    │        └─────────────────────────────────────────────────────────────┘
    │
    ├──► Empty line found → end of headers
    │
    ├──► Check for Transfer-Encoding header:
    │        getHeader("transfer-encoding") → "chunked"
    │        Utils::toLower("chunked") → "chunked"
    │        _isChunked = true                              ← SET CHUNKED MODE
    │
    ├──► Check for Content-Length:
    │        getHeader("content-length") → "" (empty)
    │        _contentLength = 0                             ← NO PREDETERMINED SIZE
    │
    └──► _state = PARSE_BODY_CHUNKED                        ← DIFFERENT STATE!


┌─────────────────────────────────────────────────────────────────────────────┐
│               PARSE_BODY_CHUNKED - State Machine                            │
└─────────────────────────────────────────────────────────────────────────────┘

The chunked body parser is a state machine with these states:

    CHUNK_SIZE    →  Reading chunk size (hex number)
    CHUNK_DATA    →  Reading chunk data (N bytes)
    CHUNK_END     →  Reading \r\n after chunk data
    COMPLETE      →  Received final 0-size chunk

_request->parse() continues (PARSE_BODY_CHUNKED):
    │
    │  ════════════════════════════════════════════════════════════════════
    │                        FIRST CHUNK
    │  ════════════════════════════════════════════════════════════════════
    │
    ├──► _chunkState = CHUNK_SIZE (initial)
    │
    ├──► Read until \r\n: "3ff\r\n"
    │        │
    │        ├──► chunkSizeLine = "3ff"
    │        │
    │        ├──► Parse hex size:
    │        │        std::strtol("3ff", NULL, 16) → 1023
    │        │
    │        ├──► _currentChunkSize = 1023
    │        │
    │        ├──► _chunkBytesRead = 0
    │        │
    │        └──► _chunkState = CHUNK_DATA
    │
    ├──► Read 1023 bytes of data:
    │        │
    │        ├──► available = buffer.size() - currentPos
    │        │
    │        ├──► toRead = min(1023 - 0, available)
    │        │
    │        ├──► _body.append(buffer.substr(pos, toRead))
    │        │
    │        ├──► _chunkBytesRead += toRead
    │        │
    │        └──► If _chunkBytesRead == 1023:
    │                 _chunkState = CHUNK_END
    │
    ├──► Read chunk terminator \r\n:
    │        │
    │        ├──► Verify next 2 bytes are \r\n
    │        │
    │        └──► _chunkState = CHUNK_SIZE (ready for next chunk)
    │
    │  ════════════════════════════════════════════════════════════════════
    │                        SECOND CHUNK
    │  ════════════════════════════════════════════════════════════════════
    │
    ├──► Read "2a0\r\n"
    │        _currentChunkSize = 0x2a0 = 672
    │        _chunkState = CHUNK_DATA
    │
    ├──► Read 672 bytes
    │        _body.append(data)
    │        _chunkState = CHUNK_END
    │
    ├──► Read \r\n
    │        _chunkState = CHUNK_SIZE
    │
    │  ════════════════════════════════════════════════════════════════════
    │                        THIRD CHUNK
    │  ════════════════════════════════════════════════════════════════════
    │
    ├──► Read "b5\r\n"
    │        _currentChunkSize = 0xb5 = 181
    │        _chunkState = CHUNK_DATA
    │
    ├──► Read 181 bytes
    │        _body.append(data)
    │        _chunkState = CHUNK_END
    │
    ├──► Read \r\n
    │        _chunkState = CHUNK_SIZE
    │
    │  ════════════════════════════════════════════════════════════════════
    │                        FINAL CHUNK (Size 0)
    │  ════════════════════════════════════════════════════════════════════
    │
    ├──► Read "0\r\n"
    │        │
    │        ├──► _currentChunkSize = 0       ← ZERO SIZE = END OF BODY
    │        │
    │        └──► _chunkState = CHUNK_TRAILERS (or COMPLETE if no trailers)
    │
    ├──► Read final \r\n
    │
    ├──► _state = PARSE_COMPLETE              ← CHUNKED BODY FULLY RECEIVED!
    │
    └──► Returns: true


┌─────────────────────────────────────────────────────────────────────────────┐
│               MULTIPLE recv() CALLS FOR CHUNKED DATA                        │
└─────────────────────────────────────────────────────────────────────────────┘

With chunked encoding, data typically arrives in multiple pieces:

epoll_wait(4, events, 64, 1000) → 1     ─┐
events[0] = {EPOLLIN, fd=5}              │
    │                                    │
    ▼                                    │
recv(5, buffer, 8192, 0)                 │  First network packet
    │                                    │  Headers + partial chunk
    └──► Returns: 600 bytes              │
         Contains: headers + "3ff\r\n"   │
                   + partial data       ─┘
    │
    ▼
parse() → _state = PARSE_BODY_CHUNKED
          Need more data (chunk incomplete)
          Returns: false
    │
    ▼
epoll_wait(4, events, 64, 1000) → 1     ─┐
events[0] = {EPOLLIN, fd=5}              │
    │                                    │
    ▼                                    │
recv(5, buffer, 8192, 0)                 │  Second network packet
    │                                    │  Rest of first chunk +
    └──► Returns: 800 bytes              │  second chunk
         Contains: rest of chunk 1       │
                   + "2a0\r\n" + data   ─┘
    │
    ▼
parse() → Completes chunk 1
          Starts chunk 2
          Still needs more data
          Returns: false
    │
    ▼
epoll_wait(4, events, 64, 1000) → 1     ─┐
events[0] = {EPOLLIN, fd=5}              │
    │                                    │
    ▼                                    │
recv(5, buffer, 8192, 0)                 │  Third network packet
    │                                    │  Final chunks
    └──► Returns: 500 bytes              │
         Contains: rest of chunk 2       │
                   + chunk 3 + "0\r\n"  ─┘
    │
    ▼
parse() → Completes chunk 2
          Completes chunk 3
          Sees "0\r\n" (final chunk)
          _state = PARSE_COMPLETE
          Returns: true
```

## Comparison: Content-Length vs Chunked Parsing

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    CONTENT-LENGTH BODY PARSING                              │
└─────────────────────────────────────────────────────────────────────────────┘

_state = PARSE_BODY
    │
    └──► Simple loop:
             while (_bodyBytesRead < _contentLength):
                 available = buffer.size()
                 toRead = min(remaining, available)
                 _body.append(buffer, toRead)
                 _bodyBytesRead += toRead

             if (_bodyBytesRead == _contentLength):
                 _state = PARSE_COMPLETE


┌─────────────────────────────────────────────────────────────────────────────┐
│                    CHUNKED BODY PARSING                                     │
└─────────────────────────────────────────────────────────────────────────────┘

_state = PARSE_BODY_CHUNKED
    │
    └──► State machine loop:
             while (true):
                 switch (_chunkState):

                     case CHUNK_SIZE:
                         line = readLine(buffer)     // Read until \r\n
                         if (line not complete):
                             return false            // Need more data
                         _currentChunkSize = parseHex(line)
                         if (_currentChunkSize == 0):
                             _state = PARSE_COMPLETE
                             return true             // Done!
                         _chunkBytesRead = 0
                         _chunkState = CHUNK_DATA
                         break

                     case CHUNK_DATA:
                         available = buffer.size()
                         needed = _currentChunkSize - _chunkBytesRead
                         toRead = min(needed, available)
                         _body.append(buffer, toRead)
                         _chunkBytesRead += toRead
                         if (_chunkBytesRead == _currentChunkSize):
                             _chunkState = CHUNK_END
                         else:
                             return false            // Need more data
                         break

                     case CHUNK_END:
                         if (buffer.size() < 2):
                             return false            // Need \r\n
                         verify buffer starts with \r\n
                         consume \r\n
                         _chunkState = CHUNK_SIZE    // Next chunk
                         break
```

## Summary: Key Differences

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    SIDE-BY-SIDE COMPARISON                                  │
└─────────────────────────────────────────────────────────────────────────────┘

┌──────────────────────┬────────────────────────┬────────────────────────────┐
│       Aspect         │    Content-Length      │    Chunked Encoding        │
├──────────────────────┼────────────────────────┼────────────────────────────┤
│ Header               │ Content-Length: 1847   │ Transfer-Encoding: chunked │
├──────────────────────┼────────────────────────┼────────────────────────────┤
│ Size known upfront?  │ YES                    │ NO                         │
├──────────────────────┼────────────────────────┼────────────────────────────┤
│ Parse state          │ PARSE_BODY             │ PARSE_BODY_CHUNKED         │
├──────────────────────┼────────────────────────┼────────────────────────────┤
│ Body format          │ Raw bytes              │ Size + data + \r\n chunks  │
├──────────────────────┼────────────────────────┼────────────────────────────┤
│ Completion check     │ bytes == contentLength │ chunk size == 0            │
├──────────────────────┼────────────────────────┼────────────────────────────┤
│ Max body check       │ Compare contentLength  │ Check as chunks arrive     │
│                      │ to max upfront         │ (running total)            │
├──────────────────────┼────────────────────────┼────────────────────────────┤
│ Memory overhead      │ Minimal                │ State machine + chunk vars │
├──────────────────────┼────────────────────────┼────────────────────────────┤
│ recv() calls         │ Typically fewer        │ May be more fragmented     │
└──────────────────────┴────────────────────────┴────────────────────────────┘


┌─────────────────────────────────────────────────────────────────────────────┐
│                    WIRE FORMAT COMPARISON                                   │
└─────────────────────────────────────────────────────────────────────────────┘

Content-Length format:
═══════════════════════════════════════════
POST /uploads HTTP/1.1
Content-Length: 1847
Content-Type: multipart/form-data; ...

--------------------------abc123xyz
Content-Disposition: form-data; ...

[1847 bytes of data exactly]
═══════════════════════════════════════════

Chunked format:
═══════════════════════════════════════════
POST /uploads HTTP/1.1
Transfer-Encoding: chunked
Content-Type: multipart/form-data; ...

3ff                          ← hex(1023)
[1023 bytes of data]
                             ← \r\n
2a0                          ← hex(672)
[672 bytes of data]
                             ← \r\n
b5                           ← hex(181)
[181 bytes of data]
                             ← \r\n
0                            ← final chunk
                             ← \r\n (end)
═══════════════════════════════════════════
```

## System Calls Comparison

| Phase | Content-Length Upload | Chunked Upload |
|-------|----------------------|----------------|
| **Read** | `recv()` × ~2 calls | `recv()` × ~3-5 calls (more fragmented) |
| **Parse overhead** | Simple byte count | Hex parsing per chunk |
| **Validation** | Check size once at start | Check running total per chunk |
| **Completion** | `bytesRead == contentLength` | `chunkSize == 0` |
| **Rest of flow** | Identical | Identical |

The key insight is that **after parsing completes**, the rest of the flow is **identical** — the `_body` member contains the reassembled content (with chunk framing removed), and the multipart parsing, file saving, and response generation work exactly the same way.
