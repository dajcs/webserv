/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:55:03 by anemet            #+#    #+#             */
/*   Updated: 2025/12/08 13:43:27 by anemet           ###   ########.fr       */
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
		// uri.compare(uri start pos, nr char to compare, the string to be matched)
		// in C++20: uri.starts_with(locPath)
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
		1. current directory (./webserv.conf)
		2. /etc/webserv/webserv.conf (system-wide)
	We will use ./config directory for simplicity
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
std::string Config::trim(const std::string& str) const
{
	size_t start = str.find_first_not_of(" \t\r\n\v");
	if (start == std::string::npos)
	{
		return;
	}
	size_t end = str.find_last_not_of(" \t\r\n\v");
	return str.substr(start, end - start + 1);
}

/*
	split: Split a string by a delimiter character
	Used to parse directives like "listen 0.0.0.0:8080"
	where we need to split "0.0.0.0:8080" by ';'
*/
std::vector<std::string> Config::split(const std::string& str, char delimiter) const
{
	std::vector<std::string> tokens;
	std::stringstream ss(str);
	std::string token;

	// getline: read from `ss` into `token` until `delimiter`
	// e.g.: str = "apple:banana:cherry", delimiter = ':'
	// -> loop1: apple, loop2: banana, loop3: cherry
	while (std::getline(ss, token, delimiter))
	{
		if (!token.empty())
		{
			tokens.push_back(trim(token));
		}
	}
}

/*
	removeComments: Strip comments from a line
	NGINX-style configs use # for comments:
		listen 8080; # This is a comment
	We need to remove everything after '#' (including '#')
*/
std::string Config::removeComments(const std::string& line) const
{
	size_t commentPos = line.find('#');
	if (commentPos != std::string::npos)
	{
		return line.substr(0, commentPos);
	}
	return line;
}

/*
	parseSize: Convert human readable size to bytes
	Config files often specify sizes like "10M" instead of "10485760"
	Supported suffixes:
		K or k = kilobytes (1024 bytes)
		M or m = megabytes (1024 * 1024 bytes)
		G or g = gigabytes (1024 * 1024 * 1024 bytes)
		no suffix = bytes
*/
size_t Config::parseSize(const std::string& sizeStr) const
{
	if (sizeStr.empty())
	{
		return 0;
	}

	std::string numPart = sizeStr;
	size_t multiplier = 1;

	// Check for size suffix (K, M, G)
	char lastChar = sizeStr[sizeStr.length() - 1];
	if (lastChar == 'K' || lastChar == 'k')
	{
		multiplier = 1024;
		numPart = sizeStr.substr(0, sizeStr.length() - 1);
	} else if (lastChar == 'M' || lastChar == 'm')
	{
		multiplier = 1024 * 1024;
		numPart = sizeStr.substr(0, sizeStr.length() - 1);
	} else if (lastChar == 'G' || lastChar == 'g')
	{
		multiplier = 1024 * 1024 * 1024;
		numPart = sizeStr.substr(0, sizeStr.length() - 1);
	}

	// Convert to number - atol is C++98 compatible ( no std::stoul)
	// `atol` is a C function from #include <cstdlib>
	// it works on classic '\0' terminated C string, => `.c_str()`
	long size = std::atol(numPart.c_str());
	if (size < 0)
	{
		throw ConfigException("Invalid size value: " + sizeStr);
	}

	return static_cast<size_t>(size) * multiplier;
}


/*
	parseFile: Main parsing entry point
	Opens the config file and parses its contents.
	The expected structure:

	server {
		... server directives ...
		location /path {
			... location directives ...
		}
	}
*/
void Config::parseFile(const std::string& path)
{
	
}
