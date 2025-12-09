/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:55:03 by anemet            #+#    #+#             */
/*   Updated: 2025/12/09 11:48:37 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Config.hpp"


/*
	key concepts

	1. Server Blocks: Each server {} defines a virtual server that listens on a specific host:port.
		This allows one webserv to host multiple websites.

	2. Location Blocks: Each location {} within a server defines how to handle requests for a specific URL path.
		The "longest prefix match" algorithm finds the most specific match.

	3. Directives: Configuration options like listen, root, index. Each modifies server behavior.

*/


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
		return "";
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
	return tokens;
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

	server
	{
		... server directives ...

		location /path
		{
			... location directives ...
		}
	}
*/
void Config::parseFile(const std::string& path)
{
	std::ifstream file(path.c_str());

	if (!file.is_open())
	{
		throw ConfigException("Cannot open config file: " + path);
	}

	std::string line;
	// read line-by-line into `line` without terminating '\n'
	while (std::getline(file, line))
	{
		// Clean up the line: remove comments and whitespace
		line = removeComments(line);
		line = trim(line);

		// Skip empty lines
		if (line.empty())
		{
			continue;
		}

		// Look for "server" keyword to start a server block
		if (line.find("server") == 0)
		{
			parseServerBlock(file, line);
		}
	}

	file.close();

	// Must have at least one server defined
	if (_servers.empty())
	{
		throw ConfigException("No server blocks defined in config file");
	}
}

/*
	parseServerBlock: Parse a single server { ... } block

	A server block contains:
		- listen: host:port to bind to
		- server_name: virtual host names
		- error_page: custom error pages
		- client_max_body_size: max upload size
		- location blocks: route-specific settings
*/
void Config::parseServerBlock(std::ifstream& file, std::string& line)
{
	ServerConfig server;

	// Look for opening brace
	// 	might be on the same line or next line)
	if (line.find('{') == std::string::npos)
	{
		// Opening brace should be on next line
		if (!std::getline(file, line))
		{
			throw ConfigException("Unexpected end of file in server block");
		}
		line = trim(removeComments(line));
		if (line != "{")
		{
			throw ConfigException("Expected '{' after 'server'");
		}
	}

	// Parse server block contents until closing brace
	while (std::getline(file, line))
	{
		line = removeComments(line);
		line = trim(line);

		if (line.empty()) 	continue;

		// End of server block
		if (line == "}") 	break;

		// Check for location block
		if (line.find("location") == 0)
		{
			parseLocationBlock(file, line, server);
			continue;
		}

		// Remove trailing semicolon for easier parsing
		// "listen 8080;" -> "listen 8080"
		if (line[line.length() - 1] == ';')
		{
			line = line.substr(0, line.length() - 1);
		}

		// Split directive into tokens: "listen 8080" -> ["listen", "8080"]
		std::vector<std::string> tokens = split(line, ' ');

		if (tokens.empty()) 	continue;

		std::string directive = tokens[0];

		// Parse each directive type
		if (directive == "listen")
		{
			/*
				listen directive: specifies host:port to listen on
				Formats:
					listen 8080;			-> listen on 0.0.0.0:8080
					listen 127.0.0.1:8080;	-> listen on localhost only
			*/
			if (tokens.size() < 2)
			{
				throw ConfigException("listen directive requires a value");
			}

			std::string listenValue = tokens[1];
			size_t colonPos = listenValue.find(':');

			if (colonPos != std::string::npos)
			{
				// Format -> host:port
				server.host = listenValue.substr(0, colonPos);
				server.port = std::atoi(listenValue.substr(colonPos + 1).c_str());
			}
			else
			{
				// Format: port only (bind to all interfaces)
				server.host = "0.0.0.0";
				server.port = std::atoi(listenValue.c_str());
			}
		}
		else if (directive == "server_name")
		{
			/*
				server_name directive: virtual host names
				Used for name based virtual hosting
				(out of scope per subject, but useful for routing if implemented)
				Example: server_name example.com www.example.com
			*/for (size_t i = 1; i < tokens.size(); ++i)
			{
				server.server_names.push_back(tokens[i]);
			}
		}
		else if (directive == "error_page")
		{
			/*
				error_page directive: custom error pages
				Format: error_page 404 /errors/404.html
				Maps HTTP status codes to custom HTML files
			*/
			if (tokens.size() < 3)
			{
				throw ConfigException("error_page directive requires status code and path");
			}
			int statusCode = std::atoi(tokens[1].c_str());
			std::string errorPath = tokens[2];
			server.error_pages[statusCode] = errorPath;
		}
		else if (directive == "client_max_body_size")
		{
			/*
				client_max_body_size directive: limits request body size
				Prevents denial-of-service attacks via huge uploads
				Example: client_max_body_size 10M;
			*/
			if (tokens.size() < 2)
			{
				throw ConfigException("client_max_body_size directive requires a value");
			}
			server.client_max_body_size = parseSize(tokens[1]);
		}

	} //end of server while (getline) loop

	// Add a default location if none specified
	if (server.locations.empty())
	{
		LocationConfig defaultLoc;
		defaultLoc.path = "/";
		defaultLoc.root = "www";
		server.locations.push_back(defaultLoc);
	}


	_servers.push_back(server);
}


/*
	parseLocationBlock: Parse a single location { ... } block

	A location block defines behavior for a specific URL path.
	It contains directives like:
		- root: filesystem path to serve files from
		- index: default file for directory requests
		- allowed_methods: which HTTP methods are permitted
		- autoindex: whether to show directory listings
		- cgi_pass: CGI configuration
		- return: HTTP redirects
*/
void Config::parseLocationBlock(std::ifstream& file, std::string& line, ServerConfig& server)
{
	LocationConfig location;

	// Extract the location path from "location /path {"
	// We need to handle both "location /path {" and "location /path"
	std::vector<std::string> parts = split(line, ' ');
	if (parts.size() < 2)
	{
		throw ConfigException("location directive requires a path");
	}

	location.path = parts[1];
	// Remove opening brace if attached to path
	if (!location.path.empty() && location.path[location.path.length() - 1] == '{')
	{
		location.path = location.path.substr(0, location.path.length() - 1);
	}
	location.path = trim(location.path);

	// Look for opening brace if not on this line
	if (line.find('{') == std::string::npos)
	{
		if (!std::getline(file, line))
		{
			throw ConfigException("Unexpected end of file in location block");
		}
		line = trim(removeComments(line));
		if (line != "{")
		{
			throw ConfigException("Expected '{' after location path");
		}
	}

	// Parse location block contents
	while (std::getline(file, line))
	{
		line = removeComments(line);
		line = trim(line);

		if (line.empty())	continue;

		// End of location block
		if (line == "}") 	break;

		// Remove trailing semicolon
		if (line[line.length() - 1] == ';')
		{
			line = line.substr(0, line.length() - 1);
		}

		std::vector<std::string> tokens = split(line, ' ');
		if (tokens.empty()) 	continue;

		std::string directive = tokens[0];

		if (directive == "root")
		{
			/*
				root directive: filesystem directory for this location
				Example: root /var/www/html;
				Request for /images/photo.jpg would look for /var/www/html/images/photo.jpg
			*/
			if (tokens.size() < 2)
			{
				throw ConfigException("root directive requires a path");
			}
			location.root = tokens[1];
		}
		else if (directive == "index")
		{
			/*
				index directive: default file for directory requests
				Example: index index.html index.htm;
				When user requests "/", server looks for /root/index.html first
			*/
			if (tokens.size() < 2)
			{
				throw ConfigException("index directive requires a filename");
			}
			location.index = tokens[1];
		}
		else if (directive == "allowed_methods" || directive == "limit_except")
		{
			/*
				allowed_methods directive: which HTTP methods are allowed
				Example: allowed_methods GET POST;
				This is crucial for security - only allowe necessary methods
			*/
			location.allowed_methods.clear();
			for (size_t i = 1; i < tokens.size(); ++i)
			{
				location.allowed_methods.insert(tokens[i]);
			}
		}
		else if (directive == "autoindex")
		{
			/*
				autoindex directive: enable directory listing
				Example: autoindex on;
				When enabled, if no index file exists, shows directory contents
				Security note: can expose file structure, disable by default
			*/
			location.autoindex = (tokens[1] == "on");
		}
		else if (directive == "puload_path" || directive == "upload_store")
		{
			/*
				upload_path directive: where to save uploaded files
				Example: upload_path /var/www/uploads;
				POST requests with file uploads will save files here
			*/
			if (tokens.size() < 2)
			{
				throw ConfigException("uplaod_path directive requires a path");
			}
			location.upload_path = tokens[1];
		}
		else if (directive == "cgi_pass" || directive == "fastcgi_pass")
		{
			/*
				cgi_pass directive: CGI interpreter path
				Example: cgi_pass /usr/bin/php-cgi;
				When a request matches cgi_extension, run this program
			*/
			if (tokens.size() < 2)
			{
				throw ConfigException("cgi_pass directive requires a path");
			}
			location.cgi_path = tokens[1];
		}
		else if (directive == "cgi_extension")
		{
			/*
				cgi_extension directive: file extension to trigger CGI
				Example: cgi_extension .php
				Files ending in .php will be executed via CGI
			*/
			if (tokens.size() < 2)
			{
				throw ConfigException("cgi_extension directive requires an extension");
			}
			location.cgi_extension = tokens[1];
		}
		else if (directive == "return")
		{
			/*
				return directive: HTTP redirection
				Example: return 301 https://newsite.com;
				301 = permanent redirect (browsers cache this)
				302 = temporary redirect
				Used for URL restructuring or forcing HTTPS
			*/
			if (tokens.size() < 2)
			{
				throw ConfigException("return directive requires status code");
			}
			location.redirect_code = std::atoi(tokens[1].c_str());
			if (tokens.size() >= 3)
			{
				location.redirect_url = tokens[2];
			}
		}
	}  //end of location while (getline) loop

	server.locations.push_back(location);
}


/*
	validateConfig: Ensure the parsed configuration is valid

	Checks for common configuration errors:
		- port numbers in valid range (1..65535)
		- root directories exist (optional, could check at request time)
		- no duplicate port bindings on same host
*/
void Config::validateConfig() const
{
	std::set<std::pair<std::string, int> > usedPorts;

	for (size_t i = 0; i < _servers.size(); ++i)
	{
		const ServerConfig& server = _servers[i];

		// Validate port number
		if (server.port <= 0 || server.port > 65535)
		{
			std::ostringstream oss;
			oss << "Invalid port number: " << server.port;
			throw ConfigException(oss.str());
		}

		// Check for duplicate host:port combinations
		std::pair<std::string, int> hostPort(server.host, server.port);
		if (usedPorts.find(hostPort) != usedPorts.end())
		{
			std::ostringstream oss;
			oss << "Duplicate listen address: " << server.host << ":" << server.port;
			throw ConfigException(oss.str());
		}
		usedPorts.insert(hostPort);

		// Validate locations
		for (size_t j = 0; j < server.locations.size(); ++j)
		{
			const LocationConfig& loc = server.locations[j];

			//Location path must start with /
			if (loc.path.empty() || loc.path[0] != '/')
			{
				throw ConfigException("Location path must start with /: " + loc.path);
			}

			// If redirect is set, code must be valid
			if (!loc.redirect_url.empty() &&
				(loc.redirect_code < 300 || loc.redirect_code > 399))
			{
				throw ConfigException("Redirect code must be 3xx");
			}
		}
	}
}


/*
	getServers: Return all parsed server configurations
	Used by the Server class to set up listening sockets
*/
const std::vector<ServerConfig>& Config::getServers() const
{
	return _servers;
}


/*
	getServerByHostPort: Find a server config by its listen address
	Used to route incoming connections to the correct server block
*/
const ServerConfig* Config::getServerByHostPort(const std::string& host, int port) const
{
	for (size_t i = 0; 8 < _servers.size(); ++i)
	{
		if (_servers[i].host == host && _servers[i].port == port)
		{
			return &_servers[i];
		}
		// Also check for 0.0.0.0 which matches any host
		if (_servers[i].host == "0.0.0.0" && _servers[i].port == port)
		{
			return &_servers[i];
		}
	}

	return NULL;
}


/*
	printConfig: Debug function to display parsed configuration
	Useful for verifying the parser is working correctly
*/
void Config::printConfig() const
{
	std::cout << "=== Parsed Configuration ===" << std::endl;

	for (size_t i = 0; i < _servers.size(); ++i)
	{
		const ServerConfig& server = _servers[i];

		std::cout << "\nServer *" << (i + 1) << ":" << std::endl;
		std::cout << "  Listen: " << server.host << ":" << server.port << std::endl;

		std::cout << "  Server names: ";
		for (size_t j = 0; j < server.server_names.size(); ++j)
		{
			std::cout << server.server_names[j] << " ";
		}
		std::cout << std::endl;

		std::cout << "  Client max body size: " << server.client_max_body_size << " bytes" << std::endl;

		std::cout << "  Error pages:" << std::endl;
		for (std::map<int, std::string>::const_iterator it = server.error_pages.begin();
															it != server.error_pages.end(); ++it)
		{
			std::cout << "    " << it->first << " -> " << it->second << std::endl;
		}

		std::cout << "  Locations:" << std::endl;
		for (size_t j = 0; j < server.locations.size(); ++j)
		{
			const LocationConfig& loc = server.locations[j];
			std::cout << "    Location: " << loc.path << std::endl;
			std::cout << "      Root: " << loc.root << std::endl;
			std::cout << "      Index: " << loc.index << std::endl;
			std::cout << "      Autoindex: " << (loc.autoindex ? "on" : "off") << std::endl;
			std::cout << "      Methods: ";
			for (std::set<std::string>::const_iterator it = loc.allowed_methods.begin();
															it != loc.allowed_methods.end(); ++it)
			{
				std::cout << *it << " ";
			}
			std::cout << std::endl;
			if (!loc.cgi_extension.empty())
			{
				std::cout << "      CGI: " << loc.cgi_extension << " -> " << loc.cgi_path << std::endl;
			}
			if (!loc.redirect_url.empty())
			{
				std::cout << "      Redirect: " << loc.redirect_code << " " << loc.redirect_url << std::endl;
			}
		}
	}
}
