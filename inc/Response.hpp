/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:57:21 by anemet            #+#    #+#             */
/*   Updated: 2025/12/11 15:59:35 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>
#include <sstream>

class Response
{

	private:
		int _statusCode;
		std::string _reasonPhrase;
		std::map<std::string, std::string> _headers;
		std::string _body;
		mutable std::string _builtResponse;
		mutable bool _dirty;
		bool _keepAlive;

		void buildIfNeeded() const;


	public:
		Response();
		~Response();
		Response(const Response& other);
		Response& operator=(const Response& other);

		// Set response components
		void setStatus(int code);
		void setStatus(int code, const std::string& reason);
		void setHeader(const std::string& name, const std::string& value);
		void setBody(const std::string& body);
		void setBody(const char* data, size_t length);

		// Convenience methods
		void setContentType(const std::string& type);
		void setContentLength(size_t length);
		void setConnection(bool keepAlive);

		// Build the complete HTTP response string
		std::string build() const;

		// Get raw response data for sending
		const std::string& getData() const;
		size_t getSize() const;

		// Getters
		int getStatusCode() const;
		const std::string& getBody() const;
		bool shouldKeepAlive() const;

		// Static factory methods for common responses
		static Response ok(const std::string& body, const std::string& contentType = "text/html");
		// static Response error(int code);
		// static Response redirect(int code, const std::string& location);

		// // Static helpers
		// static std::string getReasonPhrase(int code);
		// static std::string getMimeType(std::string& extension);


		/*
			getMimeType() - Get MIME type from file extension

			MIME types tell the browser how to handle content.
			Without correct MIME type, browser might not render correctly.
		*/
		static std::string getMimeType(const std::string& extension)
		{
			// Common MIME types
			if (extension == ".html" || extension == ".htm")
				return "text/html";
			if (extension == ".css")
				return "text/css";
			if (extension == ".js")
				return "application/javascript";
			if (extension == ".json")
				return "application/json";
			if (extension == ".xml")
				return "application/xml";
			if (extension == ".txt")
				return "text/plain";
			if (extension == ".jpg" || extension == ".jpeg")
				return "image/jpeg";
			if (extension == ".png")
				return "image/png";
			if (extension == ".gif")
				return "image/gif";
			if (extension == ".ico")
				return "image/x-icon";
			if (extension == ".svg")
				return "image/svg+xml";
			if (extension == ".pdf")
				return "application/pdf";
			if (extension == ".zip")
				return "application/zip";

			// Default for unknown types
			return "application/octet-stream";
		}

		/*
			error() - Create a default error response
		*/
		static Response error(int code)
		{
			Response response;
			response.setStatus(code);
			response.setContentType("text/html");

			std::ostringstream body;
			body << "<!DOCTYPE html>\n";
			body << "<html>\n<head>\n";
			body << "<title>" << code << " " << getReasonPhrase(code) << "</title>\n";
			body << "</head>\n<body>\n";
			body << "<h1>" << code << " " << getReasonPhrase(code) << "</h1>\n";
			body << "<hr>\n<p>webserv</p>\n";
			body << "</body>\n</html>\n";

			response.setBody(body.str());
			return response;
		}

		/*
			redirect() - Create a redirect response
		*/
		static Response redirect(int code, const std::string& location)
		{
			Response response;
			response.setStatus(code);
			response.setHeader("Location", location);
			response.setContentType("text/html");

			std::ostringstream body;
			body << "<!DOCTYPE html>\n";
			body << "<html>\n<head>\n";
			body << "<title>Redirect</title>\n";
			body << "</head>\n<body>\n";
			body << "<h1>Redirecting...</h1>\n";
			body << "<p>If you are not redirected, <a href=\"" << location << "\">click here</a>.</p>\n";
			body << "</body>\n</html>\n";

			response.setBody(body.str());
			return response;
		}

		/*
			getReasonPhrase() - Get standard reason phrase for status code
		*/
		static std::string getReasonPhrase(int code)
		{
			switch (code)
			{
				case 200: return "OK";
				case 201: return "Created";
				case 204: return "No Content";
				case 301: return "Moved Permanently";
				case 302: return "Found";
				case 304: return "Not Modified";
				case 400: return "Bad Request";
				case 403: return "Forbidden";
				case 404: return "Not Found";
				case 405: return "Method Not Allowed";
				case 409: return "Conflict";
				case 413: return "Payload Too Large";
				case 500: return "Internal Server Error";
				case 501: return "Not Implemented";
				case 502: return "Bad Gateway";
				case 504: return "Gateway Timeout";
				case 505: return "HTTP Version Not Supported";
				default:  return "Unknown";
			}
		}

};

#endif
