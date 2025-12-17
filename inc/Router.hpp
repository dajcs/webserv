/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:57:30 by anemet            #+#    #+#             */
/*   Updated: 2025/12/17 10:14:39 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <string>
#include <vector>
#include <map>
#include <iomanip>
#include <sys/types.h>  // For off_t and time_t types

#include "Request.hpp"
#include "Response.hpp"

// Forward declarations
class Config;
struct ServerConfig;
struct LocationConfig;

/*
	=================================
		ROUTER CLASS
	=================================

	The Router is responsible for:
	1. Matching incoming requests to server/location configurations
	2. Determining how to handle each request (static file, CGI, redirect, etc.)
	3. Generating appropriate HTTP responses

	It acts as the central dispatcher in the request handling pipeline:

		Request → Router → Handler (GET/POST/DELETE/CGI) → Response

	The Router doesn't handle network I/O - it only transforms Requests
	into Responses based on the configuration.
*/

class Router
{
	public:
		// ================================
		//  Constructors & Destructor
		// ================================
		Router();
		explicit Router(const Config& config);
		virtual ~Router();
		Router(const Router& other);
		Router& operator=(const Router& other);


		// Set/update configuration
		void setConfig(const Config& config);

		/*
			route() - Main entry point for request handling

			Takes a parsed HTTP request and determines how to respond.
			Uses the configuration to find the right server/location and
			applies the appropriate handling (file serving, CGI, redirect, etc.)

			Parameters:
				request:    Parsed HTTP request
				serverPort: The port this request came in on

			Returns:
				Complete HTTP Response ready to send
		*/
		Response route(const Request& request, int serverPort);

private:
	// ================================
	//  Server/Location Finding
	// ================================
	const ServerConfig* findServer(int port, const std::string& hostname = "") const;
	const LocationConfig* findLocation(const ServerConfig& server,
										const std::string& path) const;

	// ================================
	//  Path Resolution
	// ================================
	std::string resolvePath(const std::string& uri, const LocationConfig& location);
	bool isMethodAllowed(const std::string& method, const LocationConfig& location);
	bool isCgiRequest(const std::string& path, const LocationConfig& location);

	// ================================
	//  Request Handlers
	// ================================
	Response handleGet(const Request& request, const LocationConfig& location);
	Response handlePost(const Request& request, const LocationConfig& location);
	Response handleDelete(const Request& request, const LocationConfig& location);
	Response handleCgi(const Request& request, const std::string& scriptPath,
						const LocationConfig& location);

	// ================================
	//  File/Directory Serving
	// ================================
	Response serveFile(const std::string& filepath);
	Response serveDirectory(const std::string& dirpath, const LocationConfig& location);

	// ================================
	//  Directory Listing Helpers (Step 9.2)
	// ================================

	/*
		escapeHtml() - Escape HTML special characters

		Prevents XSS attacks by converting characters like < > & to
		their HTML entity equivalents.
	*/
	std::string escapeHtml(const std::string& str);

	/*
		formatFileSize() - Convert bytes to human-readable size

		Examples: 1024 -> "1.0 KB", 1048576 -> "1.0 MB"
	*/
	std::string formatFileSize(off_t size);

	/*
		formatTime() - Format Unix timestamp for display

		Returns format: "YYYY-MM-DD HH:MM"
	*/
	std::string formatTime(time_t timestamp);

	// ================================
	//  Error Responses
	// ================================
	Response errorResponse(int code, const ServerConfig* server = NULL);

	// ================================
	//  Member Variables
	// ================================
	const Config* _config;
};

#endif
