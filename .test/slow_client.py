#!/usr/bin/env python3

import socket
import time

HOST = '127.0.0.1'
PORT = 8080

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST, PORT))

# Send a request for the large file
request = "GET /large.bin HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
s.sendall(request.encode())

print("Request sent. Reading slowly...")

total_bytes = 0
try:
    while True:
        # Read a tiny amount of data
        data = s.recv(1024)
        if not data:
            break
        total_bytes += len(data)
        
        # PRINT STATUS
        print(f"\rReceived {total_bytes} bytes...", end="")
        
        # SLEEP to force server buffer to fill up
        # This forces the server to return to poll() loop
        time.sleep(0.05) 
except KeyboardInterrupt:
    print("\nTest stopped.")

s.close()
