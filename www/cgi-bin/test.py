#!/usr/bin/env python3
"""
CGI Test Script for webserv

This script tests various CGI capabilities:
1. Basic output (headers + body)
2. Environment variable access
3. Query string parsing
4. POST body reading

CGI scripts communicate with the web server via:
- Environment variables (REQUEST_METHOD, QUERY_STRING, etc.)
- stdin (for POST request body)
- stdout (for response: headers + blank line + body)

Response format:
    Content-Type: text/html\r\n
    \r\n
    <html>...</html>
"""

import os
import sys


def main():
    """Main CGI handler."""
    
    # ==========================================
    #  Step 1: Output HTTP headers
    # ==========================================
    # CGI requires at minimum a Content-Type header
    # Headers are separated from body by a blank line
    print("Content-Type: text/html\r")
    print("\r")  # Empty line marks end of headers
    
    # ==========================================
    #  Step 2: Build HTML response
    # ==========================================
    print("<!DOCTYPE html>")
    print("<html>")
    print("<head><title>CGI Test</title></head>")
    print("<body>")
    print("<h1>CGI Test Script - Python</h1>")
    print("<p>If you see this, CGI execution is working!</p>")
    
    # ==========================================
    #  Step 3: Display environment variables
    # ==========================================
    print("<h2>Environment Variables</h2>")
    print("<table border='1'>")
    print("<tr><th>Variable</th><th>Value</th></tr>")
    
    # Key CGI environment variables
    cgi_vars = [
        'REQUEST_METHOD',
        'QUERY_STRING', 
        'CONTENT_TYPE',
        'CONTENT_LENGTH',
        'SCRIPT_NAME',
        'SCRIPT_FILENAME',
        'PATH_INFO',
        'SERVER_PROTOCOL',
        'SERVER_NAME',
        'SERVER_PORT',
        'GATEWAY_INTERFACE',
        'HTTP_HOST',
        'HTTP_USER_AGENT',
    ]
    
    for var in cgi_vars:
        value = os.environ.get(var, '<not set>')
        print(f"<tr><td>{var}</td><td>{value}</td></tr>")
    
    print("</table>")
    
    # ==========================================
    #  Step 4: Parse and display query string
    # ==========================================
    print("<h2>Query String Parameters</h2>")
    query_string = os.environ.get('QUERY_STRING', '')
    
    if query_string:
        print("<table border='1'>")
        print("<tr><th>Parameter</th><th>Value</th></tr>")
        
        # Simple query string parsing (key=value&key2=value2)
        for pair in query_string.split('&'):
            if '=' in pair:
                key, value = pair.split('=', 1)
                print(f"<tr><td>{key}</td><td>{value}</td></tr>")
            else:
                print(f"<tr><td>{pair}</td><td><em>no value</em></td></tr>")
        
        print("</table>")
    else:
        print("<p><em>No query string provided</em></p>")
        print("<p>Try: <code>?name=World&count=42</code></p>")
    
    # ==========================================
    #  Step 5: Read and display POST body
    # ==========================================
    print("<h2>POST Body</h2>")
    
    method = os.environ.get('REQUEST_METHOD', 'GET')
    content_length = os.environ.get('CONTENT_LENGTH', '')
    
    if method == 'POST' and content_length:
        try:
            length = int(content_length)
            body = sys.stdin.read(length)
            print(f"<p>Received {len(body)} bytes:</p>")
            print(f"<pre>{body}</pre>")
        except Exception as e:
            print(f"<p>Error reading body: {e}</p>")
    else:
        print("<p><em>No POST body (GET request or no Content-Length)</em></p>")
    
    print("</body>")
    print("</html>")


if __name__ == '__main__':
    main()
