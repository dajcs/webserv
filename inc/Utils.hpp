/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:57:49 by anemet            #+#    #+#             */
/*   Updated: 2025/12/12 16:33:08 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <map>
#include <vector>

/*
	=================================
		MULTIPART/FORM-DATA OVERVIEW
	=================================

	When a browser uploads files via HTML forms with enctype="multipart/form-data",
	the request body is formatted in a special way called MIME multipart.

	Example HTML form:
		<form action="/upload" method="POST" enctype="multipart/form-data">
			<input type="file" name="myfile">
			<input type="text" name="description">
			<button type="submit">Upload</button>
		</form>

	The resulting HTTP request looks like:
		POST /upload HTTP/1.1
		Host: localhost:8080
		Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW
		Content-Length: 1234

		------WebKitFormBoundary7MA4YWxkTrZu0gW
		Content-Disposition: form-data; name="description"

		My file description
		------WebKitFormBoundary7MA4YWxkTrZu0gW
		Content-Disposition: form-data; name="myfile"; filename="photo.jpg"
		Content-Type: image/jpeg

		<binary file data here>
		------WebKitFormBoundary7MA4YWxkTrZu0gW--

	Key concepts:
	1. BOUNDARY: A unique string that separates each part (prefixed with --)
	2. Each part has:
	   - Headers (Content-Disposition, optionally Content-Type)
	   - Empty line separator
	   - Body (the actual data)
	3. Final boundary ends with -- suffix

	Content-Disposition header contains:
	- name: The form field name
	- filename: (optional) Original filename if it's a file upload
*/


/*
	MultipartPart: Represents one part of a multipart message

	A multipart form can have multiple parts:
	- Text fields (name + value)
	- File uploads (name + filename + content-type + data)
*/
struct MultipartPart
{
	std::string name;			// Form field name (from Content-Disposition)
	std::string filename;		// Original filename (empty for non-file fields)
	std::string contentType;	// MIME type of the content
	std::string data;			// The actual content/data

	// Helper to check if this part is a file upload
	// bool hasFile() const { return !filename.empty(); }

	// Default constructor
	MultipartPart() : name(""), filename(""), contentType("text/plain"), data("") {}
};


/*
	Utility namespace for various helper functions:
		- String manipulation
		- Multipart parsing
		- File operations
		- URL encoding/decoding
*/
namespace Utils
{
	// ==================
	//  String Utilities
	// ==================

	// Remove leading and trailing whitespace
	std::string trim(const std::string& str);

	// convert string to lowercase (for case-insensitive comparisons)
	std::string toLower(const std::string& str);

	// Check if string starts with a prefix
	bool startsWith(const std::string& str, const std::string& prefix);

	// Check if string ends with a suffix
	bool endsWith(const std::string& str, const std::string& suffix);

	// Split string by delimiter
	std::vector<std::string> split(const std::string& str, const std::string& delimiter);


	// ========================
	//  Multipart Parsing
	// ========================

	// Extract the boudary string from Content-Type header
	std::string extractBoundary(const std::string& contentType);

	// Parse a multipart/form-data into its parts
	std::vector<MultipartPart> parseMultipart(const std::string& body,
											const std::string& boundary);

	// Parse the Content-Disposition header
	void parseContentDisposition(const std::string& header,
								std::string& name, std::string& filename);


	// ========================
	//  URL Encoding/Decoding
	// ========================

	// Decode URL encoded string
	std::string urlDecode(const std::string& str);

	// Parse application/x-www-form-urlencoded data
	std::map<std::string, std::string> parseFormUrlEncoded(const std::string& body);


	// =======================
	//  File Utilities
	// =======================

	// Make filename safe for filesystem storage
	std::string sanitizeFilename(const std::string& filename);

	// Create a unique filename
	std::string generateUniqueFilename(const std::string& directory,
										const std::string& baseFilename);

	// Extract file extension from filename
	std::string getFileExtension(const std::string& filename);

	// Check if directory exists
	bool directoryExists(const std::string& path);

	// Create a directory (and parent directories if needed)
	bool createDirectory(const std::string& path);


} // end namespace Utils


#endif
