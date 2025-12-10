/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:57:30 by anemet            #+#    #+#             */
/*   Updated: 2025/12/10 21:39:33 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "Request.hpp"
#include "Response.hpp"
#include "Config.hpp"
#include <string>

// Forward declaration to avoid circular dependency
class Config;
struct ServerConfig;
struct LocationConfig;

class Router
{
	public:
		Router();
		explicit Router(const Config& config);
		virtual ~Router();
		Router(const Router& other);
		Router& operator=(const Router& other);

		// Main routing method - processes request and returns response
		// Virtual to allow MockRouter to override
		virtual Response route(const Request& request, int serverPort);

		// Set/update configuration
		void setConfig(const Config& config);

	protected:
		// Helper methods (can be overridden by MockRouter if needed)
		virtual Response handleGet(const Request& request, const LocationConfig& location);
		virtual Response handlePost(const Request& request, const LocationConfig& location);
		virtual Response handleDelete(const Request& request, const LocationConfig& location);

		// Find matching server and location
		const ServerConfig* findServer(int port) const;
		const LocationConfig* findLocation(const ServerConfig& server, const std::string& path) const;

		// Static file operations
		Response serveFile(const std::string& filepath);
		Response serveDirectory(const std::string& dirpath, const LocationConfig& location);
		Response handleCgi(const Request& request, const std::string& scriptPath,
						const LocationConfig& location);

		// Error responses
		Response errorResponse(int code, const ServerConfig* server = NULL);

		// Utility
		std::string resolvePath(const std::string& uri, const LocationConfig& location);
		bool isMethodAllowed(const std::string& method, const LocationConfig& location);
		bool isCgiRequest(const std::string& path, const LocationConfig& location);

	private:
		const Config* _config;
};

#endif
