#!/usr/bin/env python3
"""
CGI Script demonstrating Status header

CGI scripts can set HTTP status codes using the Status header:
    Status: 404 Not Found

This allows CGI scripts to return errors, redirects, etc.

Usage:
    /cgi-bin/status.py?code=404
    /cgi-bin/status.py?code=302&redirect=/new-page
"""

import os

def main():
    query = os.environ.get('QUERY_STRING', '')
    status_code = 200
    redirect_url = None
    
    # Parse query string
    for param in query.split('&'):
        if param.startswith('code='):
            try:
                status_code = int(param.split('=')[1])
            except ValueError:
                pass
        elif param.startswith('redirect='):
            redirect_url = param.split('=')[1]
    
    # Status phrases
    phrases = {
        200: "OK",
        201: "Created",
        204: "No Content",
        301: "Moved Permanently",
        302: "Found",
        400: "Bad Request",
        403: "Forbidden",
        404: "Not Found",
        500: "Internal Server Error",
    }
    
    phrase = phrases.get(status_code, "Unknown")
    
    # Output headers
    print(f"Status: {status_code} {phrase}\r")
    
    if redirect_url:
        print(f"Location: {redirect_url}\r")
    
    print("Content-Type: text/html\r")
    print("\r")
    
    # Output body
    print("<!DOCTYPE html>")
    print("<html>")
    print(f"<head><title>{status_code} {phrase}</title></head>")
    print("<body>")
    print(f"<h1>{status_code} {phrase}</h1>")
    print(f"<p>CGI script returned status code {status_code}</p>")
    
    if redirect_url:
        print(f"<p>Redirect to: <a href='{redirect_url}'>{redirect_url}</a></p>")
    
    print("</body>")
    print("</html>")


if __name__ == '__main__':
    main()
