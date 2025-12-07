/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:55:03 by anemet            #+#    #+#             */
/*   Updated: 2025/12/07 22:03:50 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Config.hpp"

//==================================
//	LocationConfig Implementation
//==================================

/*
	LocationConfig default constructor
	Sets sensible defaults for a location bloc:
		- No redirects (code 0)
		- Directory listing disabled for security
		- Only GET method allowed by default (safest option)
*/
LocationConfig::LocationConfig() :
	path("/"),
	root(""),
	index("index.html"),
	upload_path(""),
	cgi_extension(""),
	cgi_path(""),
	redirect_url(""),
	redirect_code(0),
	autoindex(false)
{
	// By default, only allow GET, the safest HTTP method
	// POST and DELETE must be explicitly enabled in config
	allowed_methods.insert("GET");
}


//==================================================
//	ServerConfig Implementation
//==================================================

/*
	ServerConfig default constructor
	Sets defaults for a server block:
		- listen in all interfaces (0.0.0.0) port 8080
		- 1MB max body size (prevents exhaustion from large uploads)
		- No custom error pages (will use defaults)
*/
ServerConfig::ServerConfig() :
	host("0.0.0.0"),
	port(8080),
	client_max_body_size(1048576) // 1MB default, same as many webservers
{}


/*
	findLocation: Find the best matching location for a URI

	HTTP servers use "longest prefix match" to find the right location.
	For URI "/images/phot.jpg":
		- location "/" matches (prefix "/" is in URI)
		- location "/images" matches better (longer prefix)
		- location "/images/photo.jpg" would match best if it existed

	This is NGINX way of working and we're replicating that behaviour.
*/
const LocationConfig* ServerConfig::findLocation(const std::string &uri) const
{
	const LocationConfig* bestMatch = NULL;
	size_t longestMatch = 0;

	for (size_t i = 0; i < locations.size(); ++i)
	{
		const std::string& locPath = locations[i].path;

		// Check if this location's path is a prefix of the requested URI
		// Example: locPath = "/api" matches uri = "/api/users" but not "/apix"
		if (uri.compare(0, locPath.length(), locPath) == 0)
		{
			// For non-root locations, ensure we match at a path boundary
			// "/api" should match "/api" and "/api/foo" but not "/apiary"
			if (locPath != "/" && uri.length() > locPath.length()
				&& uri[locPath.length()] != '/')
			{
				continue;
			}

			// Keep track of the longest (most specific) match
			if (locPath.length() > longestMatch)
			{
				longestMatch = locPath.length();
				bestMatch = &locations[i];
			}
		}
	}
	return bestMatch;
}


//===========================================
//	Config Implementation
//===========================================

/*
	Default constructor - look for config in standard location
	If no config file is specified, servers typically look in:
		1. current directory (./webserv.con)
		2. /etc/webserv/webserv.conf (system-wide)
	We will use current directory for sinplicity
*/
Config::Config() : _configPath("config/default.conf")
{
	parseFile(_configPath);
	validateConfig();
}

/*
	Parametrized Consructor
	Used when the user specifies a config file on the command line:
		./webserv /path/to/my/config.conf
*/
Config::Config(const std::string& path) : _configPath(path)
{
	parseFile(_configPath);
	validateConfig();
}

// Destructor
Config::~Config() {}


/*
	trim: Remove leading and trailing whitespace from a string
	Essential for parsing config files where indentation varies
	Example:
			"	listen 8080;    " -> "listen 8080;"
*/
