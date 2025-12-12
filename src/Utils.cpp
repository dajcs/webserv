/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:56:20 by anemet            #+#    #+#             */
/*   Updated: 2025/12/12 16:53:17 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Utils.hpp"
#include <algorithm>
#include <sstream>
#include <ctime>
#include <sys/stat.h>
#include <cstdlib>


/*
	// =========================
	//  Utils Implementation
	// =========================

	Utility functions that can be used throughout the webserv project.
	The main focus is the multipart/form-data parsing for file uploads.

	Multipart parsing is complex:
		- Binary-safe: File data can contain any bytes, including null bytes
		- Boundaries can appear in data: must be handled correctly
		- Headers need parsing: Content-Disposition header has structured format
		- Multiple parts: A form can have many fields and files

	RFC 2046 defines the multipart format. Key points:
	- Parts are separated by "--" + boundary
	- Final part ends with "--" + boundary + "--"
	- Each part has headers, then blank line, then body
*/


namespace Utils
{

// ==================================
//  String Utilities Implementation
// ==================================

/*
	trim() - Remove whitespace from both ends of a string
*/
std::string trim(const std::string& str)
{
	// Find first non-whitespace character
	size_t start = str.find_first_not_of(" \t\r\n\v");
	if (start == std::string::npos)
	{
		return ""; // String is all whitespace
	}

	// Find last non-whitespace character
	size_t end = str.find_last_not_of(" \t\r\n\v");

	return str.substr(start, end - start + 1);
}

/*
	toLower()  -  Convert string to lowercase
*/
std::string toLower(const std::string& str)
{
	std::string result = str;
	for (size_t i = 0; i < result.length(); ++i)
	{
		// convert ASCII upper to lower
		if (result[i] >= 'A' &&result[i] <= 'Z')
		{
			result[i] = result[i] + 32;
		}
	}
	return result;
}

/*
	startsWith()  --  Check if string starts with given prefix
*/
bool startsWith(const std::string& str, const std::string& prefix)
{
	if (prefix.length() > str.length())
	{
		return false;
	}

	return str.compare(0, prefix.length(), prefix) == 0;
}

/*
	endsWith()  -  check if string ends with given suffix
*/
bool endsWith(const std::string& str, const std::string& suffix)
{
	if (suffix.length() > str.length())
	{
		return false;
	}
	return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

/*
	split()  -  Split a string by a delimiter string

	Example:
		split("a--b--c") --> ["a", "b", "c"]
*/
std::vector<std::string> split(const std::string& str, const std::string& delimiter)
{
	std::vector<std::string> result;
	size_t start = 0;
	size_t end = 0;

	while ((end = str.find(delimiter, start)) != std::string::npos)
	{
		result.push_back(str.substr(start, end - start));
		start = end + delimiter.length();
	}

	// Add the last segment (after the final delimiter)
	result.push_back(str.substr(start));

	return result;
}


// =======================================
//  Multipart Parsing Implementation
// =======================================

/*
	extractBoundary() - Extract boundary from Content-Type header

	The Content-Type header for multipart requests looks like:
		multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW

	We need to find "boundary=" and extract the value that follows.

	Important edge cases:
	- Boundary may or may not be quoted: boundary="abc" or boundary=abc
	- There may be other parameters before boundary
	- Whitespace around the = sign should be handled
*/
std::string extractBoundary(const std::string& contentType)
{
	// Look for "boundary=" in the Content-Type (case-insensitive)
	std::string lowerCT = toLower(contentType);
	size_t boundaryPos = lowerCT.find("boundary");

	if (boundaryPos == std::string::npos)
	{
		return ""; // No boundary found
	}

	// Skip "boundary="
	size_t pos = boundaryPos + 8; // length of "boundary"

	// Skip optional whitespace before '='
	while (pos < contentType.length() && (contentType[pos] == ' ' ||
												contentType[pos] == '\t'))
	{
		pos++;
	}

	// Expect '='
	if (pos >= contentType.length() || contentType[pos] != '=')
	{
		return ""; // Malformed - no '=' after boundary
	}
	pos++; // Skip '='

	// Skip optional whitespace after '='
	while (pos < contentType.length() && (contentType[pos] == ' ' ||
												contentType[pos] == '\t'))
	{
		pos++;
	}


	// Check if value is quoted
	if (pos < contentType.length() && contentType[pos] == '"')
	{
		// Quoted value - find closing quote
		pos++;
		size_t valueEnd = contentType.find('"', pos);
		if (valueEnd == std::string::npos)
		{
			return ""; // Malformed - no closing quote
		}
		return contentType.substr(pos, valueEnd - pos);
	}
	else
	{
		// Unquoted value - ends at ; or end of string
		size_t valueEnd = contentType.find(';', pos);
		if (valueEnd == std::string::npos)
		{
			valueEnd = contentType.length();
		}
		return trim(contentType.substr(pos, valueEnd - pos));
	}
}

/*
	parseContentdisposition()  -  ParseContent-Disposition header

	Format: form-data; name="fieldname";, filename="original.txt"

	The header contains:
	- name: The form field name (required)
	- filename Original filename for file uploads (optional)
*/
void parseContentDisposition(const std::string& header,
					std::string& name, std::string& filename)
{
	name.clear();
	filename.clear();

	// Split header into parts by semicolon
	size_t pos = 0;
	while (pos < header.length())
	{
		// Skip leading whitespace
		while (pos < header.length() && (header[pos] == ' ' || header[pos] == '\t'))
		{
			pos++;
		}

		// Find the next semicolon or end of string
		size_t semicolonPos = header.find(';', pos);
		if (semicolonPos == std::string::npos)
		{
			semicolonPos = header.length();
		}

		// Extract this part
		std::string part = header.substr(pos, semicolonPos - pos);

		// Look for = sign (key=value pair)
		size_t equalsPos = part.find('=');
		if (equalsPos != std::string::npos)
		{
			std::string key = trim(part.substr(0, equalsPos));
			std::string value = trim(part.substr(equalsPos + 1));

			// Remove quotes if present
			if (value.length() >= 2 && value[0] == '"' && value[value.length() - 1] == '"')
			{
				value = value.substr(1, value.length() - 2);
			}

			// Store the values we care about
			if (toLower(key) == "name")
			{
				name = value;
			}
			else if (toLower(key) == "filename")
			{
				filename = value;
			}
		}

		pos = semicolonPos + 1;
	} // end of while(pos < header.length())
}


/*
	parseMultipart()  -  Parse multipart/form-data body

	The main parsing function for file uploads.
	It takes the HTTP body and splits it into individual parts.

	Multipart body structure:
	-------------------------
	--boundary\r\n
	headers\r\n
	\r\n
	body data\r\n
	--boundary\r\n
	headers\r\n
	\r\n				<- empty line separates header & body data
	body data\r\n
	--boundary--\r\n	<- the trailing \r\n optional & trimmed away

	Algorithm:
	1. Find each boundary occurrence
	2. Extract content between boundaries
	3. For each part, separate headers from body
	4. Parse headers to get name/filename/content-type
	5. Store the body data

	The body is binary-safe: it can contain any bytes, including
	the boundary string as actual data.
	We rely on the boundary being preceded by \r\n (except at the start of the body)
*/
std::vector<MultipartPart> parseMultipart(const std::string& body,
										const std::string& boundary)
{
	std::vector<MultipartPart> parts;

	if (boundary.empty())
	{
		return parts; // No boundary means we can't parse
	}

	// The actual delimiter in the body is "--" + boundary
	std::string delimiter = "--" + boundary;

	// find the fist delimiter
	size_t pos = body.find(delimiter);
	if (pos == std::string::npos)
	{
		return parts; // No valid multipart content
	}

	// Move past the first delimiter
	pos += delimiter.length();

	// process each part
	while (pos < body.length())
	{
		// Skip CRLF after delimiter
		if (pos < body.length() - 1 && body[pos] == '\r' && body[pos + 1] == '\n')
		{
			pos += 2;
		}
		else if (pos < body.length() && body[pos] == '\n')
		{
			pos += 1;  // Handle LF only (some clients)
		}

		// Check for final delimiter (--boundary--)
		if (pos < body.length() - 1 && body[pos] == '-' && body[pos+1] == '-')
		{
			break; // Last part, End of multipart message
		}

		// Find the next delimiter to know where this part ends
		size_t nextDelim = body.find(delimiter, pos);
		if (nextDelim == std::string::npos)
		{
			break; // No more parts
		}

		// Extract this part (everything between current pos and next delimiter)
		// The next delimiter is preceded by \r\n, so we need to exclude that
		size_t partEnd = nextDelim;
		if (partEnd >= 2 && body[partEnd - 2] == '\r' && body[partEnd - 1] == '\n')
		{
			partEnd -= 2;
		}
		else if (partEnd >= 1 && body[partEnd - 1] == '\n')
		{
			partEnd -= 1;
		}

		std::string partContent = body.substr(pos, partEnd - pos);

		// Now parse this part: separate headers from body
		// Headers and body are separated by blank line (\r\n\r\n)
		size_t headerEnd = partContent.find("\r\n\r\n");
		if (headerEnd == std::string::npos)
		{
			// Try LF only
			headerEnd = partContent.find("\n\n");
			if (headerEnd == std::string::npos)
			{
				// No body in this part - skip it
				pos = nextDelim + delimiter.length();
				continue;
			}
		}

		std::string headersStr = partContent.substr(0, headerEnd);
		size_t bodyStart = headerEnd + (partContent[headerEnd] == '\r' ? 4 : 2);
		std::string partBody = "";
		if (bodyStart < partContent.length())
		{
			partBody = partContent.substr(bodyStart);
		}

		// Parse the headers
		MultipartPart part;
		part.data = partBody;

		// Split headers by line and parse each
		std::vector<std::string> headerLines = split(headersStr, "\r\n");
		for (size_t i = 0; i < headerLines.size(); ++i)
		{
			std::string line = headerLines[i];
			if (line.empty())
				continue;

			// Handle LF-only line endings
			if (!line.empty() && line[line.length() - 1] == '\n')
			{
				line = line.substr(0, line.length() - 1);
			}

			// Find the colon separator
			size_t colonPos = line.find(':');
			if (colonPos == std::string::npos)
				continue;

			std::string headerName = toLower(trim(line.substr(0, colonPos)));
			std::string headerValue = trim(line.substr(colonPos + 1));

			if (headerName == "content-disposition")
			{
				parseContentDisposition(headerValue, part.name, part.filename);
			}
			else if (headerName == "content-type")
			{
				part.contentType = headerValue;
			}
		} // end of for(headerLines)

		// Only add parts that have a name
		if (!part.name.empty())
		{
			parts.push_back(part);
		}

		// Move to next part
		pos = nextDelim + delimiter.length();
	}

	return parts;
}


// ===============================
//  URL Encoding Implementation
// ===============================

/*
	urlDecode()  -  Decode percent-encoded URL strings

	URL encoding (RFC 3986) encodes unsafe characters as %XX where XX is
	the hex value of the byte. Also, + is used for space in query strings.

	Examples:
		%20 -> ' ' (space)
		%2F -> '/' (forward slash)
		%3D -> '=' (equals)
		+ -> ' ' (space, in form data)
*/
std::string urlDecode(const std::string& str)
{
	std::string result;
	result.reserve(str.length());  // Reserve space for efficiency

	for (size_t i = 0; i < str.length(); ++i)
	{
		if (str[i] == '%' && i + 2 < str.length())
		{
			// Decode %XX
			char hex[3] = {str[i + 1], str[i + 2], '\0'};
			char* endPtr;
			long value = strtol(hex, &endPtr, 16);

			// Check if conversion was successful
			if (*endPtr == '\0' && value >= 0 && value <= 255)
			{
				result += static_cast<char>(value);
				i += 2;  // Skip the two hex digits
			}
			else
			{
				result += str[i];  // Not a valid escape, keep as-is
			}
		}
		else if (str[i] == '+')
		{
			// + represents space in form data
			result += ' ';
		}
		else
		{
			result += str[i];
		}
	}

	return result;
}


/*
	parseFormUrlEncoded()  -  Parse application/x-www-form-urlencoded data

	This is the default encoding for HTML forms without file uploads
	Format: key1=value1&key2=value2

	Each key and value is URL-encoded separately
*/
std::map<std::string, std::string> parseFormUrlEncoded(const std::string& body)
{
	std::map<std::string, std::string> result;

	//Split by & to get key=value pairs
	std::vector<std::string> pairs = split(body, "&");


	for (size_t i = 0; i < pairs.size(); ++i)
	{
		std::string pair = pairs[i];
		if (pair.empty())
			continue;

		// Find the = separator
		size_t equalsPos = pair.find('=');
		if (equalsPos == std::string::npos)
		{
			// Key with no value
			result[urlDecode(pair)] = "";
		}
		else
		{
			std::string key = urlDecode(pair.substr(0, equalsPos));
			std::string value = urlDecode(pair.substr(equalsPos + 1));
			result[key] = value;
		}
	}

	return result;
}


// =================================
//  File Utilities Implementation
// =================================

/*
	sanitizeFilename() - Make filename safe for storage

	User-provided filenames are dangerous! They can contain:
	- Path traversal: "../../../etc/passwd"
	- Absolute paths: "/etc/passwd"
	- Special characters: "; rm -rf /"
	- Very long names that cause issues

	Our sanitization:
	1. Remove any directory components (keep only filename)
	2. Replace or remove dangerous characters
	3. Limit length
	4. Provide fallback for empty result
*/
std::string sanitizeFilename(const std::string& filename)
{
	if (filename.empty())
	{
		return "unnamed";
	}

	std::string result;

	// Find the last path separator to get just the filename
	size_t lastSlash = filename.find_last_of("/\\");
	std::string baseName;
	if (lastSlash != std::string::npos)
	{
		baseName = filename.substr(lastSlash + 1);
	}
	else
	{
		baseName = filename;
	}

	// Filter characters - allow only safe ones
	// Safe: a-z, A-Z, 0-9, _, -, .
	for (size_t i = 0; i < baseName.length() && result.length() < 200; ++i)
	{
		char c = baseName[i];
		if ((c >= 'a' && c <= 'z') ||
			(c >= 'A' && c <= 'Z') ||
			(c >= '0' && c <= '9') ||
			c == '_' || c == '-' || c == '.')
		{
			result += c;
		}
		else
		{
			// Replace other characters with underscore
			result += '_';
		}
	}

	// Remove leading dots (hidden files, parent traversal attempts)
	while (!result.empty() && result[0] == '.')
	{
		result = result.substr(1);
	}

	// Ensure we have something left
	if (result.empty())
	{
		return "unnamed";
	}

	return result;
}


/*
	generateUniqueFilename() - Create unique filename to avoid collisions

	If "photo.jpg" already exists, we want to save as:
	- "photo_1.jpg"
	- "photo_2.jpg"
	- etc.

	Our approach: use timestamp
	- "photo_1702382400.jpg"

*/
std::string generateUniqueFilename(const std::string& directory,
									const std::string& baseFilename)
{
	std::string sanitized = sanitizeFilename(baseFilename);

	// Extract name and extension
	std::string name = sanitized;
	std::string extension = "";
	size_t dotPos = sanitized.find_last_of('.');
	if (dotPos != std::string::npos && dotPos > 0)
	{
		name = sanitized.substr(0, dotPos);
		extension = sanitized.substr(dotPos);
	}

	// Construct full path and check if file exists
	std::string fullPath = directory;
	if (!fullPath.empty() && fullPath[fullPath.length() - 1] != '/')
	{
		fullPath += "/";
	}
	fullPath += sanitized;

	// Check if file exists
	struct stat st;
	if (stat(fullPath.c_str(), &st) != 0)
	{
		// File doesn't exist, we can use this name
		return fullPath;
	}

	// File exists - add timestamp for uniqueness
	std::stringstream ss;
	ss << directory;
	if (!directory.empty() && directory[directory.length() - 1] != '/')
	{
		ss << "/";
	}
	ss << name << "_" << time(NULL) << extension;

	return ss.str();
}


/*
	getFileExtension() - Get the file extension

	Returns the extension including the dot, or empty string if none.
	Example: "photo.jpg" -> ".jpg"
*/
std::string getFileExtension(const std::string& filename)
{
	size_t dotPos = filename.find_last_of('.');
	if (dotPos == std::string::npos || dotPos == filename.length() - 1)
	{
		return "";
	}
	return filename.substr(dotPos);
}


/*
	directoryExists() - Check if directory exists

	Uses stat() to check if path exists and is a directory.
*/
bool directoryExists(const std::string& path)
{
	struct stat st;
	if (stat(path.c_str(), &st) != 0)
	{
		return false;
	}
	return S_ISDIR(st.st_mode);
}



/*
	createDirectory() - Create directory if it doesn't exist

	Note: This simple version doesn't create parent directories.
	For full functionality, would need to iterate and create each level.
*/
bool createDirectory(const std::string& path)
{
	if (directoryExists(path))
	{
		return true;
	}

	// Create directory with permissions: rwxr-xr-x (755)
	return mkdir(path.c_str(), 0755) == 0;
}


} // namespace Utils
