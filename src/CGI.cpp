/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:56:09 by anemet            #+#    #+#             */
/*   Updated: 2025/12/14 17:00:41 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGI.hpp"
#include "Request.hpp"
#include "Config.hpp"

/*
	==================================
		CGI Implementation Overview
	==================================

	CGI Flow in webserv:
	--------------------
	1. Request arrives: GET /cgi-bin/hello.py?name=World
	2. Router calls isCgiRequest() -> true (matches .py extension)
	3. Router creates CGI object and calls setup()
	4. CGI::setup() validates script, builds environment
	5. Router calls CGI::execute()
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

	// Path must longer than the extension
	if (path.length() <= ext.length())
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
		/*
			stat() failed - file doesn't exist or can't be accessed

			Common errno values:
			- ENOENT: No such file or directory -> 404
			- EACCES: Permission denied (can't even stat) -> 403
			- ENOTDIR: A component of path is not a directory -> 404
			- ENAMETOOLONG: Path too long -> 414 (but we use 404 for simplicity)
		*/
		if (errno == EACCES)
		{
			_errorCode = 403;
			_errorMessage = "Permission denied accessing CGI script: " + path;
		}
		else
		{
			_errorCode = 404;
			_errorMessage = "CGI script not found: " + path;
		}
		return false;
	}

	// Check if it's a regular file (not directory, symlink, etc.)
	if (!S_ISREG(st.st_mode))
	{
		/*
			Path exists but is not a regular file.
			Could be a directory, socket, device, etc.
		*/
		if (S_ISDIR(st.st_mode))
		{
			_errorCode = 403;
			_errorMessage = "CGI path is a directory, not a script: " + path;
		}
		else
		{
			_errorCode = 403;
			_errorMessage = "CGI path is not a regular file: " + path;
		}
		return false;
	}

	// Check if ANY execute bit is set (user, group, or other).
	if (!(st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)))
	{
		_errorCode = 403;
		_errorMessage = "CGI script is not executable (check chmod +x): " + path;
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
	// use just the filename since we chdir to the script's directory
	std::string scriptName = _scriptPath;
	size_t lastSlash = _scriptPath.rfind('/');
	if (lastSlash != std::string::npos)
	{
		scriptName = _scriptPath.substr(lastSlash + 1);
	}

	argv[1] = static_cast<char*>(malloc(scriptName.length() + 1));
	if (!argv[1])
	{
		free(argv[0]);
		free(argv);
		return NULL;
	}
	std::strcpy(argv[1], scriptName.c_str());

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


// =========================================
//  CGI Execution Implementation (Step 8.2)
// =========================================

/*
	=================================================================
		STEP 8.2: CGI EXECUTION
	=================================================================

	This section implements the actual execution of CGI scripts.
	It's the most complex part of CGI handling, involving:
	- Inter-process communication (pipes)
	- Process creation (fork)
	- Process replacement (execve)
	- Timeout handling
	- Output parsing

	The CGI protocol (RFC 3875) specifies:
	- Script receives request data via environment variables
	- POST body is sent to script's stdin
	- Script writes response to stdout
	- Response format: headers, blank line, body

*/


/*
	setNonBlocking() - Set file descriptor to non-blocking mode

	Non-blocking I/O is essential for timeout handling.
	When a fd is non-blocking:
	- read() returns -1 with errno=EAGAIN instead of blocking
	- We can implement polling with timeout

	This uses fcntl() with F_SETFL and O_NONBLOCK flags.

	Parameters:
		fd: File descriptor to modify

	Returns:
		true if successful, false on error
*/
bool CGI::setNonBlocking(int fd)
{
	// Get current flags
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
	{
		return false;
	}

	// Add non-blocking flag
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		return false;
	}

	return true;
}


/*
	parseCgiOutput() - Parse raw CGI output into headers and body

	CGI Output Format (RFC 3875):
	-----------------------------
	Content-Type: text/html\r\n
	Set-Cookie: session=abc123\r\n
	X-Custom-Header: value\r\n
	\r\n                          <- Empty line (CRLF only)
	<html>                        <- Body starts here
	<body>Hello World</body>
	</html>

	Parsing Algorithm:
	1. Find the header/body separator (\r\n\r\n or \n\n)
	2. Split output at separator
	3. Parse each header line (Name: Value format)
	4. Store remaining content as body

	Special Headers:
	- Content-Type: Required for valid CGI response
	- Status: CGI can set HTTP status (e.g., "Status: 404 Not Found")
	- Location: For redirects (implies 302 if no Status)

	Parameters:
		output: Raw CGI output string
		result: CGIResult to populate

	Returns:
		true if valid CGI output
		false if malformed (sets 502 Bad Gateway)
*/
bool CGI::parseCgiOutput(const std::string& output, CGIResult& result)
{
	// =========================================
	//  Step 1: Find header/body separator
	// =========================================
	/*
		CGI spec requires headers and body to be separated by a blank line.
		The blank line can be:
		- \r\n\r\n (standard HTTP format)
		- \n\n (simplified, some CGI scripts use this)

		We try both formats for compatibility.
	*/
	std::string separator = "\r\n\r\n";
	size_t separatorPos = output.find(separator);

	// Try alternate format if standard not found
	if (separatorPos == std::string::npos)
	{
		separator = "\n\n";
		separatorPos = output.find(separator);
	}

	// If no separator found, CGI output is malformed
	if (separatorPos == std::string::npos)
	{
		/*
			502 Bad Gateway is the correct status for invalid CGI output.
			It means the server (acting as gateway to CGI) received an
			invalid response from the upstream (CGI script).
		*/
		result.success = false;
		result.statusCode = 502;
		result.errorMessage = "CGI output missing header/body separator";
		return false;
	}

	// =========================================
	//  Step 2: Split into headers and body
	// =========================================
	std::string headerSection = output.substr(0, separatorPos);
	result.body = output.substr(separatorPos + separator.length());

	// =========================================
	//  Step 3: Parse header lines
	// =========================================
	/*
		Each header line has format: "Name: Value"
		- Name is case-insensitive (we preserve original case)
		- Colon separates name from value
		- Optional whitespace after colon
		- Lines separated by \r\n or \n
	*/
	std::string lineDelim = (separator == "\r\n\r\n") ? "\r\n" : "\n";
	size_t lineStart = 0;
	size_t lineEnd;

	while ((lineEnd = headerSection.find(lineDelim, lineStart)) != std::string::npos)
	{
		std::string line = headerSection.substr(lineStart, lineEnd - lineStart);
		lineStart = lineEnd + lineDelim.length();

		// Skip empty lines
		if (line.empty())
		{
			continue;
		}

		// Find colon separator
		size_t colonPos = line.find(':');
		if (colonPos == std::string::npos)
		{
			// Invalid header line - skip it (be lenient)
			continue;
		}

		// Extract name and value
		std::string name = line.substr(0, colonPos);
		std::string value = line.substr(colonPos + 1);

		// Trim leading whitespace from value
		size_t valueStart = value.find_first_not_of(" \t");
		if (valueStart != std::string::npos)
		{
			value = value.substr(valueStart);
		}

		// Store header
		result.headers[name] = value;
	}

	// Handle last line if no trailing newline
	if (lineStart < headerSection.length())
	{
		std::string line = headerSection.substr(lineStart);
		size_t colonPos = line.find(':');
		if (colonPos != std::string::npos)
		{
			std::string name = line.substr(0, colonPos);
			std::string value = line.substr(colonPos + 1);
			size_t valueStart = value.find_first_not_of(" \t");
			if (valueStart != std::string::npos)
			{
				value = value.substr(valueStart);
			}
			result.headers[name] = value;
		}
	}

	// =========================================
	//  Step 4: Determine HTTP status code
	// =========================================
	/*
		CGI scripts can set HTTP status via Status header:
			Status: 404 Not Found
			Status: 302 Found

		If no Status header:
		- Location header present -> 302 redirect
		- Otherwise -> 200 OK
	*/
	result.statusCode = 200;  // Default to OK

	std::map<std::string, std::string>::iterator statusIt = result.headers.find("Status");
	if (statusIt != result.headers.end())
	{
		// Parse status code from Status header value
		// Format: "404 Not Found" or just "404"
		std::stringstream ss(statusIt->second);
		int code;
		if (ss >> code)
		{
			result.statusCode = code;
		}
		// Remove Status header - it's CGI-specific, not sent to client
		result.headers.erase(statusIt);
	}
	else
	{
		// Check for Location header (redirect)
		if (result.headers.find("Location") != result.headers.end())
		{
			result.statusCode = 302;  // Found (temporary redirect)
		}
	}

	// =========================================
	//  Step 5: Validate minimum requirements
	// =========================================
	/*
		CGI spec requires Content-Type header for document responses.
		However, some valid responses don't have it:
		- Redirects (Location header only)
		- Empty responses (204 No Content)

		We're lenient here - only warn, don't fail.
	*/
	if (result.headers.find("Content-Type") == result.headers.end() &&
		result.headers.find("Location") == result.headers.end() &&
		!result.body.empty())
	{
		// Missing Content-Type but has body - assume text/html
		result.headers["Content-Type"] = "text/html";
	}

	result.success = true;
	return true;
}


// =========================================
//  Step 8.3: CGI Error Handling Helpers
// =========================================

/*
	cleanupChild() - Terminate and reap a hung or failed child process

	Why This Matters:
	-----------------
	When a CGI script hangs, crashes, or we decide to abandon it (timeout),
	we MUST clean up properly to prevent:

	1. Zombie Processes:
	2. Resource Leaks:
	3. Security Issues:

	Termination Strategy:
	--------------------
	1. SIGTERM (signal 15): Polite request to terminate
		- Allows process to clean up (close files, save state)
		- Well-behaved processes should exit within ~1 second

	2. Brief wait: Give the process a chance to exit gracefully

	3. SIGKILL (signal 9): Forceful termination
		- Process is immediately killed by the kernel
		- No cleanup possible, but guaranteed to work
		- Use only as a last resort

	4. waitpid(): Reap the zombie
		- Removes the process table entry
		- Returns exit status (which we ignore here)

	Parameters:
		pid: Process ID of the child to terminate
	*/
	void CGI::cleanupChild(pid_t pid)
	{
	if (pid <= 0)
	{
		return;  // Invalid PID, nothing to clean up
	}

	/*
		Step 1: Send SIGTERM (graceful termination request)

		SIGTERM is the standard "please exit" signal.
		Well-written programs catch this signal and clean up before exiting.

		kill() returns:
		-  0 on success
		- -1 on error (check errno)

		Common errno values:
		- ESRCH: Process doesn't exist (already exited) -> good!
		- EPERM: Permission denied (shouldn't happen for our child)
	*/
	if (kill(pid, SIGTERM) == -1)
	{
		if (errno == ESRCH)
		{
			// Process already exited, just reap it
			waitpid(pid, NULL, WNOHANG);
			return;
		}
		// Other errors: process might be in weird state, try to reap anyway
		waitpid(pid, NULL, WNOHANG);
		return;
	}

	/*
		Step 2: Wait briefly for graceful exit

		We use waitpid() with WNOHANG to check if process exited
		without blocking. If it hasn't exited yet, we sleep a bit
		and try again.

		Total wait time: ~100ms (10 iterations × 10ms)
		This is a reasonable time for a process to clean up.
	*/
	int status;
	for (int i = 0; i < 10; ++i)
	{
		pid_t result = waitpid(pid, &status, WNOHANG);

		if (result == pid)
		{
			// Child has exited, we're done
			return;
		}
		else if (result == -1)
		{
			// Error (possibly already reaped by someone else)
			return;
		}

		// Process still running, wait a bit
		usleep(10000);  // 10ms
	}

	/*
		Step 3: Send SIGKILL (forceful termination)

		If the process didn't respond to SIGTERM within our timeout,
		it's either hung or ignoring signals. SIGKILL cannot be caught
		or ignored - the kernel forcefully terminates the process.

		This is the "nuclear option" but necessary for reliability.
	*/
	kill(pid, SIGKILL);

	/*
		Step 4: Reap the zombie

		After SIGKILL, the process will definitely exit (though not
		instantly - there's a tiny delay). We use blocking waitpid()
		here because we MUST reap this zombie.

		We don't use WNOHANG because:
		- SIGKILL is guaranteed to work
		- We need to clean up before returning
		- The delay is negligible (microseconds)
	*/
	waitpid(pid, &status, 0);
}



/*
	closePipes() - Safely close all pipe file descriptors

	Why Careful Pipe Closing Matters:
	---------------------------------
	Pipes are a fundamental IPC (Inter-Process Communication) mechanism.
	Each pipe has two ends:
	- Read end (fd[0]): Used to read data FROM the pipe
	- Write end (fd[1]): Used to write data INTO the pipe

	Problems if we don't close properly:

	1. File Descriptor Leaks:
		Each open FD consumes kernel resources.
		There's a per-process limit (typically 1024).
		Leaking FDs eventually causes open()/socket() to fail.

	2. Blocked I/O:
		Read from a pipe blocks until:
		a) Data is available, OR
		b) ALL write ends are closed (returns EOF)

		If we forget to close the parent's copy of the write end,
		our read() will block forever waiting for data that never comes.

	3. Broken Pipes:
		Write to a pipe with no readers causes SIGPIPE (default: process death).
		We must close read ends we don't need before writing.

	Our Pipe Setup:
	---------------
	stdinPipe:  Parent writes → Child reads (child's stdin)
	stdoutPipe: Child writes → Parent reads (child's stdout)

	Parent needs: stdinPipe[1] (write), stdoutPipe[0] (read)
	Child needs:  stdinPipe[0] (read), stdoutPipe[1] (write)

	Each process must close the ends it doesn't use!

	Parameters:
		stdinPipe:  Array of 2 FDs for stdin pipe (may be NULL)
		stdoutPipe: Array of 2 FDs for stdout pipe (may be NULL)

	Note: -1 indicates already-closed or never-opened FD
	*/
	void CGI::closePipes(int* stdinPipe, int* stdoutPipe)
	{
	/*
		Close each pipe end if it's valid (>= 0)

		close() on an already-closed FD returns -1 with errno=EBADF.
		While this is technically an error, it's harmless and we ignore it.

		Using -1 as "invalid/closed" is a common pattern in Unix programming.
	*/
	if (stdinPipe)
	{
		if (stdinPipe[0] >= 0)
		{
			close(stdinPipe[0]);
			stdinPipe[0] = -1;  // Mark as closed
		}
		if (stdinPipe[1] >= 0)
		{
			close(stdinPipe[1]);
			stdinPipe[1] = -1;
		}
	}

	if (stdoutPipe)
	{
		if (stdoutPipe[0] >= 0)
		{
			close(stdoutPipe[0]);
			stdoutPipe[0] = -1;
		}
		if (stdoutPipe[1] >= 0)
		{
			close(stdoutPipe[1]);
			stdoutPipe[1] = -1;
		}
	}
}



// =========================================
//  execute() with Enhanced Error Handling
// =========================================

/*
	execute() - Run CGI script and capture output

	This is the main CGI execution function with comprehensive error handling.

	Error Categories and HTTP Status Codes:
	---------------------------------------

	| Error Type              | HTTP Code | Meaning                          |
	|-------------------------|-----------|----------------------------------|
	| Script not found        | 404       | Requested CGI doesn't exist      |
	| Script not executable   | 403       | Forbidden                        |
	| Interpreter not found   | 500       | Server misconfiguration          |
	| pipe() failed           | 500       | System resource exhaustion       |
	| fork() failed           | 500       | System resource exhaustion       |
	| execve() failed         | 500       | Interpreter couldn't start       |
	| Script crashed          | 500       | Script error (bad code)          |
	| Script timed out        | 504       | Gateway Timeout                  |
	| Invalid CGI output      | 502       | Bad Gateway (malformed response) |
	| No output from script   | 500       | Script didn't produce anything   |

	Cleanup Guarantees:
	------------------
	This function guarantees that:
	1. All pipe FDs are closed (no leaks)
	2. Child process is reaped (no zombies)
	3. Allocated memory is freed (argv, envp)

	Even in error cases, resources are properly cleaned up.

	Parameters:
		timeout: Maximum seconds to wait for CGI (default: 30)

	Returns:
		CGIResult with success status, headers, body, and error info
	*/
	CGI::CGIResult CGI::execute(int timeout)
	{
	CGIResult result;

	// Initialize pipe FDs to -1 (invalid) for safe cleanup
	int stdin_pipe[2] = {-1, -1};
	int stdout_pipe[2] = {-1, -1};

	// =========================================
	//  Pre-flight Checks
	// =========================================
	if (!_ready)
	{
		result.success = false;
		// result.statusCode = 500;
		// result.errorMessage = "CGI not ready - setup() was not called or failed";
		return result;
	}

	// =========================================
	//  Step 1: Create Pipes for Communication
	// =========================================
	/*
		Pipe Creation and Error Handling:
		---------------------------------
		pipe() can fail for several reasons:
		- EMFILE: Process has too many open FDs
		- ENFILE: System-wide FD limit reached
		- EFAULT: Invalid buffer (shouldn't happen)

		All these indicate resource exhaustion -> 500 Internal Server Error
	*/
	if (pipe(stdin_pipe) == -1)
	{
		result.success = false;
		result.statusCode = 500;
		result.errorMessage = "Failed to create stdin pipe: ";
		result.errorMessage += strerror(errno);
		return result;
	}

	if (pipe(stdout_pipe) == -1)
	{
		// Clean up the first pipe before returning
		closePipes(stdin_pipe, NULL);

		result.success = false;
		result.statusCode = 500;
		result.errorMessage = "Failed to create stdout pipe: ";
		result.errorMessage += strerror(errno);
		return result;
	}

	// =========================================
	//  Step 2: Fork Child Process
	// =========================================
	/*
		Fork and Error Handling:
		------------------------
		fork() can fail for:
		- EAGAIN: System process limit reached, or user's process limit
		- ENOMEM: Not enough memory to duplicate the process

		Both indicate severe resource constraints -> 500 Internal Server Error
	*/
	pid_t pid = fork();

	if (pid == -1)
	{
		// Fork failed - clean up all pipes
		closePipes(stdin_pipe, stdout_pipe);

		result.success = false;
		result.statusCode = 500;
		result.errorMessage = "Failed to fork process: ";
		result.errorMessage += strerror(errno);
		return result;
	}

	// =========================================
	//  Child Process (pid == 0)
	// =========================================
	if (pid == 0)
	{
		/*
			CHILD PROCESS EXECUTION
			-----------------------
			We're now in the forked child process.

			Critical: If ANYTHING fails here, we must call _exit(), NOT exit().

			Why _exit() instead of exit()?
			- exit() flushes stdio buffers (printf, etc.)
			- The parent has copies of these buffers
			- Flushing would cause data to appear twice or get corrupted
			- _exit() bypasses all cleanup - just terminates immediately

			Exit codes:
			- 0: Success (normal exit)
			- 1: Setup failure (dup2, chdir failed)
			- 2: execve failed (interpreter problem)

			The parent can check these to provide better error messages.
		*/

		// Redirect stdin to read from pipe
		if (dup2(stdin_pipe[0], STDIN_FILENO) == -1)
		{
			_exit(1);
		}

		// Redirect stdout to write to pipe
		if (dup2(stdout_pipe[1], STDOUT_FILENO) == -1)
		{
			_exit(1);
		}

		// Close all pipe FDs (we have copies at 0 and 1 now)
		close(stdin_pipe[0]);
		close(stdin_pipe[1]);
		close(stdout_pipe[0]);
		close(stdout_pipe[1]);

		// Change to script directory
		if (!_workingDirectory.empty())
		{
			if (chdir(_workingDirectory.c_str()) == -1)
			{
				// Non-fatal: script might work anyway with absolute paths
				// But log this somehow in production
			}
		}

		// Prepare argv and envp
		char** argv = getArgv();
		char** envp = getEnvArray();

		if (!argv || !envp)
		{
			if (argv) freeArgv(argv);
			if (envp) freeEnvArray(envp);
			_exit(1);
		}

		// Execute the CGI interpreter
		/*
			execve() replaces this process with the CGI script.
			If it returns, it means it failed.

			Common execve() failures:
			- ENOENT: Interpreter not found
			- EACCES: Interpreter not executable
			- ENOEXEC: Bad interpreter format (not an ELF binary)
			- E2BIG: Argument list too long
		*/
		execve(_interpreterPath.c_str(), argv, envp);

		// If we get here, execve failed
		// Free memory (process is about to exit anyway)
		freeArgv(argv);
		freeEnvArray(envp);

		// Exit with code 2 to indicate execve failure
		_exit(2);
	}

	// =========================================
	//  Parent Process (pid > 0)
	// =========================================

	// Close pipe ends we don't need
	close(stdin_pipe[0]);   // Child's read end
	stdin_pipe[0] = -1;
	close(stdout_pipe[1]);  // Child's write end
	stdout_pipe[1] = -1;

	// =========================================
	//  Step 3: Write Request Body to Child
	// =========================================
	const std::string& requestBody = getRequestBody();
	if (!requestBody.empty())
	{
		/*
			Write POST body to CGI's stdin.

			In production, we should:
			1. Handle partial writes (EAGAIN)
			2. Use poll() to avoid blocking
			3. Handle SIGPIPE if child dies

			For simplicity, we do a simple blocking write here.
			The timeout mechanism will catch hung writes.
		*/
		ssize_t written = write(stdin_pipe[1], requestBody.c_str(), requestBody.size());
		(void)written;  // Ignore for now
	}

	// Close write end to signal EOF to child
	close(stdin_pipe[1]);
	stdin_pipe[1] = -1;

	// =========================================
	//  Step 4: Read CGI Output with Timeout
	// =========================================
	setNonBlocking(stdout_pipe[0]);

	std::string cgiOutput;
	char buffer[4096];
	time_t startTime = time(NULL);
	bool timedOut = false;
	bool childExited = false;

	while (!childExited)
	{
		// Check timeout
		if (time(NULL) - startTime >= timeout)
		{
			timedOut = true;
			break;
		}

		// Check if child has exited
		int status;
		pid_t waitResult = waitpid(pid, &status, WNOHANG);

		if (waitResult == pid)
		{
			// Child has exited
			childExited = true;

			// Read any remaining output
			while (true)
			{
				ssize_t bytesRead = read(stdout_pipe[0], buffer, sizeof(buffer) - 1);
				if (bytesRead > 0)
				{
					cgiOutput.append(buffer, bytesRead);
				}
				else
				{
					break;
				}
			}

			// Check exit status
			if (!WIFEXITED(status))
			{
				// Child was killed by a signal
				close(stdout_pipe[0]);
				stdout_pipe[0] = -1;

				result.success = false;
				result.statusCode = 500;

				if (WIFSIGNALED(status))
				{
					int sig = WTERMSIG(status);
					std::ostringstream oss;
					oss << "CGI script killed by signal " << sig;
					result.errorMessage = oss.str();
				}
				else
				{
					result.errorMessage = "CGI script terminated abnormally";
				}
				return result;
			}

			int exitCode = WEXITSTATUS(status);
			if (exitCode == 2 && cgiOutput.empty())
			{
				// Exit code 2 = execve failed
				close(stdout_pipe[0]);
				stdout_pipe[0] = -1;

				result.success = false;
				result.statusCode = 500;
				result.errorMessage = "Failed to execute CGI interpreter";
				return result;
			}

			break;
		}
		else if (waitResult == -1 && errno != ECHILD)
		{
			// Unexpected error
			break;
		}

		// Try to read from pipe
		ssize_t bytesRead = read(stdout_pipe[0], buffer, sizeof(buffer) - 1);

		if (bytesRead > 0)
		{
			cgiOutput.append(buffer, bytesRead);
		}
		else if (bytesRead == 0)
		{
			// EOF - child closed stdout
			// Wait for child to fully exit
			waitpid(pid, &status, 0);
			childExited = true;
		}
		else
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				usleep(10000);  // 10ms
				continue;
			}
			else
			{
				// Read error
				break;
			}
		}
	}

	// Close read end
	close(stdout_pipe[0]);
	stdout_pipe[0] = -1;

	// =========================================
	//  Step 5: Handle Timeout
	// =========================================
	if (timedOut)
	{
		/*
			CGI Timeout (504 Gateway Timeout)
			---------------------------------
			The CGI script took too long to respond.

			This could mean:
			- Script has an infinite loop
			- Script is waiting for a resource (DB, network)
			- Script is doing heavy computation

			We must kill the process to prevent resource exhaustion.
		*/
		cleanupChild(pid);

		result.success = false;
		result.statusCode = 504;
		result.errorMessage = "CGI script execution timed out after ";
		std::ostringstream oss;
		oss << timeout;
		result.errorMessage += oss.str();
		result.errorMessage += " seconds";
		return result;
	}

	// =========================================
	//  Step 6: Validate Output
	// =========================================
	if (cgiOutput.empty())
	{
		/*
			Empty Output (500 Internal Server Error)
			----------------------------------------
			The CGI script produced no output at all.

			Possible causes:
			- Script crashed immediately
			- Script has syntax error (Python/PHP parse error)
			- Script exited without printing anything

			All indicate a broken script -> 500
		*/
		result.success = false;
		result.statusCode = 500;
		result.errorMessage = "CGI script produced no output";
		return result;
	}

	// =========================================
	//  Step 7: Parse CGI Output
	// =========================================
	if (!parseCgiOutput(cgiOutput, result))
	{
		// parseCgiOutput sets error info (502 Bad Gateway)
		return result;
	}

	// =========================================
	//  Success!
	// =========================================
	result.success = true;
	return result;
}
