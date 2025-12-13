#!/usr/bin/env python3
"""
Slow CGI Script for Timeout Testing

This script intentionally sleeps to test the CGI timeout handling.
The webserv should kill this script and return 504 Gateway Timeout.

Usage:
    Set the timeout in the URL: /cgi-bin/slow.py?sleep=5
    Default sleep is 60 seconds (should trigger timeout).
"""

import os
import sys
import time

def main():
    # Get sleep duration from query string
    query = os.environ.get('QUERY_STRING', '')
    sleep_time = 60  # Default: longer than typical timeout
    
    if query:
        for param in query.split('&'):
            if param.startswith('sleep='):
                try:
                    sleep_time = int(param.split('=')[1])
                except ValueError:
                    pass
    
    # Sleep (this should be interrupted by timeout)
    time.sleep(sleep_time)
    
    # If we get here, timeout didn't work
    print("Content-Type: text/plain\r")
    print("\r")
    print(f"Slept for {sleep_time} seconds")
    print("This should not appear if timeout is working!")


if __name__ == '__main__':
    main()
