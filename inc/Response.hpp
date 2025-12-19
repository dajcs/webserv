/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:57:21 by anemet            #+#    #+#             */
/*   Updated: 2025/12/19 08:44:25 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>
#include <sstream>
#include <ctime>
#include <cstring>
#include <fstream>

/*
	=================================
		HTTP RESPONSE BASICS
	=================================

	An HTTP response is what the server sends back to the client (browser).
	It consists of three parts:

	1. STATUS LINE: HTTP version + status code + reason phrase
	   Example: "HTTP/1.1 200 OK"

	2. HEADERS: Key-value pairs with metadata about the response
	   Example: "Content-Type: text/html"

	3. BODY: The actual content (HTML, JSON, image data, etc.)
	   Example: "<html><body>Hello</body></html>"

	These parts are separated by CRLF (\r\n):

	HTTP/1.1 200 OK\r\n
	Content-Type: text/html\r\n
	Content-Length: 42\r\n
	Date: Wed, 11 Dec 2025 12:00:00 GMT\r\n
	Server: webserv/1.0\r\n
	Connection: keep-alive\r\n
	\r\n                              <- Empty line marks end of headers
	<html><body>Hello World</body></html>

	Why is this format important?
	- HTTP is a text-based protocol with strict formatting rules
	- Browsers expect exact format; any deviation causes parsing failures
	- \r\n (CRLF) is MANDATORY, not just \n
*/

class Response
{

	private:
		int _statusCode;								// HTTP status code (200, 404, 500, etc.)
		std::string _reasonPhrase;						// Human-readable status ("OK", "Not Found")
		std::map<std::string, std::string> _headers;	// HTTP headers (key -> value)
		std::string _body;								// Response body content
		mutable std::string _builtResponse;				// Cached complete HTTP response string
		mutable bool _dirty;							// Flag: does response need rebuilding?
		bool _keepAlive;								// Should connection stay open?

		// Lazy building pattern - only rebuild when needed
		void buildIfNeeded() const;


	public:
		// ===== Constructors & Destructor =====
		Response();
		~Response();
		Response(const Response& other);
		Response& operator=(const Response& other);

		// ===== Setters for response components =====
		void setStatus(int code);
		void setStatus(int code, const std::string& reason);
		void setHeader(const std::string& name, const std::string& value);
		void setBody(const std::string& body);
		void setBody(const char* data, size_t length);

		// ===== Convenience methods for common headers =====
		void setContentType(const std::string& type);
		void setContentLength(size_t length);
		void setConnection(bool keepAlive);

		// === Standard headers (Step 6.1 requirement) ===
		void addDateHeader();		// Add current date/time
		void addServerHeader();		// Add "Server: webser/1.0"
		void addStandardHeaders();	// Add all standard headers at once

		// ===== Building the response =====
		std::string build() const;			// Build the complete HTTP response string
		const std::string& getData() const;	// Get raw response data for sending
		size_t getSize() const;

		// ===== Getters =====
		int getStatusCode() const;
		const std::string& getBody() const;
		const std::string& getReasonPhraseValue() const;
		std::string getHeader(const std::string& name) const;
		bool hasHeader(const std::string& name) const;
		bool shouldKeepAlive() const;

		// ===== Static factory methods for common responses =====
		static Response ok(const std::string& body, const std::string& contentType = "text/html");
		static Response error(int code);
		static Response error(int code, const std::string& customBody);
		static Response redirect(int code, const std::string& location);
		static Response noContent();  // 204 No Content (for DELETE success)

		// ===== Static helpers =====
		static std::string getReasonPhrase(int code);
		static std::string getDefaultErrorPage(int code);
		static std::string formatHttpDate(time_t timestamp);

		// MIME type helpers
		static std::string getMimeType(const std::string& extension);
		static std::string getMimeTypeForFile(const std::string& filepath);
		static bool isTextFile(const std::string& filepath);

};

#endif
