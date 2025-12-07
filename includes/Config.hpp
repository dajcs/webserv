/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:56:48 by anemet            #+#    #+#             */
/*   Updated: 2025/12/07 20:50:37 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>


/*
	LocationConfig: Represents a "location" block in the config file

	In HTTP servers a "location" defines how to handle requests for specific URL paths.
	For example:
		location /images { root /var/www/images; }
	tells the server to look for files in /var/www/images when the URL starts with /images
*/
struct LocationConfig
{
	std::string path;			// The URL path this location matches (e.g., "/", "/api")
	std::string root;			// Filesystem directory to serve files from
	std::string index;			// Default file to serve for directory requests (e.g., "index.html")
	std::string upload_path;	// Where to store uploaded files
	std::string cgi_extension;	// File extension that triggers CGI execution (e.g., ".php")
	std::string cgi_path;		// Path to the CGI interpreter (e.g., "/usr/bin/php-cgi")
	std::string redirect_url;	// URL to redirect to (for HTTP redirections)
	int redirect_code;			// HTTP redirect status code (301, 302, etc.)
	bool autoindex;				// Enable directory listing when no index file exists
	std::set<std::string> allowed_methods;	// HTTP methods allowed for this route (GET, POST, DELETE)

	// Constructor with sensible defaults
	LocationConfig();
};


/*
	ServerConfig: Represents a "server" block in the config file

	A server block defines a virtual server - it can listen on a specific host:port combination
	and handle requests for specific server names.
	This allows one webserv instance to host multiple websites.
*/
struct ServerConfig
{
	std::string host;			// Ip address to bind to (e.g., "0.0.0.0", "127.0.0.1")
	int port;					// TCP port to listen on (e.g., 8080)
	std::vector<std::string> server_names;	// Virtual host names (e.g., "example.com", "www.example.com")
	std::map<int, std::string> error_pages;	// Custom error pages: status_code ->file_path
	size_t client_max_body_size;			// Maximum allowed request body size in bytes
	std::vector<LocationConfig> locations;	// All locations blocks for this server

	// Constructor with sensible defaults
	ServerConfig();

	// Find the best matching location for a given URI
	// Uses "longest prefix match" - the most specific location wins
	const LocationConfig* findLocation(const std::string& uri) const;
};


/*
	Config: The main configuration class that parses and stores all settings.

	The class reads an NGINX-like configuration file and creates a structured representation
	that the server can use at runtime. The config file format:

	server
	{
		listen 8080;
		server_name localhost;
		location /
		{
			root /var/www/html;
		}
	}
*/
class Config
{
	private:
		std::vector<ServerConfig> _servers;	// All parsed server blocks
		std::string _configPath;			// Path to config file

		// Parsing helper methods
		void parseFile(const std::string& path);
		void parseServerBlock(std::ifstream& file, std::string& line);
		void parseLocationBlock(std::ifstream& file, std::string& line, ServerConfig& server);

		// Utility methods for parsing
		std::string trim(const std::string& str) const;
		std::vector<std::string> split(const std::string& str, char delimiter) const;
		std::string removeComments(const std::string& line) const;
		size_t parseSize(const std::string& sizeStr) const;

		// Validation
		void validateConfig() const;


	public:
		// Default constructor - uses default config path
		Config();

		// Parametrized constructor
		Config(const std::string& path);

		// Destructor
		~Config();


		// Accessors
		const std::vector<ServerConfig>& getServers() const;
		const ServerConfig* getServerByHostPort(const std::string& host, int port) const;

		// Debug: print parsed configuration
		void printConfig() const;
};


/*
	ConfigException: Custom exception for configuration parsing errors

	Thrown when the config file has syntax errors, missing required fields, or invalid values.
	Provide maningful error messages to help users fix their configuration.
*/
class ConfigException : public std::exception
{
	private:
		std::string _message;

	public:
		ConfigException(const std::string& msg) : _message(msg) {}
		virtual ~ConfigException() throw() {}
		virtual const char* what() const throw() { return _message.c_str(); }
};




#endif
