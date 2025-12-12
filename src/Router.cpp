/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:55:59 by anemet            #+#    #+#             */
/*   Updated: 2025/12/12 10:01:35 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Router.hpp"
#include "Config.hpp"
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <unistd.h>
#include <fcntl.h>

/*
	=================================
		ROUTER OVERVIEW
	=================================

	Routing is the process of determining how to handle an HTTP request based on:
	1. The request URI (path)
	2. The HTTP method (GET, POST, DELETE)
	3. The server configuration (location blocks)

	Example config:
		server
		{
			listen 8080;

			location /
			{
				root 			/var/www;
				index			index.html;
				allow_methods	GET;
			}

			location /upload
			{
				root			/var/www;
				allow_methods 	POST DELETE;
				upload_dir		/var/www/uploads;
			}

			location /api
			{
				root			/var/www;
				allow_methods	GET POST;
			}
		}

	When reqeust "GET /upload/file.txt" arrives:
	1. Find server listening on the request's port (8080)
	2. Match URI "/upload/file.txt" to location blocks
	3. "/upload" matches better than "/" (more specific)
	4. Check if GET is allowed for /upload -> NO! (only POST DELETE)
	5. Return 405 Method not Allowed

	When request "GET /index.html" arrives:
	1. Match URI "/" (no more specific match)
	2. GET is allowed for /
	3. Resolve path: root + URI = /var/www/index.html
	4. Serve the file

*/


// ===============================
//  Constructors and Destructors
// ===============================

// Default Constructor
// setConfig() must be called before routing requests
// This pattern allows the Router to be created early and configured later
Router::Router() : _config(NULL) {}

// Parametrized Constructor
// Creates a Router with a reference to the server configuration
// The Router does not own the Config - it just references it
// The Config must remain valid for the Router's lifetime
Router::Router(const Config& config) : _config(&config) {}

// Destructor
// Nothing to clean up, we don't own the `config`
Router::~Router() {}

// Copy Constructor
Router::Router(const Router& other) : _config(other._config) {}

// copy Assignment Operator
Router& Router::operator=(const Router &other)
{
	if (this != &other)
	{
		_config = other._config;
	}
	return *this;
}


// ================================
//  Configuration
// ================================

/*
	setConfig() - Set or update the configuration reference

	The reason for its existence:
	- Server might be created before config is fully parsed
	- Config might be reloaded at runtime (SIGHUP)
	- Allows dependency injection for testing
*/
void Router::setConfig(const Config& config)
{
	_config = &config;
}


// ===============================
//  Main Routing Logic
// ===============================

/*
	route() - The main routing function

	It takes an HTTP request and determines how to handle it
	based on server configuration.

	Routing Algorithm:
	1. Find the server block that mathces the request's port
	2. Find the location block that best matches the request URI
	3. Check if the HTTP method is allowed for this location
	4. Handle redirections if configured
	5. Dispatch to appropriate handler (GET, POST, DELETE, CGI)

	Input:
		request:		the parsed HTTP request obj
		serverPort:		the port the request came in on

	Returns:
		Response object ready to be sent to the client

	Real-world example:
		Request: GET /images/photo.jpt HTTP/1.1
		Port: 8080

		1. Find server on port 8080
		2. Match /images/photo.jpg to locations:
			- "/" matches (prefix)
			- "/images" matches better
		3. Use /images location config
		4. Check: is GET allowed? Yes
		5. Resolve path: root + /photo.jpg
		6. Serve the file
*/
Response Router::route(const Request& request, int serverPort)
{
	// Step 1: Validate config
	if (!_config)
	{
		// No config = internal server error
		return errorResponse(500);
	}

	// Step 2: Find matching server block
	/*
		A webserver can host multiple "virtual servers" on different ports.
		Example:
			- Port 8080: Main website
			- Port 8081: Admin panel
			- Port 8082: API server

		We need to find which server block handles this port
	*/
	const ServerConfig* server = findServer(serverPort);
	if (!server)
	{
		return errorResponse(500);
	}

	// Step 3: Find matching location - the longest (most specific) match
	std::string requestPath = request.getPath();
	const LocationConfig* location = findLocation(*server, requestPath);
	if (!location)
	{
		// No location matches - user server's default settings
		// or return 404 if no default location
		return errorResponse(404, server);
	}

	// Step 4: Check for redirections
	/*
		Redirections tell the client to go elsewhere.
		Example config:
			location /old-page	{
				return 301 /new-page;
			}

		Response:
			HTTP/1.1 301 Moved Permanently
			Location: /new-page

		301 = Permanent redirect (browsers cache this)
		302 = Temporary redirect (browsers don't cache)
	*/
	if (!location->redirect_url.empty())
	{
		return Response::redirect(location->redirect_code, location->redirect_url);
	}

	// Step 5: Check HTTP method
	std::string method = request.getMethod();
	if (!isMethodAllowed(method, *location))
	{
		return errorResponse(405, server);
	}

	// Step 6: Check for CGI
	/*
		CGI (Common Gateway Interface) runs external programs to generate
		dynamic content (PHP, Python, etc.)

		Detection based on file extension:
		- /script.php -> run PHP interpreter
		- /script.py -> run Python interpreter

		This is configured per-location with cgi_extension and cgi_path
	*/
	std::string resolvedPath = resolvePath(requestPath, *location);
	if (isCgiRequest(resolvedPath, *location))
	{
		return handleCgi(request, resolvedPath, *location);
	}

	// Step 7: Dispatch to method handler
	/*
		Each HTTP method has different semantics:
			- GET: Retrieve a resource (read-only)
			- POST: Submit data (create, upload)
			- DELETE: Remove a resource
	*/
	if (method == "GET" || method == "HEAD")
	{
		return handleGet(request, *location);
	}
	else if (method == "POST")
	{
		return handlePost(request, *location);
	}
	else if (method == "DELETE")
	{
		return handleDelete(request, *location);
	}

	// Method not implemented (shouldn't reach here if Request validates methods)
	return errorResponse(501, server);
}


// ==========================================
//  Server and Location Finding
// ==========================================

/*
	findServer()  -  Find the server block for a given port

	Webservers can have multiple server blocks, each handling different ports
	or hostnames (virtual hosting)

	For simplicity we match by port only.
	Full virtual hosting would also check for the Host header.

	Input: port number
	Return: Pointer to matching ServerConfig or NULL if not found
*/
const ServerConfig* Router::findServer(int port) const
{
	const std::vector<ServerConfig> &servers = _config->getServers();

	for (size_t i = 0; i < servers.size(); ++i)
	{
		if (servers[i].port == port)
		{
			return &servers[i];
		}
	}

	// No exact match - return first server as default
	// This mimics NGINX's default_server behaviour
	if (!servers.empty())
	{
		return &servers[0];
	}

	return NULL;
}

/*
	findLocation()  -  Find the best matching location for a URI path

	Input: server, path
	Return: best matching LocationConfig, or NULL if no match
*/
const LocationConfig* Router::findLocation(const ServerConfig& server,
											const std::string& path) const
{
	const LocationConfig *bestMatch = NULL;
	size_t bestMatchLength = 0;

	const std::vector<LocationConfig>& locations = server.locations;

	for (size_t i = 0; i < locations.size(); i++)
	{
		const std::string& locationPath = locations[i].path;

		// check if location path is a prefix of the request path
		// location "/api" should match "/api", "/api/", "/api/users"
		if (path.compare(0, locationPath.length(), locationPath) == 0)
		{
			// Additional check: ensure we match at path boundary
			// "/api" should NOT match "/apiary"
			// it should match: "/api", "/api/", "/api"
			if (path.length() == locationPath.length() ||	// Exact match
				path[locationPath.length()] == '/' ||		// NOT "/apiary"
				locationPath == "/")						// Root always matches
			{
				// if better than prevous matches, store it
				if (locationPath.length() > bestMatchLength)
				{
					bestMatch = &locations[i];
					bestMatchLength = locationPath.length();
				}
			}
		}
	}

	return bestMatch;
}


// =======================================
//  Path Resolution & Validation
// =======================================

/*
	resolvePath()  -  Convert URI to filesystem path

	Example:
		Location config:
			location /images {
				root /var/www;
			}

		Request: GET /images/photo.jpg
		Resolved: /var/www/images/photo.jpg

	SECURITY: Must prevent directory traversal attacks!
		Request: GET /images/../../../etc/passwd
		BAD: /var/www/images/../../../etc/passwd = /etc/passwd
		GOOD: Detect ".." and reject or sanitize

	Input:
		uri:	 	the request URI path
		location:	the matching location configuration
	Return:
		Absolute filesystem path to the requested resource
*/
std::string Router::resolvePath(const std::string& uri, const LocationConfig& location)
{
	std::string root = location.root;

	// Ensure root doesn't end with /
	if (!root.empty() && root[root.length() - 1] == '/')
	{
		root = root.substr(0, root.length() - 1);
	}

	std::string path = uri;

	// Security: Sanitize path by removing ".." components
	std::string sanitized;
	std::stringstream ss(path);
	std::string segment;
	std::vector<std::string> segments;

	// Split path by '/' and process
	while (std::getline(ss, segment, '/'))
	{
		if (segment == "..")
		{
			// Go up one directory - but don't go above root!
			if (!segments.empty())
			{
				segments.pop_back();
			}
			// If segments is empty, we'd be going above root - ignore
		}
		else if (segment == "." || segment.empty())
		{
			// current directory or empty segment - skip
			continue;
		}
		else
		{
			segments.push_back(segment);
		}
	}

	// Rebuild sanitized path
	sanitized = "";
	for (size_t i = 0; i < segments.size(); ++i)
	{
		sanitized += "/" + segments[i];
	}

	if (sanitized.empty())
	{
		sanitized = "/";
	}

	// combine root with sanitized path
	std::string fullPath = root + sanitized;

	return fullPath;
}


/*
	isMethodAllowed  -  check if HTTP method is permitted for this location

	Input: method, location obj
	Return: true if allowed, false otherwise
*/
bool Router::isMethodAllowed(const std::string& method, const LocationConfig& location)
{
	// If no methods specified, allow GET (alternatively can allow all)
	if (location.allowed_methods.empty())
	{
		// Default behaviour: allow GET only
		return (method == "GET" || method == "HEAD");
	}

	// Check if method is in the allowed_methods set
	if (location.allowed_methods.count(method) > 0)
	{
		return true;
	}

	// HEAD is allowed if GET is allowed (HEAD = GET without a body)
	if (method == "HEAD" && location.allowed_methods.count("GET") > 0)
	{
		return true;
	}

	return false;
}


/*
	isCgiRequest()  -  Determine if request should be handled by CGI

	CGI requests are identified by file extension:
		.php	-> PHP CGI
		.py		-> Python CGI
		.pl		-> Perl CGI

	The location must have CGI configured for this extension

	Input:
		path:		The resolved filesystem path
		location:	The location config
	Returns: true if CGI should be handled, false otherwise
*/
bool Router::isCgiRequest(const std::string& path, const LocationConfig& location)
{
	// Check if CGI is configured for this location
	if (location.cgi_extension.empty() || location.cgi_path.empty())
	{
		return false;
	}

	// Check if path ends with the CGI extension
	const std::string& ext = location.cgi_extension;
	if (path.length() >= ext.length())
	{
		return (path.compare(path.length() - ext.length(), ext.length(), ext) == 0);
	}

	return false;
}


// =============================================
//  HTTP Method Handlers
// =============================================

/*
	handleGet()  -  Handle GET and HEAD requests

	GET retrieves a resource (file, directory listing, etc.)
	HEAD is identical to GET but returns only headers (no body)

	Process:
	1. Resolve the filesystem path
	2. Check if path exists
	3. If directory: serve index file or directory listing
	4. if file: serve the file content

	Input:
		request:	The HTTP request
		location:	The matching location config
*/
Response Router::handleGet(const Request& request, const LocationConfig& location)
{
	std::string path = resolvePath(request.getPath(), location);

	// Use stat() to check if path exists and get info
	struct stat pathStat;
	if (stat(path.c_str(), &pathStat) != 0)
	{
		// Can't access path
		return errorResponse (404);
	}

	// Check if it's a directory
	if (S_ISDIR(pathStat.st_mode))
	{
		return serveDirectory(path, location);
	}

	// It's a file - serve it
	return serveFile(path);
}

/*
	handlePost()  -  Handle POST requests

	POST is used for:
	- Form submissions
	- File uploads
	- Creating new resources

	For file uploads, we need to:
	1. Parse the request body (multipart/form-data or raw)
	2. Save uploaded files to the upload directory
	3. Return success/failure response

	Input:
		request:	The HTTP request (contains body data)
		location:	The matching location configuration

	Returns:
		Response indicating success (201 Created) or error
*/
Response Router::handlePost(const Request& request, const LocationConfig& location)
{
	// Check if upload directory is configured
	if (location.upload_path.empty())
	{
		// No upload directory configured - reject
		// Could also handle as form submission
		return errorResponse(403);
	}

	// For now, implement simple file upload
	// Full implementation would parse multipart/form-data

	// get Content-Type to determine how to handle body
	std::string contenttype = request.getHeader("Content-Type");

	// Simple implementation: save body as raw file
	// Generate filename from timestamp
	std::stringstream filename;
	filename << location.upload_path << "/upload_" << time(NULL);

	// Try to create the file
	std::ofstream outFile(filename.str().c_str(), std::ios::binary);
	if (!outFile)
	{
		return errorResponse(500); // Couldn't create file
	}

	outFile << request.getBody();
	outFile.close();

	// Success - return 201 Created
	Response response;
	response.setStatus(201, "Created");
	response.setHeader("Location", filename.str());
	response.setContentType("text/plain");
	response.setBody("File uploaded successfully\n");

	return response;
}

/*
	handleDelete() - Handle DELETE requests

	DELETE removes a resource from the server (RFC 7231)

	How DELETE fits into HTTP semantics:
	- GET:    Read a resource
	- POST:   Create a resource
	- PUT:    Replace a resource
	- DELETE: Remove a resource

	HTTP Response Codes for DELETE:
	-------------------------------
	Success:
		- 200 OK:         Resource deleted, response body has details
		- 202 Accepted:   Deletion queued (for async operations)
		- 204 No Content: Resource deleted, no response body (most common)

	Client Errors (4xx):
		- 400 Bad Request:  Malformed request
		- 403 Forbidden:    Server refuses (permissions, read-only, etc.)
		- 404 Not Found:    Resource doesn't exist
		- 405 Method Not Allowed: DELETE not permitted for this URI
		- 409 Conflict:     Cannot delete (e.g., directory not empty)

	Server Errors (5xx):
		- 500 Internal Server Error: Unexpected server failure


	Request/Response Example:
	-------------------------
	Request:
		DELETE /uploads/document.txt HTTP/1.1
		Host: localhost:8080

	Response (success):
		HTTP/1.1 204 No Content
		Date: Thu, 12 Dec 2024 10:30:00 GMT
		Server: webserv/1.0

	Response (file not found):
		HTTP/1.1 404 Not Found
		Date: Thu, 12 Dec 2024 10:30:00 GMT
		Server: webserv/1.0
		Content-Type: text/html
		Content-Length: 162

		<html><body><h1>404 Not Found</h1></body></html>


	Implementation Notes:
	---------------------
	- We use unlink() to delete files (POSIX standard)
	- unlink() removes the directory entry; file data is freed when no
		process has it open
	- Directories require rmdir() or recursive deletion (not implemented)
	- We don't support conditional DELETE (If-Match headers) - that's advanced


	Security:
	- Only allow deletion within allowed directories
	- Don't allow deleting directories (or require special handling)
	- Log deletions for audit trail

	Input:
		request:	The HTTP request
		location:	The matching location config
	Returns:
		204 No Content on success, or error response
*/
Response Router::handleDelete(const Request& request, const LocationConfig& location)
{
	// Convert the URI (e.g., "/uploads/file.txt") to an absolute
	// filesystem path (e.g., "/var/www/uploads/file.txt").
	std::string path = resolvePath(request.getPath(), location);

	// check if file exists
	struct stat pathStat;
	if (stat(path.c_str(), &pathStat) != 0)
	{
		return errorResponse(404); // Not Found
	}

	// Don't allow deleting directories
	if (S_ISDIR(pathStat.st_mode))
	{
		return errorResponse(409); // Conflict
	}

	// Attempt to delete the file
	if (unlink(path.c_str()) != 0)
	{
		return errorResponse(403); // Forbidden (permission denied)
	}

	// Success - return 204 No Content
	return Response::noContent();
	// Alternative: 200 OK with a body confirming deletion
	// 	{"status": "deleted", "path": "/uploads/file.txt"}

}


// ===============================
//  File Serving
// ===============================

/*
	serveFile()  -  Read and return a file's contents

	Process:
	1. Open the file
	2. Read contents into memory
	3. Determine Content-Type from extension
	4. Build response with appropiriate headers

	Input:
		filepath:	Absolute path to the file
	Return:
		Response with file contents and appropriate headers
*/
Response Router::serveFile(const std::string& filepath)
{
	// Open file for reading in binary mode
	std::ifstream file(filepath.c_str(), std::ios::binary);
	if (!file)
	{
		return errorResponse(404);
	}

	// Read entire file into string
	std::stringstream contents;
	contents << file.rdbuf();
	file.close();

	// Determine Content-Type from file extension
	std::string extension;
	size_t dotPos = filepath.rfind('.');
	if (dotPos != std::string::npos)
	{
		extension = filepath.substr(dotPos);
	}

	std::string contentType = Response::getMimeType(extension);

	// Build response
	Response response;
	response.setStatus(200, "OK");
	response.setContentType(contentType);
	response.setBody(contents.str());

	return response;
}

/*
	serveDirectory()  -  Handle directory requests

	When a directory is requested, we can:
	1. Serve and index file (index.html) if it exists
	2. Generate a directory listing if enabled
	3. Return 403 Forbidden if directory listing is disabled

	Input:
		dirpath:	Absolute path to the directory
		location:	Locaction config (has index and autoindex settings)
*/
Response Router::serveDirectory(const std::string& dirpath, const LocationConfig& location)
{
	// First try to serve index file
	if (!location.index.empty())
	{
		std::string indexPath = dirpath;
		if (indexPath[indexPath.length() - 1] != '/')
		{
			indexPath += "/";
		}
		indexPath += location.index;

		struct stat indexStat;
		if (stat(indexPath.c_str(), &indexStat) == 0 && S_ISREG(indexStat.st_mode))
		{
			return serveFile(indexPath);
		}
	}

	// location has no index file,
	// check if directory listing is enabled
	if (!location.autoindex)
	{
		return errorResponse(403); // Forbidden - no directory listing
	}

	// Generate directory listing
	DIR* dir = opendir(dirpath.c_str());
	if (!dir)
	{
		return errorResponse(500);
	}

	// Build HTML directory listing
	std::stringstream html;
	html << "<!DOCTYPE html>\n";
	html << "<html>\n<head>\n";
	html << "<title>Index of " << dirpath << "</title>\n";
	html << "<style>\n";
	html << "body { font-family: monospace; }\n";
	html << "a { text-decoration: none; }\n";
	html << "a:hover { text-decoration: underline; }\n";
	html << "</style>\n";
	html << "</head>\n<body>\n";
	html << "<h1>Index of " << dirpath << "</h1>\n";
	html << "<hr>\n<pre>\n";

	// Add parent directory link
	html << "<a href=\"../\">..</a>\n";

	// List directory contents
	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL)
	{
		std::string name = entry->d_name;

		// Skip . and ..
		if (name == "." || name == "..")
		{
			continue;
		}

		// Check if it's a directory
		std::string fullPath = dirpath + "/" + name;
		struct stat entryStat;
		if (stat(fullPath.c_str(), &entryStat) == 0 && S_ISDIR(entryStat.st_mode))
		{
			name += "/";
		}

		html << "<a href=\"" << name << "\">" << name << "</a>\n";
	}

	closedir(dir);

	html << "</pre>\n<hr>\n";
	html << "</body>\n</html>\n";

	Response response;
	response.setStatus(200, "OK");
	response.setContentType("text/html");
	response.setBody(html.str());

	return response;
}


// ===============================
//  CGI Handling (Stub)
// ===============================

/*
	handleCgi()  -  Execute CGI script and return response

	This is a stub for now - full implementation in Phase 8

	CGI execution involves:
	1. fork() to create child process
	2. Set up pipes for stdin/stdout
	3. Set environment variables
	4. execve() the CGI interpreter
	5. Read output and parse headers

	Input:
		request: 	The HTTP request
		scriptPath:	Path to the CGI script
		location:	Location config
	Returns:
		Response from CGI script or error
*/
Response Router::handleCgi(const Request& request, const std::string& scriptPath,
							const LocationConfig& location)
{
	// TODO: Implement in Phase 8 (CGI)
	// For now, return 501 Not Implemented
	(void)request;
	(void)scriptPath;
	(void)location;

	return errorResponse(501);
}


// ======================================
//  Error Response Generation
// ======================================

/*
	errorResponse()  -  Generate and HTTP error response

	Creates an error response with:
	- Appropriate status code and reason phrase
	- Custom error page (if configured) or default page
	- Required headers

	Input:
		code:	HTTP error code (400, 404, 500, etc.)
		server:	Optional server config for custom error pages
	Returns:
		Error response ready to send
*/
Response Router::errorResponse(int code, const ServerConfig* server)
{
	// Check for custom error page
	if (server)
	{
		std::map<int, std::string>::const_iterator it = server->error_pages.find(code);
		if (it != server->error_pages.end())
		{
			// Custom error page configured
			std::string errorPagePath = it->second;

			// Try to serve the custom error page
			std::ifstream file(errorPagePath.c_str());
			if (file)
			{
				std::stringstream contents;
				contents << file.rdbuf();
				file.close();

				Response response;
				response.setStatus(code);
				response.setContentType("text/html");
				response.setBody(contents.str());
				return response;
			}
		}
	}
	// Default error page
	return Response::error(code);
}
