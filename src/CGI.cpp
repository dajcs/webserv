/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:56:09 by anemet            #+#    #+#             */
/*   Updated: 2025/12/13 17:42:44 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGI.hpp"
#include "Request.hpp"
#include "Config.hpp"

#include <sys/stat.h>   // stat()
#include <unistd.h>     // access()
#include <cstdlib>      // malloc, free
#include <cstring>      // strlen, strcpy
#include <sstream>

/*
	==================================
		CGI Implementation Overview
	==================================

	This file implements CGI (Common Gateway Interface) detection and setup.
	The actual execution (fork, pipe, execve) will be in Step 8.2.

	CGI Flow in webserv:
	--------------------
	1. Request arrives: GET /cgi-bin/hello.py?name=World
	2. Router calls isCgiRequest() -> true (matches .py extension)
	3. Router creates CGI object and calls setup()
	4. CGI::setup() validates script, builds environment
	5. Router calls CGI::execute() (Step 8.2)
	6. CGI output is parsed and sent back as HTTP response

	Environment Variables Deep Dive:
	--------------------------------
	CGI scripts receive information through environment variables.
	This is the ONLY way the server communicates with the script
	(besides stdin for POST data).

	When the script runs, it can do:
		import os
		method = os.environ['REQUEST_METHOD']  # "GET"
		query = os.environ['QUERY_STRING']     # "name=World"

	The environment must be complete and accurate, or scripts will fail!

	Memory Management:
	------------------
	execve() requires char** arrays that we must allocate manually.
	These are freed in the parent process after fork(), or if execve() fails.
	We use C-style allocation (malloc/free) for C function compatibility.
*/



// =========================================
//  Constructors & Destructor
// =========================================

/*
	Default Constructor
	Creates an empty CGI handler in non-ready state.
*/
CGI::CGI() :
	_request(NULL),
	_location(NULL),
	_scriptPath(""),
	_interpreterPath(""),
	_workingDirectory(""),
	_pathInfo(""),
	_ready(false),
	_errorCode(0),
	_errorMessage("")
{}

/*
	Parameterized Constructor
	Stores references to request and location for later use.

	Note: We store pointers, not copies. The Request and LocationConfig
	must remain valid for the lifetime of this CGI object.
*/
CGI::CGI(const Request& request, const LocationConfig& location) :
	_request(&request),
	_location(&location),
	_scriptPath(""),
	_interpreterPath(""),
	_workingDirectory(""),
	_pathInfo(""),
	_ready(false),
	_errorCode(0),
	_errorMessage("")
{}

/*
	Destructor
	Nothing to clean up - we don't own the Request or LocationConfig.
	Environment arrays are created on-demand and freed by caller.
*/
CGI::~CGI() {}

/*
	Copy Constructor
	Deep copy of all members. Pointers still point to same Request/Location.
*/
CGI::CGI(const CGI& other) :
	_request(other._request),
	_location(other._location),
	_scriptPath(other._scriptPath),
	_interpreterPath(other._interpreterPath),
	_workingDirectory(other._workingDirectory),
	_pathInfo(other._pathInfo),
	_envVars(other._envVars),
	_ready(other._ready),
	_errorCode(other._errorCode),
	_errorMessage(other._errorMessage)
{}


/*
	Copy Assignment Operator
*/
CGI& CGI::operator=(const CGI& other)
{
	if (this != &other)
	{
		_request = other._request;
		_location = other._location;
		_scriptPath = other._scriptPath;
		_interpreterPath = other._interpreterPath;
		_workingDirectory = other._workingDirectory;
		_pathInfo = other._pathInfo;
		_envVars = other._envVars;
		_ready = other._ready;
		_errorCode = other._errorCode;
		_errorMessage = other._errorMessage;
	}
	return *this;
}


// =========================================
//  Static Detection Method
// =========================================

/*
	isCgiRequest() - Determine if a request should be handled as CGI

	Detection logic:
	1. Check if location has CGI configured (both extension and path must be set)
	2. Check if the file path ends with the configured extension

	Example:
		Location config:
			cgi_extension: ".py"
			cgi_path: "/usr/bin/python3"

		Request path: "/var/www/cgi-bin/hello.py"
		Result: true (ends with .py, CGI is configured)

		Request path: "/var/www/index.html"
		Result: false (doesn't end with .py)

	Parameters:
		path:     Absolute filesystem path to the requested resource
		location: Location configuration to check

	Returns:
		true if this request should be handled by CGI
*/
bool CGI::isCgiRequest(const std::string& path, const LocationConfig& location)
{
	// Step 1: Check if CGI is configured for this location
	// Both extension and interpreter path must be set
	if (location.cgi_extension.empty() || location.cgi_path.empty())
	{
		return false;
	}

	// Step 2: Check if path ends with the CGI extension
	const std::string& ext = location.cgi_extension;

	// Path must be at least as long as the extension
	if (path.length() < ext.length())
	{
		return false;
	}

	// Compare the end of path with the extension
	// Example: path = "/cgi-bin/test.py", ext = ".py"
	//          Compare last 3 chars of path with ".py"
	size_t pathLen = path.length();
	size_t extLen = ext.length();

	return (path.compare(pathLen - extLen, extLen, ext) == 0);
}


// =========================================
//  Setup & Validation
// =========================================

/*
	setup() - Prepare everything needed for CGI execution

	This function performs all validation and preparation:
	1. Store and validate the script path
	2. Get and validate the interpreter path from location config
	3. Extract working directory from script path
	4. Build all environment variables

	After successful setup():
	- isReady() returns true
	- getEnvArray() returns valid environment
	- getArgv() returns valid arguments
	- getWorkingDirectory() returns directory to chdir() to

	Parameters:
		scriptPath: Absolute filesystem path to the CGI script

	Returns:
		true if everything is valid and ready
		false if any validation fails (check getErrorCode())
*/
bool CGI::setup(const std::string& scriptPath)
{
	// Reset state
	_ready = false;
	_errorCode = 0;
	_errorMessage.clear();
	_envVars.clear();

	// Validate we have required context
	if (!_request || !_location)
	{
		_errorCode = 500;
		_errorMessage = "CGI not properly initialized (missing request or location)";
		return false;
	}

	// =========================================
	//  Step 1: Validate the CGI script
	// =========================================
	/*
		The script must:
		1. Exist on the filesystem
		2. Be a regular file (not a directory or special file)
		3. Have execute permission

		Common errors:
		- 404 if script doesn't exist
		- 403 if script isn't executable
	*/
	if (!validateScript(scriptPath))
	{
		// Error code and message set by validateScript()
		return false;
	}
	_scriptPath = scriptPath;

	// =========================================
	//  Step 2: Validate the CGI interpreter
	// =========================================
	/*
		The interpreter is the program that runs the script:
		- Python scripts: /usr/bin/python3
		- PHP scripts: /usr/bin/php-cgi
		- Perl scripts: /usr/bin/perl

		The interpreter must exist and be executable.
	*/
	_interpreterPath = _location->cgi_path;
	if (!validateInterpreter(_interpreterPath))
	{
		// Error code and message set by validateInterpreter()
		return false;
	}

	// =========================================
	//  Step 3: Determine working directory
	// =========================================
	/*
		CGI scripts should run in their own directory so that
		relative file paths work correctly.

		Example:
			Script: /var/www/cgi-bin/process.py
			Working dir: /var/www/cgi-bin/

		If the script does: open("data.txt")
		It will look for: /var/www/cgi-bin/data.txt
	*/
	size_t lastSlash = _scriptPath.rfind('/');
	if (lastSlash != std::string::npos)
	{
		_workingDirectory = _scriptPath.substr(0, lastSlash);
	}
	else
	{
		_workingDirectory = ".";
	}

	// =========================================
	//  Step 4: Extract PATH_INFO
	// =========================================
	/*
		PATH_INFO is extra path information after the script name.
		This allows scripts to handle virtual paths.

		Example:
			Config: location /cgi-bin { cgi_extension .py; }
			Request: /cgi-bin/api.py/users/123
			SCRIPT_NAME: /cgi-bin/api.py
			PATH_INFO: /users/123

		The script can use PATH_INFO to route internally.
	*/
	_pathInfo = extractPathInfo();

	// =========================================
	//  Step 5: Build environment variables
	// =========================================
	buildEnvironment();

	// =========================================
	//  Success!
	// =========================================
	_ready = true;
	return true;
}

/*
	validateScript() - Check if CGI script exists and is executable

	Uses stat() to check file properties:
	- S_ISREG: Is it a regular file?
	- S_IXUSR, S_IXGRP, S_IXOTH: Execute permissions

	Returns:
		true if script is valid
		false with error code/message set on failure
*/
bool CGI::validateScript(const std::string& path)
{
	struct stat st;

	// Check if file exists and get info
	if (stat(path.c_str(), &st) != 0)
	{
		// stat() failed - file doesn't exist or can't be accessed
		_errorCode = 404;
		_errorMessage = "CGI script not found: " + path;
		return false;
	}

	// Check if it's a regular file (not directory, symlink, etc.)
	if (!S_ISREG(st.st_mode))
	{
		_errorCode = 403;
		_errorMessage = "CGI path is not a regular file: " + path;
		return false;
	}

	// Check execute permission
	// We check if ANY execute bit is set (user, group, or other)
	// In production, we might want stricter checks
	if (!(st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)))
	{
		_errorCode = 403;
		_errorMessage = "CGI script is not executable: " + path;
		return false;
	}

	return true;
}


/*
	validateInterpreter() - Check if CGI interpreter is executable

	The interpreter (python3, php-cgi, etc.) must:
	1. Exist on the filesystem
	2. Be executable

	Note: We use access() with X_OK to check executability.
	This is simpler than stat() for this purpose.
*/
bool CGI::validateInterpreter(const std::string& path)
{
	// Check if interpreter exists and is executable
	if (access(path.c_str(), X_OK) != 0)
	{
		_errorCode = 500;
		_errorMessage = "CGI interpreter not found or not executable: " + path;
		return false;
	}

	return true;
}


/*
	extractPathInfo() - Extract extra path information from request

	PATH_INFO is the portion of the URL path that comes after the script name.

	Algorithm:
	1. Get the request path
	2. Find where the CGI extension ends
	3. Everything after that is PATH_INFO

	Example:
		Request: /cgi-bin/app.py/api/users/123
		Extension: .py
		Result: /api/users/123
*/
std::string CGI::extractPathInfo() const
{
	if (!_request || !_location)
	{
		return "";
	}

	std::string requestPath = _request->getPath();
	const std::string& ext = _location->cgi_extension;

	// Find the extension in the path
	size_t extPos = requestPath.find(ext);
	if (extPos == std::string::npos)
	{
		return "";
	}

	// Calculate where extension ends
	size_t pathInfoStart = extPos + ext.length();

	// If there's nothing after the extension, PATH_INFO is empty
	if (pathInfoStart >= requestPath.length())
	{
		return "";
	}

	// Everything after the extension is PATH_INFO
	return requestPath.substr(pathInfoStart);
}


// =========================================
//  Environment Building
// =========================================

/*
	buildEnvironment() - Construct all CGI environment variables

	This is the heart of CGI setup. We create all the environment
	variables that the CGI script will receive.

	The CGI/1.1 specification (RFC 3875) defines required and optional
	variables. We implement all commonly used ones.
*/
void CGI::buildEnvironment()
{
	// Clear any existing environment
	_envVars.clear();

	// =========================================
	//  Request Information
	// =========================================

	/*
		REQUEST_METHOD (Required)
		The HTTP method used for the request.
		Values: "GET", "POST", "DELETE", etc.

		Script usage:
			if os.environ['REQUEST_METHOD'] == 'POST':
				# Read request body from stdin
	*/
	_envVars["REQUEST_METHOD"] = _request->getMethod();

	/*
		QUERY_STRING (Required)
		URL-encoded query parameters after the '?' in the URL.
		May be empty string if no query string.

		Example:
			URL: /search.py?q=hello&page=1
			QUERY_STRING: "q=hello&page=1"

		Script usage:
			import urllib.parse
			params = urllib.parse.parse_qs(os.environ['QUERY_STRING'])
			query = params.get('q', [''])[0]  # "hello"
	*/
	_envVars["QUERY_STRING"] = _request->getQueryString();

	/*
		CONTENT_TYPE (Required for POST)
		MIME type of the request body.
		Only set when request has a body (POST with Content-Type header).

		Common values:
			"application/x-www-form-urlencoded" - HTML form
			"multipart/form-data" - File upload
			"application/json" - JSON API
	*/
	std::string contentType = _request->getHeader("Content-Type");
	if (!contentType.empty())
	{
		_envVars["CONTENT_TYPE"] = contentType;
	}

	/*
		CONTENT_LENGTH (Required for POST)
		Size of request body in bytes.
		The script reads exactly this many bytes from stdin.

		Script usage:
			length = int(os.environ.get('CONTENT_LENGTH', 0))
			body = sys.stdin.read(length)
	*/
	std::string contentLength = _request->getHeader("Content-Length");
	if (!contentLength.empty())
	{
		_envVars["CONTENT_LENGTH"] = contentLength;
	}
	else if (_request->getBodySize() > 0)
	{
		// If no Content-Length header but we have a body (chunked transfer),
		// set CONTENT_LENGTH from actual body size
		std::stringstream ss;
		ss << _request->getBodySize();
		_envVars["CONTENT_LENGTH"] = ss.str();
	}

	// =========================================
	//  Script Information
	// =========================================

	/*
		SCRIPT_NAME (Required)
		Virtual path to the CGI script (the URL path, not filesystem path).

		Example:
			URL: /cgi-bin/search.py?q=hello
			SCRIPT_NAME: "/cgi-bin/search.py"

		Used by scripts to construct self-referencing URLs.
	*/
	std::string scriptName = _request->getPath();
	// Remove PATH_INFO from script name if present
	if (!_pathInfo.empty())
	{
		size_t pathInfoPos = scriptName.find(_pathInfo);
		if (pathInfoPos != std::string::npos)
		{
			scriptName = scriptName.substr(0, pathInfoPos);
		}
	}
	_envVars["SCRIPT_NAME"] = scriptName;

	/*
		SCRIPT_FILENAME
		Absolute filesystem path to the CGI script.
		Not in original CGI spec but widely used (PHP requires it).

		Example: "/var/www/cgi-bin/search.py"
	*/
	_envVars["SCRIPT_FILENAME"] = _scriptPath;

	/*
		PATH_INFO
		Extra path information after the script name.
		Allows scripts to handle virtual paths.

		Example:
			URL: /cgi-bin/api.py/users/123
			SCRIPT_NAME: "/cgi-bin/api.py"
			PATH_INFO: "/users/123"
	*/
	_envVars["PATH_INFO"] = _pathInfo;

	/*
		PATH_TRANSLATED
		PATH_INFO translated to a filesystem path.
		Maps the virtual path to the document root.

		Example:
			PATH_INFO: "/users/123"
			Document root: "/var/www"
			PATH_TRANSLATED: "/var/www/users/123"

		Note: We use the location's root for translation.
	*/
	if (!_pathInfo.empty() && !_location->root.empty())
	{
		std::string translated = _location->root;
		if (!translated.empty() && translated[translated.length() - 1] == '/')
		{
			translated = translated.substr(0, translated.length() - 1);
		}
		translated += _pathInfo;
		_envVars["PATH_TRANSLATED"] = translated;
	}

	// =========================================
	//  Protocol Information
	// =========================================

	/*
		SERVER_PROTOCOL (Required)
		The HTTP version used in the request.
		Values: "HTTP/1.0" or "HTTP/1.1"
	*/
	_envVars["SERVER_PROTOCOL"] = _request->getHttpVersion();

	/*
		GATEWAY_INTERFACE (Required)
		CGI specification version.
		Always "CGI/1.1" for modern CGI.
	*/
	_envVars["GATEWAY_INTERFACE"] = "CGI/1.1";

	/*
		REQUEST_URI
		Full request URI including query string.
		Not in CGI spec but commonly provided.

		Example: "/search.py?q=hello"
	*/
	_envVars["REQUEST_URI"] = _request->getUri();

	// =========================================
	//  Server Information
	// =========================================

	/*
		SERVER_SOFTWARE
		Name and version of the server software.
	*/
	_envVars["SERVER_SOFTWARE"] = "webserv/1.0";

	/*
		SERVER_NAME
		Hostname of the server.
		Taken from Host header or config.
	*/
	std::string host = _request->getHeader("Host");
	if (!host.empty())
	{
		// Remove port if present (Host: localhost:8080 -> localhost)
		size_t colonPos = host.find(':');
		if (colonPos != std::string::npos)
		{
			_envVars["SERVER_NAME"] = host.substr(0, colonPos);
		}
		else
		{
			_envVars["SERVER_NAME"] = host;
		}
	}
	else
	{
		_envVars["SERVER_NAME"] = "localhost";
	}

	/*
		SERVER_PORT
		Port number the server is listening on.
		Extracted from Host header.
	*/
	if (!host.empty())
	{
		size_t colonPos = host.find(':');
		if (colonPos != std::string::npos)
		{
			_envVars["SERVER_PORT"] = host.substr(colonPos + 1);
		}
		else
		{
			_envVars["SERVER_PORT"] = "80";  // Default HTTP port
		}
	}
	else
	{
		_envVars["SERVER_PORT"] = "80";
	}

	// =========================================
	//  Client Information
	// =========================================

	/*
		REMOTE_ADDR
		IP address of the client.
		TODO: This should come from the connection, not the request.
		For now, we use a placeholder.
	*/
	_envVars["REMOTE_ADDR"] = "127.0.0.1";  // Placeholder

	/*
		REMOTE_HOST
		Hostname of the client (if available via reverse DNS).
		Often same as REMOTE_ADDR if DNS lookup not done.
	*/
	_envVars["REMOTE_HOST"] = "127.0.0.1";  // Placeholder

	// =========================================
	//  HTTP Headers as Environment Variables
	// =========================================
	/*
		All HTTP headers are converted to environment variables
		with HTTP_ prefix. This allows scripts to access any header.

		Conversion rules:
		1. Add "HTTP_" prefix
		2. Convert to uppercase
		3. Replace "-" with "_"

		Examples:
			Host: localhost           -> HTTP_HOST=localhost
			User-Agent: Mozilla/5.0   -> HTTP_USER_AGENT=Mozilla/5.0
			Accept-Language: en-US    -> HTTP_ACCEPT_LANGUAGE=en-US
			X-Custom-Header: value    -> HTTP_X_CUSTOM_HEADER=value
	*/
	addHttpHeaders();

	// =========================================
	//  Document Root (bonus, not standard)
	// =========================================
	/*
		DOCUMENT_ROOT
		Filesystem path to the document root.
		Used by some scripts (especially PHP) for include paths.
	*/
	if (!_location->root.empty())
	{
		_envVars["DOCUMENT_ROOT"] = _location->root;
	}

	/*
		REDIRECT_STATUS
		Required by PHP-CGI for security reasons.
		Must be "200" for the script to execute.
	*/
	_envVars["REDIRECT_STATUS"] = "200";
}


/*
	addHttpHeaders() - Convert all HTTP headers to CGI environment format

	This allows CGI scripts to access any header sent by the client.
	Headers are prefixed with HTTP_ and transformed to uppercase with
	underscores.
*/
void CGI::addHttpHeaders()
{
	const std::map<std::string, std::string>& headers = _request->getHeaders();

	for (std::map<std::string, std::string>::const_iterator it = headers.begin();
		 it != headers.end(); ++it)
	{
		std::string name = it->first;
		const std::string& value = it->second;

		/*
			Skip headers that are handled specially:
			- Content-Type becomes CONTENT_TYPE (no HTTP_ prefix)
			- Content-Length becomes CONTENT_LENGTH (no HTTP_ prefix)
		*/
		std::string lowerName = name;
		for (size_t i = 0; i < lowerName.length(); ++i)
		{
			if (lowerName[i] >= 'A' && lowerName[i] <= 'Z')
			{
				lowerName[i] = lowerName[i] + 32;
			}
		}

		if (lowerName == "content-type" || lowerName == "content-length")
		{
			continue;  // Already handled above without HTTP_ prefix
		}

		/*
			Transform header name:
			1. Convert to uppercase
			2. Replace '-' with '_'
			3. Add HTTP_ prefix

			Example: "Accept-Language" -> "HTTP_ACCEPT_LANGUAGE"
		*/
		std::string envName = "HTTP_";
		for (size_t i = 0; i < name.length(); ++i)
		{
			char c = name[i];
			if (c >= 'a' && c <= 'z')
			{
				// Convert to uppercase
				envName += static_cast<char>(c - 32);
			}
			else if (c == '-')
			{
				// Replace dash with underscore
				envName += '_';
			}
			else
			{
				envName += c;
			}
		}

		_envVars[envName] = value;
	}
}


// =========================================
//  Environment Array for execve()
// =========================================

/*
	getEnvArray() - Convert environment map to char** for execve()

	execve() requires environment as:
		char* envp[] = {
			"REQUEST_METHOD=GET",
			"QUERY_STRING=name=World",
			"SERVER_PROTOCOL=HTTP/1.1",
			NULL  // Null terminator required
		};

	Memory layout:
		- Array of char* pointers
		- Each pointer points to a C string "NAME=VALUE"
		- Last pointer is NULL

	IMPORTANT: Caller must free with freeEnvArray()!
*/
char** CGI::getEnvArray() const
{
	// Calculate array size: number of variables + 1 for NULL terminator
	size_t count = _envVars.size();

	// Allocate array of pointers
	// Using malloc for C compatibility with execve
	char** envArray = static_cast<char**>(malloc(sizeof(char*) * (count + 1)));
	if (!envArray)
	{
		return NULL;  // Allocation failed
	}

	// Fill array with "NAME=VALUE" strings
	size_t i = 0;
	for (std::map<std::string, std::string>::const_iterator it = _envVars.begin();
		 it != _envVars.end(); ++it, ++i)
	{
		// Create "NAME=VALUE" string
		std::string envStr = it->first + "=" + it->second;

		// Allocate and copy the string
		envArray[i] = static_cast<char*>(malloc(envStr.length() + 1));
		if (!envArray[i])
		{
			// Cleanup on failure
			for (size_t j = 0; j < i; ++j)
			{
				free(envArray[j]);
			}
			free(envArray);
			return NULL;
		}

		// Copy string (including null terminator)
		std::strcpy(envArray[i], envStr.c_str());
	}

	// Null terminate the array
	envArray[count] = NULL;

	return envArray;
}


/*
	freeEnvArray() - Free memory allocated by getEnvArray()

	Must be called:
	- In parent process after fork()
	- If execve() fails in child process (before exit)
*/
void CGI::freeEnvArray(char** envArray)
{
	if (!envArray)
	{
		return;
	}

	// Free each string
	for (size_t i = 0; envArray[i] != NULL; ++i)
	{
		free(envArray[i]);
	}

	// Free the array itself
	free(envArray);
}


// =========================================
//  Argument Array for execve()
// =========================================

/*
	getArgv() - Create argument array for execve()

	For CGI, the arguments are:
		argv[0] = interpreter path (e.g., "/usr/bin/python3")
		argv[1] = script path (e.g., "/var/www/cgi-bin/test.py")
		argv[2] = NULL (terminator)

	This tells execve to run:
		/usr/bin/python3 /var/www/cgi-bin/test.py

	Note: Some CGI implementations pass additional arguments.
	We keep it simple with just interpreter and script.
*/
char** CGI::getArgv() const
{
	// Allocate array: interpreter + script + NULL
	char** argv = static_cast<char**>(malloc(sizeof(char*) * 3));
	if (!argv)
	{
		return NULL;
	}

	// argv[0]: Interpreter path
	argv[0] = static_cast<char*>(malloc(_interpreterPath.length() + 1));
	if (!argv[0])
	{
		free(argv);
		return NULL;
	}
	std::strcpy(argv[0], _interpreterPath.c_str());

	// argv[1]: Script path
	argv[1] = static_cast<char*>(malloc(_scriptPath.length() + 1));
	if (!argv[1])
	{
		free(argv[0]);
		free(argv);
		return NULL;
	}
	std::strcpy(argv[1], _scriptPath.c_str());

	// argv[2]: NULL terminator
	argv[2] = NULL;

	return argv;
}


/*
	freeArgv() - Free memory allocated by getArgv()
*/
void CGI::freeArgv(char** argv)
{
	if (!argv)
	{
		return;
	}

	// Free each string (we know there are exactly 2)
	if (argv[0]) free(argv[0]);
	if (argv[1]) free(argv[1]);

	// Free the array
	free(argv);
}




// =========================================
//  Getters
// =========================================

const std::string& CGI::getScriptPath() const
{
	return _scriptPath;
}

const std::string& CGI::getInterpreterPath() const
{
	return _interpreterPath;
}

const std::string& CGI::getWorkingDirectory() const
{
	return _workingDirectory;
}

int CGI::getErrorCode() const
{
	return _errorCode;
}

const std::string& CGI::getErrorMessage() const
{
	return _errorMessage;
}

bool CGI::isReady() const
{
	return _ready;
}

const std::string& CGI::getRequestBody() const
{
	static std::string empty;
	if (_request)
	{
		return _request->getBody();
	}
	return empty;
}

const std::map<std::string, std::string>& CGI::getEnvMap() const
{
	return _envVars;
}

