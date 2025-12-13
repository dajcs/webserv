/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:57:40 by anemet            #+#    #+#             */
/*   Updated: 2025/12/12 21:57:07 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_HPP
#define CGI_HPP


/*
	==============================================
		CGI (Common Gateway Interface) Overview
	==============================================

	What is CGI?
	------------
	CGI is a standard protocol that allows web servers to execute external
	programs (scripts) to generate dynamic content. Instead of serving a
	static file, the server runs a program and sends its output to the client.

	Example flow:
		1. Client requests: GET /cgi-bin/hello.py?name=World
		2. Server detects .py extension -> CGI request
		3. Server forks a child process
		4. Child sets environment variables (REQUEST_METHOD=GET, QUERY_STRING=name=World)
		5. Child executes: /usr/bin/python3 /var/www/cgi-bin/hello.py
		6. Script runs, outputs: "Content-Type: text/html\r\n\r\n<h1>Hello World!</h1>"
		7. Server reads script output, sends to client

	Why CGI?
	--------
	- Dynamic content: Generate pages based on user input, database queries, time, etc.
	- Language agnostic: PHP, Python, Perl, Ruby, even compiled C programs
	- Simple protocol: Just environment variables and stdin/stdout
	- Isolation: Each request runs in its own process (security benefit)

	CGI Environment Variables (RFC 3875):
	-------------------------------------
	The server communicates with the CGI script via environment variables.
	These are set before execve() and the script reads them with getenv().

	Required variables:
		REQUEST_METHOD    - HTTP method (GET, POST, DELETE)
		QUERY_STRING      - URL parameters after '?' (e.g., "name=John&age=30")
		CONTENT_TYPE      - MIME type of request body (for POST)
		CONTENT_LENGTH    - Size of request body in bytes (for POST)
		SCRIPT_NAME       - Virtual path to the script (e.g., "/cgi-bin/test.py")
		SERVER_PROTOCOL   - Protocol version (e.g., "HTTP/1.1")
		GATEWAY_INTERFACE - CGI version (e.g., "CGI/1.1")

	Server information:
		SERVER_NAME       - Hostname or IP
		SERVER_PORT       - Port number
		SERVER_SOFTWARE   - Server identification

	Script information:
		SCRIPT_FILENAME   - Absolute filesystem path to script
		PATH_INFO         - Extra path after script name
		PATH_TRANSLATED   - PATH_INFO mapped to filesystem

	Client information:
		REMOTE_ADDR       - Client's IP address
		REMOTE_HOST       - Client's hostname (if available)

	HTTP Headers (prefixed with HTTP_):
		HTTP_HOST         - From Host header
		HTTP_USER_AGENT   - From User-Agent header
		HTTP_ACCEPT       - From Accept header
		HTTP_COOKIE       - From Cookie header
		etc.

	CGI Script Output Format:
	-------------------------
	The script writes to stdout in this format:
		Header1: Value1\r\n
		Header2: Value2\r\n
		\r\n
		<body content>

	Minimum required header: Content-Type
	Example output:
		Content-Type: text/html\r\n
		\r\n
		<html><body>Hello!</body></html>

	POST Request Handling:
	----------------------
	For POST requests with a body:
		1. CONTENT_LENGTH tells script how many bytes to read from stdin
		2. Server writes request body to script's stdin (via pipe)
		3. Script reads stdin, processes data
		4. Script writes response to stdout

	Security Considerations:
	------------------------
	- Validate script path (prevent directory traversal)
	- Check script is executable
	- Set timeout to prevent hung scripts
	- Run script in restricted directory
	- Don't expose server internals in error messages
*/


#include <string>
#include <map>
#include <vector>

// Forward declarations to avoid circular includes
class Request;
struct LocationConfig;

/*
	CGI Class
	---------
	Handles CGI script detection, environment setup, and execution preparation.

	Lifecycle:
		1. Create CGI object with request and location config
		2. Call setup() to validate and prepare environment
		3. If setup succeeds, call execute() (implemented in Step 8.2)
		4. Get response from getResponse()

	Usage example:
		CGI cgi(request, location);
		if (cgi.setup(scriptPath)) {
			Response response = cgi.execute();
			// send response to client
		} else {
			// return error response (cgi.getErrorCode())
		}
*/
class CGI
{
public:
	// ===========================
	//  Constructors & Destructor
	// ===========================

	/*
		Default constructor
		Creates an uninitialized CGI handler.
		Must call setup() before use.
	*/
	CGI();

	/*
		Parameterized constructor
		Initializes CGI with request context.

		Parameters:
			request:  The HTTP request being processed
			location: The location config that matched (contains CGI settings)
	*/
	CGI(const Request& request, const LocationConfig& location);

	/*
		Destructor
		Cleans up any allocated resources.
		Note: Child process cleanup is handled separately.
	*/
	~CGI();

	/*
		Copy constructor and assignment operator
		Required for C++98 compliance and container storage.
	*/
	CGI(const CGI& other);
	CGI& operator=(const CGI& other);


	// ===========================
	//  Setup & Detection
	// ===========================

	/*
		setup() - Prepare CGI for execution

		This is the main setup function that:
		1. Validates the script exists and is executable
		2. Validates the CGI interpreter exists and is executable
		3. Builds all required environment variables
		4. Prepares arguments for execve()

		Parameters:
			scriptPath: Absolute filesystem path to the CGI script

		Returns:
			true if setup successful, false on error
			On error, getErrorCode() returns appropriate HTTP status

		Error codes:
			404 - Script not found
			403 - Script not executable
			500 - CGI interpreter not found or not executable
	*/
	bool setup(const std::string& scriptPath);

	/*
		isCgiRequest() - Static helper to detect CGI requests

		Checks if a request should be handled as CGI based on:
		1. File extension matches cgi_extension in location config
		2. CGI interpreter is configured for this location

		Parameters:
			path:     Filesystem path to the requested resource
			location: Location config to check CGI settings

		Returns:
			true if this should be handled as CGI
	*/
	static bool isCgiRequest(const std::string& path, const LocationConfig& location);


	// ===========================
	//  Environment Variables
	// ===========================

	/*
		getEnvArray() - Get environment as char** for execve()

		execve() requires environment as null-terminated array of
		null-terminated strings in format "NAME=VALUE".

		Returns:
			Array suitable for execve(). Caller must free with freeEnvArray().

		Example:
			char** env = cgi.getEnvArray();
			execve(interpreter, args, env);
			// In parent process after fork:
			CGI::freeEnvArray(env);
	*/
	char** getEnvArray() const;

	/*
		freeEnvArray() - Free memory allocated by getEnvArray()

		Static method to properly deallocate the environment array.
		Must be called after execve() fails or in parent after fork.
	*/
	static void freeEnvArray(char** envArray);

	/*
		getEnvMap() - Get environment as map (for debugging/testing)

		Returns a copy of the environment variable map.
		Useful for unit testing and debugging.
	*/
	const std::map<std::string, std::string>& getEnvMap() const;


	// ===========================
	//  Getters
	// ===========================

	/*
		getScriptPath() - Get absolute path to CGI script
	*/
	const std::string& getScriptPath() const;

	/*
		getInterpreterPath() - Get path to CGI interpreter

		Examples:
			"/usr/bin/python3" for .py files
			"/usr/bin/php-cgi" for .php files
			"/usr/bin/perl" for .pl files
	*/
	const std::string& getInterpreterPath() const;

	/*
		getWorkingDirectory() - Get directory to chdir() to before exec

		CGI scripts should run in their own directory so relative
		file paths work correctly.
	*/
	const std::string& getWorkingDirectory() const;

	/*
		getErrorCode() - Get HTTP error code if setup failed

		Returns:
			0 if no error
			HTTP status code (404, 403, 500, etc.) on error
	*/
	int getErrorCode() const;

	/*
		getErrorMessage() - Get human-readable error description
	*/
	const std::string& getErrorMessage() const;

	/*
		isReady() - Check if CGI is ready for execution
	*/
	bool isReady() const;

	/*
		getRequestBody() - Get request body to send to CGI stdin
	*/
	const std::string& getRequestBody() const;


	// ===========================
	//  Arguments for execve()
	// ===========================

	/*
		getArgv() - Get argument array for execve()

		Returns null-terminated array:
			[0] = interpreter path (e.g., "/usr/bin/python3")
			[1] = script path (e.g., "/var/www/cgi-bin/test.py")
			[2] = NULL

		Caller must free with freeArgv().
	*/
	char** getArgv() const;

	/*
		freeArgv() - Free memory allocated by getArgv()
	*/
	static void freeArgv(char** argv);



private:
	// ===========================
	//  Private Helper Methods
	// ===========================

	/*
		buildEnvironment() - Construct all CGI environment variables

		Called by setup() to populate _envVars map with all required
		and optional environment variables.
	*/
	void buildEnvironment();

	/*
		addHttpHeaders() - Convert HTTP headers to CGI format

		HTTP headers become environment variables with HTTP_ prefix:
			Host: localhost        -> HTTP_HOST=localhost
			User-Agent: Mozilla    -> HTTP_USER_AGENT=Mozilla
			Accept-Language: en    -> HTTP_ACCEPT_LANGUAGE=en

		Note: Header names are uppercased and dashes become underscores.
	*/
	void addHttpHeaders();

	/*
		validateScript() - Check script exists and is executable

		Uses stat() to verify:
		1. File exists
		2. File is a regular file (not directory)
		3. File has execute permission

		Returns:
			true if script is valid
			false and sets _errorCode/_errorMessage on failure
	*/
	bool validateScript(const std::string& path);

	/*
		validateInterpreter() - Check CGI interpreter is executable

		Verifies the interpreter (e.g., /usr/bin/python3) exists
		and is executable.
	*/
	bool validateInterpreter(const std::string& path);

	/*
		extractPathInfo() - Extract PATH_INFO from request

		PATH_INFO is extra path information after the script name.
		Example:
			Request: /cgi-bin/script.py/extra/path?query=value
			SCRIPT_NAME: /cgi-bin/script.py
			PATH_INFO: /extra/path

		This is useful for RESTful CGI scripts.
	*/
	std::string extractPathInfo() const;


	// ===========================
	//  Member Variables
	// ===========================

	// Request context
	const Request* _request;          // Pointer to HTTP request (not owned)
	const LocationConfig* _location;  // Pointer to location config (not owned)

	// Script information
	std::string _scriptPath;          // Absolute path to CGI script
	std::string _interpreterPath;     // Path to interpreter (python, php, etc.)
	std::string _workingDirectory;    // Directory to run script in
	std::string _pathInfo;            // Extra path after script name

	// Environment variables
	std::map<std::string, std::string> _envVars;  // NAME=VALUE pairs

	// Status
	bool _ready;                      // True if setup() succeeded
	int _errorCode;                   // HTTP error code (0 = no error)
	std::string _errorMessage;        // Human-readable error description

























};

#endif

