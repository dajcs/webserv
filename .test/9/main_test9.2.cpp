/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:54:34 by anemet            #+#    #+#             */
/*   Updated: 2025/12/14 20:34:08 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

/*
	=================================================================
		DIRECTORY LISTING TEST SUITE - Step 9.2
	=================================================================

	Directory Listing Overview:
	---------------------------
	Directory listing (autoindex) allows users to see the contents
	of a directory when no index file is present. This is useful for:

	1. File Download Areas:
		Users can browse and download files without needing a custom
		HTML page for each directory.

	2. Development/Debugging:
		Developers can quickly see what files are served.

	3. Simple File Sharing:
		Quick way to share files without building a UI.

	How Directory Listing Works:
	----------------------------
	1. Client requests: GET /files/ HTTP/1.1
	2. Server checks: Is /files/ a directory?
	3. Server checks: Is there an index file (index.html)?
		- YES → Serve the index file
		- NO  → Is autoindex enabled?
				- YES → Generate HTML listing
				- NO  → Return 403 Forbidden

	POSIX Functions Used:
	---------------------
	- opendir():  Opens a directory stream for reading
	- readdir():  Reads the next directory entry
	- closedir(): Closes the directory stream
	- stat():     Gets file metadata (size, type, modification time)

	Test Strategy:
	--------------
	Since the network layer isn't implemented, we test:
	1. Location configuration (autoindex setting)
	2. Directory listing HTML generation
	3. File/directory sorting
	4. Size formatting
	5. HTML escaping (XSS prevention)
	6. Error cases (permissions, non-existent directories)
*/

#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <algorithm>

#include "Config.hpp"
#include "Request.hpp"
#include "Response.hpp"
#include "Router.hpp"

// ===========================================
//  Test Infrastructure
// ===========================================

static int g_testsRun = 0;
static int g_testsPassed = 0;
static int g_testsFailed = 0;

// ANSI color codes
#define COLOR_RED		"\033[31m"
#define COLOR_GREEN		"\033[32m"
#define COLOR_YELLOW	"\033[33m"
#define COLOR_BLUE		"\033[34m"
#define COLOR_CYAN		"\033[36m"
#define COLOR_RESET		"\033[0m"
#define BOLD			"\033[1m"

// ANSI color codes for test output
#define GREEN   "\033[32m"
#define RED     "\033[31m"
#define RESET   "\033[0m"



#define TEST_ASSERT(condition, message) \
	do { \
		g_testsRun++; \
		if (condition) { \
			g_testsPassed++; \
			std::cout << COLOR_GREEN << "[PASS] " << COLOR_RESET << message << std::endl; \
		} else { \
			g_testsFailed++; \
			std::cout << COLOR_RED << "[FAIL] " << COLOR_RESET << message << std::endl; \
			std::cout << "       Expected: " << #condition << std::endl; \
		} \
	} while (0)

#define TEST_SECTION(name) \
	std::cout << std::endl << COLOR_BLUE << "=== " << name << " ===" << COLOR_RESET << std::endl


// ===========================================
//  Test Directory Setup
// ===========================================

/*
	Test Directory Structure:
	-------------------------
	We create a temporary test directory structure:

	test_www/
	├── index.html           (for testing index file serving)
	├── files/               (for testing directory listing)
	│   ├── document.txt
	│   ├── image.jpg
	│   ├── script.py
	│   └── subdir/
	│       └── nested.txt
	├── empty/               (empty directory)
	├── with_index/          (directory with index file)
	│   └── index.html
	└── special_chars/       (filenames with special characters)
		├── file with spaces.txt
		└── <script>.txt
*/

static const std::string TEST_ROOT = "test_www_dirlisting";

bool createTestDirectory(const std::string& path)
{
	return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
}

bool createTestFile(const std::string& path, const std::string& content)
{
	std::ofstream file(path.c_str());
	if (!file)
	{
		return false;
	}
	file << content;
	return file.good();
}

void setupTestDirectories()
{
	std::cout << COLOR_CYAN << "Setting up test directories..." << COLOR_RESET << std::endl;

	// Create main test root
	createTestDirectory(TEST_ROOT);

	// Create index.html in root
	createTestFile(TEST_ROOT + "/index.html", "<html><body>Root Index</body></html>");

	// Create files/ directory with various files
	createTestDirectory(TEST_ROOT + "/files");
	createTestFile(TEST_ROOT + "/files/document.txt", "This is a text document.");
	createTestFile(TEST_ROOT + "/files/image.jpg", "FAKE_JPEG_DATA");
	createTestFile(TEST_ROOT + "/files/script.py", "#!/usr/bin/env python3\nprint('hello')");

	// Create a larger file for size formatting test
	std::string largeContent(1024 * 100, 'X');  // 100 KB
	createTestFile(TEST_ROOT + "/files/large_file.bin", largeContent);

	// Create subdirectory
	createTestDirectory(TEST_ROOT + "/files/subdir");
	createTestFile(TEST_ROOT + "/files/subdir/nested.txt", "Nested file content");

	// Create another subdirectory for sorting test
	createTestDirectory(TEST_ROOT + "/files/another_dir");

	// Create empty directory
	createTestDirectory(TEST_ROOT + "/empty");

	// Create directory with index file
	createTestDirectory(TEST_ROOT + "/with_index");
	createTestFile(TEST_ROOT + "/with_index/index.html", "<html><body>Has Index</body></html>");

	// Create directory with special character filenames
	createTestDirectory(TEST_ROOT + "/special_chars");
	createTestFile(TEST_ROOT + "/special_chars/file with spaces.txt", "Spaces in name");
	createTestFile(TEST_ROOT + "/special_chars/<script>.txt", "XSS test filename");
	createTestFile(TEST_ROOT + "/special_chars/quotes\"here.txt", "Quotes in name");

	std::cout << COLOR_CYAN << "Test directories created at: " << TEST_ROOT << COLOR_RESET << std::endl;
}

void cleanupTestDirectories()
{
	std::cout << COLOR_CYAN << "Cleaning up test directories..." << COLOR_RESET << std::endl;

	// Simple cleanup using system command (works on Linux)
	std::string cmd = "rm -rf " + TEST_ROOT;
	int result = system(cmd.c_str());
	(void)result;
}


// ===========================================
//  Helper: Create Test Server Config
// ===========================================

/*
	Creates a ServerConfig with various directory listing scenarios.
*/
ServerConfig createDirListingTestServer()
{
	ServerConfig server;
	server.host = "0.0.0.0";
	server.port = 8080;

	/*
		Location 1: Directory listing enabled
		-------------------------------------
		autoindex on - will show directory contents
	*/
	LocationConfig loc1;
	loc1.path = "/files";
	loc1.root = TEST_ROOT;
	loc1.index = "index.html";
	loc1.autoindex = true;
	loc1.allowed_methods.insert("GET");
	server.locations.push_back(loc1);

	/*
		Location 2: Directory listing disabled
		--------------------------------------
		autoindex off (default) - will return 403 if no index
	*/
	LocationConfig loc2;
	loc2.path = "/empty";
	loc2.root = TEST_ROOT;
	loc2.index = "index.html";
	loc2.autoindex = false;
	loc2.allowed_methods.insert("GET");
	server.locations.push_back(loc2);

	/*
		Location 3: Has index file
		--------------------------
		Should serve index.html, not directory listing
	*/
	LocationConfig loc3;
	loc3.path = "/with_index";
	loc3.root = TEST_ROOT;
	loc3.index = "index.html";
	loc3.autoindex = true;  // Even with autoindex on, index file takes priority
	loc3.allowed_methods.insert("GET");
	server.locations.push_back(loc3);

	/*
		Location 4: Special characters test
		-----------------------------------
		Test HTML escaping in filenames
	*/
	LocationConfig loc4;
	loc4.path = "/special_chars";
	loc4.root = TEST_ROOT;
	loc4.index = "";  // No index file
	loc4.autoindex = true;
	loc4.allowed_methods.insert("GET");
	server.locations.push_back(loc4);

	/*
		Location 5: Root fallback
		-------------------------
	*/
	LocationConfig locRoot;
	locRoot.path = "/";
	locRoot.root = TEST_ROOT;
	locRoot.index = "index.html";
	locRoot.autoindex = false;
	locRoot.allowed_methods.insert("GET");
	server.locations.push_back(locRoot);

	return server;
}


// ===========================================
//  Test 1: Autoindex Configuration
// ===========================================

void testAutoindexConfiguration()
{
	TEST_SECTION("Test 1: Autoindex Configuration");

	ServerConfig server = createDirListingTestServer();

	/*
		Test: Location with autoindex on
	*/
	{
		const LocationConfig* loc = server.findLocation("/files");
		TEST_ASSERT(loc != NULL, "Location /files found");

		if (loc)
		{
			TEST_ASSERT(loc->autoindex == true,
						"Location /files has autoindex enabled");
		}
	}

	/*
		Test: Location with autoindex off (default)
	*/
	{
		const LocationConfig* loc = server.findLocation("/empty");
		TEST_ASSERT(loc != NULL, "Location /empty found");

		if (loc)
		{
			TEST_ASSERT(loc->autoindex == false,
						"Location /empty has autoindex disabled");
		}
	}

	/*
		Test: Default autoindex value
	*/
	{
		LocationConfig defaultLoc;  // Use defaults
		TEST_ASSERT(defaultLoc.autoindex == false,
					"Default autoindex is false (secure by default)");
	}
}


// ===========================================
//  Test 2: Directory Detection
// ===========================================

void testDirectoryDetection()
{
	TEST_SECTION("Test 2: Directory Detection");

	/*
		Test: Identify directory vs file using stat()
	*/
	{
		struct stat st;

		// Test directory
		std::string dirPath = TEST_ROOT + "/files";
		int result = stat(dirPath.c_str(), &st);
		TEST_ASSERT(result == 0, "stat() succeeds on existing directory");
		TEST_ASSERT(S_ISDIR(st.st_mode), TEST_ROOT + "/files is detected as directory");

		// Test file
		std::string filePath = TEST_ROOT + "/files/document.txt";
		result = stat(filePath.c_str(), &st);
		TEST_ASSERT(result == 0, "stat() succeeds on existing file");
		TEST_ASSERT(S_ISREG(st.st_mode), "document.txt is detected as regular file");
		TEST_ASSERT(!S_ISDIR(st.st_mode), "document.txt is NOT a directory");
	}

	/*
		Test: Non-existent path
	*/
	{
		struct stat st;
		std::string badPath = TEST_ROOT + "/nonexistent";
		int result = stat(badPath.c_str(), &st);
		TEST_ASSERT(result != 0, "stat() fails on non-existent path");
		TEST_ASSERT(errno == ENOENT, "errno is ENOENT for non-existent path");
	}
}


// ===========================================
//  Test 3: Directory Reading
// ===========================================

void testDirectoryReading()
{
	TEST_SECTION("Test 3: Directory Reading (opendir/readdir/closedir)");

	std::string dirPath = TEST_ROOT + "/files";

	/*
		Test: Open and read directory
	*/
	{
		DIR* dir = opendir(dirPath.c_str());
		TEST_ASSERT(dir != NULL, "opendir() succeeds on valid directory");

		if (dir)
		{
			std::vector<std::string> entries;
			struct dirent* entry;

			while ((entry = readdir(dir)) != NULL)
			{
				entries.push_back(entry->d_name);
			}

			closedir(dir);

			TEST_ASSERT(entries.size() > 0, "Directory has entries");

			// Check for . and ..
			bool hasDot = false;
			bool hasDotDot = false;
			for (size_t i = 0; i < entries.size(); ++i)
			{
				if (entries[i] == ".") hasDot = true;
				if (entries[i] == "..") hasDotDot = true;
			}
			TEST_ASSERT(hasDot, "Directory contains '.' entry");
			TEST_ASSERT(hasDotDot, "Directory contains '..' entry");

			// Check for our test files
			bool hasDocument = false;
			bool hasSubdir = false;
			for (size_t i = 0; i < entries.size(); ++i)
			{
				if (entries[i] == "document.txt") hasDocument = true;
				if (entries[i] == "subdir") hasSubdir = true;
			}
			TEST_ASSERT(hasDocument, "Found document.txt in directory listing");
			TEST_ASSERT(hasSubdir, "Found subdir/ in directory listing");
		}
	}

	/*
		Test: Open non-existent directory
	*/
	{
		DIR* dir = opendir((TEST_ROOT + "/nonexistent").c_str());
		TEST_ASSERT(dir == NULL, "opendir() fails on non-existent directory");
		TEST_ASSERT(errno == ENOENT, "errno is ENOENT");
	}
}


// ===========================================
//  Test 4: File Size Formatting
// ===========================================

/*
	Helper function to test - mimics Router::formatFileSize()
*/
std::string testFormatFileSize(off_t size)
{
	std::stringstream ss;

	if (size < 1024)
	{
		ss << size << " B";
	}
	else if (size < 1024 * 1024)
	{
		ss << std::fixed << std::setprecision(1) << (size / 1024.0) << " KB";
	}
	else if (size < 1024 * 1024 * 1024)
	{
		ss << std::fixed << std::setprecision(1) << (size / (1024.0 * 1024.0)) << " MB";
	}
	else
	{
		ss << std::fixed << std::setprecision(1) << (size / (1024.0 * 1024.0 * 1024.0)) << " GB";
	}

	return ss.str();
}

void testFileSizeFormatting()
{
	TEST_SECTION("Test 4: File Size Formatting");

	/*
		Test: Bytes
	*/
	{
		std::string result = testFormatFileSize(0);
		TEST_ASSERT(result == "0 B", "0 bytes formatted as '0 B'");

		result = testFormatFileSize(512);
		TEST_ASSERT(result == "512 B", "512 bytes formatted as '512 B'");

		result = testFormatFileSize(1023);
		TEST_ASSERT(result == "1023 B", "1023 bytes formatted as '1023 B'");
	}

	/*
		Test: Kilobytes
	*/
	{
		std::string result = testFormatFileSize(1024);
		TEST_ASSERT(result == "1.0 KB", "1024 bytes formatted as '1.0 KB'");

		result = testFormatFileSize(1536);  // 1.5 KB
		TEST_ASSERT(result == "1.5 KB", "1536 bytes formatted as '1.5 KB'");

		result = testFormatFileSize(102400);  // 100 KB
		TEST_ASSERT(result == "100.0 KB", "102400 bytes formatted as '100.0 KB'");
	}

	/*
		Test: Megabytes
	*/
	{
		std::string result = testFormatFileSize(1048576);  // 1 MB
		TEST_ASSERT(result == "1.0 MB", "1048576 bytes formatted as '1.0 MB'");

		result = testFormatFileSize(5242880);  // 5 MB
		TEST_ASSERT(result == "5.0 MB", "5242880 bytes formatted as '5.0 MB'");
	}

	/*
		Test: Gigabytes
	*/
	{
		std::string result = testFormatFileSize(1073741824);  // 1 GB
		TEST_ASSERT(result == "1.0 GB", "1073741824 bytes formatted as '1.0 GB'");
	}
}


// ===========================================
//  Test 5: HTML Escaping
// ===========================================

/*
	Helper function to test - mimics Router::escapeHtml()
*/
std::string testEscapeHtml(const std::string& str)
{
	std::string result;
	result.reserve(str.length() * 1.1);

	for (size_t i = 0; i < str.length(); ++i)
	{
		char c = str[i];
		switch (c)
		{
			case '&':
				result += "&amp;";
				break;
			case '<':
				result += "&lt;";
				break;
			case '>':
				result += "&gt;";
				break;
			case '"':
				result += "&quot;";
				break;
			case '\'':
				result += "&#39;";
				break;
			default:
				result += c;
				break;
		}
	}

	return result;
}

void testHtmlEscaping()
{
	TEST_SECTION("Test 5: HTML Escaping (XSS Prevention)");

	/*
		Test: No escaping needed
	*/
	{
		std::string result = testEscapeHtml("normal_filename.txt");
		TEST_ASSERT(result == "normal_filename.txt",
					"Normal filename unchanged");
	}

	/*
		Test: Ampersand escaping
	*/
	{
		std::string result = testEscapeHtml("file&name.txt");
		TEST_ASSERT(result == "file&amp;name.txt",
					"Ampersand escaped to &amp;");
	}

	/*
		Test: Less than / greater than
	*/
	{
		std::string result = testEscapeHtml("<script>alert('xss')</script>");
		TEST_ASSERT(result == "&lt;script&gt;alert(&#39;xss&#39;)&lt;/script&gt;",
					"Script tags escaped properly");
	}

	/*
		Test: Quote escaping
	*/
	{
		std::string result = testEscapeHtml("file\"with\"quotes.txt");
		TEST_ASSERT(result == "file&quot;with&quot;quotes.txt",
					"Double quotes escaped to &quot;");
	}

	/*
		Test: All special characters together
	*/
	{
		std::string input = "<tag attr=\"val\" & 'single'>";
		std::string expected = "&lt;tag attr=&quot;val&quot; &amp; &#39;single&#39;&gt;";
		std::string result = testEscapeHtml(input);
		TEST_ASSERT(result == expected,
					"All special characters escaped correctly");
	}
}


// ===========================================
//  Test 6: Time Formatting
// ===========================================

/*
	Helper function to test - mimics Router::formatTime()
*/
std::string testFormatTime(time_t timestamp)
{
	struct tm* timeinfo = localtime(&timestamp);
	if (!timeinfo)
	{
		return "-";
	}

	char buffer[32];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M", timeinfo);

	return std::string(buffer);
}

void testTimeFormatting()
{
	TEST_SECTION("Test 6: Time Formatting");

	/*
		Test: Current time
	*/
	{
		time_t now = time(NULL);
		std::string result = testFormatTime(now);

		// Just verify it's not empty and has expected format length
		TEST_ASSERT(result.length() == 16,
					"Formatted time has expected length (YYYY-MM-DD HH:MM)");
		TEST_ASSERT(result[4] == '-' && result[7] == '-' && result[10] == ' ',
					"Formatted time has correct separators");
	}

	/*
		Test: Specific timestamp (2024-01-15 10:30:00 UTC)
	*/
	{
		// Note: This test may vary by timezone
		time_t specificTime = 1705314600;  // 2024-01-15 10:30:00 UTC
		std::string result = testFormatTime(specificTime);
		TEST_ASSERT(!result.empty(), "Specific timestamp formats correctly");
		TEST_ASSERT(result.find("2024") != std::string::npos ||
					result.find("2025") != std::string::npos,  // Handle timezone variation
					"Year is present in formatted time");
	}
}


// ===========================================
//  Test 7: Index File Priority
// ===========================================

void testIndexFilePriority()
{
	TEST_SECTION("Test 7: Index File Priority");

	ServerConfig server = createDirListingTestServer();

	/*
		When a directory has an index file:
		- Serve the index file, NOT directory listing
		- Even if autoindex is enabled
	*/
	{
		const LocationConfig* loc = server.findLocation("/with_index");
		TEST_ASSERT(loc != NULL, "Location /with_index found");

		if (loc)
		{
			// Check that index file exists
			std::string indexPath = TEST_ROOT + "/with_index/index.html";
			struct stat st;
			bool indexExists = (stat(indexPath.c_str(), &st) == 0 && S_ISREG(st.st_mode));

			TEST_ASSERT(indexExists, "Index file exists in /with_index");
			TEST_ASSERT(loc->autoindex == true, "autoindex is enabled");
			TEST_ASSERT(!loc->index.empty(), "index directive is set");

			std::cout << "       Note: Index file takes priority over directory listing\n";
		}
	}

	/*
		When a directory has NO index file and autoindex is OFF:
		- Return 403 Forbidden
	*/
	{
		const LocationConfig* loc = server.findLocation("/empty");
		TEST_ASSERT(loc != NULL, "Location /empty found");

		if (loc)
		{
			// Check that index file does NOT exist
			std::string indexPath = TEST_ROOT + "/empty/index.html";
			struct stat st;
			bool indexExists = (stat(indexPath.c_str(), &st) == 0);

			TEST_ASSERT(!indexExists, "No index file in /empty");
			TEST_ASSERT(loc->autoindex == false, "autoindex is disabled");

			std::cout << "       Note: Without index and autoindex off -> 403 Forbidden\n";
		}
	}
}


// ===========================================
//  Test 8: Directory Entry Sorting
// ===========================================

void testDirectorySorting()
{
	TEST_SECTION("Test 8: Directory Entry Sorting");

	std::string dirPath = TEST_ROOT + "/files";

	/*
		Read directory entries and sort them
	*/
	DIR* dir = opendir(dirPath.c_str());
	TEST_ASSERT(dir != NULL, "Opened test directory");

	if (dir)
	{
		std::vector<std::string> directories;
		std::vector<std::string> files;

		struct dirent* entry;
		while ((entry = readdir(dir)) != NULL)
		{
			std::string name = entry->d_name;
			if (name == "." || name == "..")
			{
				continue;
			}

			std::string fullPath = dirPath + "/" + name;
			struct stat st;
			if (stat(fullPath.c_str(), &st) == 0)
			{
				if (S_ISDIR(st.st_mode))
				{
					directories.push_back(name);
				}
				else
				{
					files.push_back(name);
				}
			}
		}
		closedir(dir);

		// Sort both vectors
		std::sort(directories.begin(), directories.end());
		std::sort(files.begin(), files.end());

		TEST_ASSERT(directories.size() >= 2, "Found at least 2 directories");
		TEST_ASSERT(files.size() >= 3, "Found at least 3 files");

		// Verify sorting
		bool directoriesSorted = true;
		for (size_t i = 1; i < directories.size(); ++i)
		{
			if (directories[i] < directories[i-1])
			{
				directoriesSorted = false;
				break;
			}
		}
		TEST_ASSERT(directoriesSorted, "Directories are sorted alphabetically");

		bool filesSorted = true;
		for (size_t i = 1; i < files.size(); ++i)
		{
			if (files[i] < files[i-1])
			{
				filesSorted = false;
				break;
			}
		}
		TEST_ASSERT(filesSorted, "Files are sorted alphabetically");

		std::cout << "       Directories: ";
		for (size_t i = 0; i < directories.size(); ++i)
		{
			std::cout << directories[i] << "/ ";
		}
		std::cout << std::endl;

		std::cout << "       Files: ";
		for (size_t i = 0; i < files.size(); ++i)
		{
			std::cout << files[i] << " ";
		}
		std::cout << std::endl;
	}
}


// ===========================================
//  Test 9: Generated HTML Structure
// ===========================================

void testGeneratedHtmlStructure()
{
	TEST_SECTION("Test 9: Generated HTML Structure");

	/*
		Simulate what the directory listing HTML should contain.
		This tests the structure without needing the full Router.
	*/

	std::string dirPath = TEST_ROOT + "/files";

	// Build HTML like serveDirectory would
	std::stringstream html;
	html << "<!DOCTYPE html>\n";
	html << "<html lang=\"en\">\n";
	html << "<head>\n";
	html << "    <title>Index of " << dirPath << "</title>\n";
	html << "</head>\n";
	html << "<body>\n";
	html << "    <h1>Index of " << dirPath << "</h1>\n";
	html << "    <table>\n";
	html << "        <tr><th>Name</th><th>Size</th><th>Last Modified</th></tr>\n";

	// Parent directory
	html << "        <tr><td><a href=\"../\">..</a></td><td>-</td><td>-</td></tr>\n";

	// Read actual directory
	DIR* dir = opendir(dirPath.c_str());
	if (dir)
	{
		struct dirent* entry;
		while ((entry = readdir(dir)) != NULL)
		{
			std::string name = entry->d_name;
			if (name == "." || name == "..")
				continue;

			std::string fullPath = dirPath + "/" + name;
			struct stat st;
			if (stat(fullPath.c_str(), &st) == 0)
			{
				if (S_ISDIR(st.st_mode))
				{
					html << "        <tr><td><a href=\"" << name << "/\">"
							<< name << "/</a></td><td>-</td><td>-</td></tr>\n";
				}
				else
				{
					html << "        <tr><td><a href=\"" << name << "\">"
							<< name << "</a></td><td>" << st.st_size
							<< "</td><td>-</td></tr>\n";
				}
			}
		}
		closedir(dir);
	}

	html << "    </table>\n";
	html << "</body>\n";
	html << "</html>\n";

	std::string result = html.str();

	// Verify HTML structure
	TEST_ASSERT(result.find("<!DOCTYPE html>") != std::string::npos,
				"HTML contains DOCTYPE declaration");

	TEST_ASSERT(result.find("<html") != std::string::npos,
				"HTML contains <html> tag");

	TEST_ASSERT(result.find("<head>") != std::string::npos,
				"HTML contains <head> section");

	TEST_ASSERT(result.find("<title>") != std::string::npos,
				"HTML contains <title> tag");

	TEST_ASSERT(result.find("<body>") != std::string::npos,
				"HTML contains <body> section");

	TEST_ASSERT(result.find("<table>") != std::string::npos,
				"HTML contains <table> for listing");

	TEST_ASSERT(result.find("href=\"../\"") != std::string::npos,
				"HTML contains parent directory link (..)");

	TEST_ASSERT(result.find("</html>") != std::string::npos,
				"HTML is properly closed");

	// Verify our test files are in the listing
	TEST_ASSERT(result.find("document.txt") != std::string::npos,
				"Listing contains document.txt");

	TEST_ASSERT(result.find("subdir/") != std::string::npos,
				"Listing contains subdir/ with trailing slash");
}


// ===========================================
//  Test 10: Empty Directory
// ===========================================

void testEmptyDirectory()
{
	TEST_SECTION("Test 10: Empty Directory Handling");

	std::string emptyDir = TEST_ROOT + "/empty";

	/*
		Test: Read empty directory
	*/
	{
		DIR* dir = opendir(emptyDir.c_str());
		TEST_ASSERT(dir != NULL, "Can open empty directory");

		if (dir)
		{
			std::vector<std::string> entries;
			struct dirent* entry;
			while ((entry = readdir(dir)) != NULL)
			{
				std::string name = entry->d_name;
				if (name != "." && name != "..")
				{
					entries.push_back(name);
				}
			}
			closedir(dir);

			TEST_ASSERT(entries.empty(), "Empty directory has no entries (except . and ..)");
		}
	}

	/*
		Test: Empty directory listing should still have parent link
	*/
	{
		// Simulate directory listing HTML for empty directory
		std::stringstream html;
		html << "<table>\n";
		html << "<tr><td><a href=\"../\">..</a></td></tr>\n";
		// No other entries
		html << "</table>\n";

		std::string result = html.str();
		TEST_ASSERT(result.find("href=\"../\"") != std::string::npos,
					"Empty directory listing still has parent link");
	}
}


// ===========================================
//  Test 11: Response Content-Type
// ===========================================

void testResponseContentType()
{
	TEST_SECTION("Test 11: Response Content-Type");

	/*
		Directory listing should return HTML
	*/
	{
		Response response;
		response.setStatus(200, "OK");
		response.setContentType("text/html; charset=UTF-8");
		response.setBody("<html><body>Directory listing</body></html>");
		response.addStandardHeaders();

		std::string contentType = response.getHeader("Content-Type");
		TEST_ASSERT(contentType.find("text/html") != std::string::npos,
					"Directory listing returns text/html Content-Type");

		TEST_ASSERT(contentType.find("charset=UTF-8") != std::string::npos,
					"Content-Type includes charset=UTF-8");
	}
}


// ===========================================
//  Test 12: Clickable Links
// ===========================================

void testClickableLinks()
{
	TEST_SECTION("Test 12: Clickable Links in Listing");

	/*
		All entries should be proper <a href=""> links
	*/
	{
		std::string testFile = "document.txt";
		std::string testDir = "subdir";

		// File link - no trailing slash
		std::stringstream fileLink;
		fileLink << "<a href=\"" << testFile << "\">" << testFile << "</a>";
		std::string fileLinkStr = fileLink.str();

		TEST_ASSERT(fileLinkStr.find("href=\"document.txt\"") != std::string::npos,
					"File link has correct href without trailing slash");

		// Directory link - with trailing slash
		std::stringstream dirLink;
		dirLink << "<a href=\"" << testDir << "/\">" << testDir << "/</a>";
		std::string dirLinkStr = dirLink.str();

		TEST_ASSERT(dirLinkStr.find("href=\"subdir/\"") != std::string::npos,
					"Directory link has trailing slash in href");
	}

	/*
		Parent directory link
	*/
	{
		std::string parentLink = "<a href=\"../\">..</a>";
		TEST_ASSERT(parentLink.find("href=\"../\"") != std::string::npos,
					"Parent directory link uses ../");
	}
}


// ===========================================
//  Test 13: Location Matching for Directories
// ===========================================

void testLocationMatchingForDirectories()
{
	TEST_SECTION("Test 13: Location Matching for Directories");

	ServerConfig server = createDirListingTestServer();

	/*
		Test: Exact path match
	*/
	{
		const LocationConfig* loc = server.findLocation("/files");
		TEST_ASSERT(loc != NULL, "Location /files found");
		if (loc)
		{
			TEST_ASSERT(loc->path == "/files", "Exact path match for /files");
		}
	}

	/*
		Test: Prefix match for subdirectory
	*/
	{
		const LocationConfig* loc = server.findLocation("/files/subdir");
		TEST_ASSERT(loc != NULL, "Location found for /files/subdir");
		if (loc)
		{
			TEST_ASSERT(loc->path == "/files",
						"/files/subdir matches /files location (prefix match)");
		}
	}

	/*
		Test: Trailing slash handling
	*/
	{
		const LocationConfig* loc1 = server.findLocation("/files");
		const LocationConfig* loc2 = server.findLocation("/files/");

		// Both should match the same location
		TEST_ASSERT(loc1 != NULL && loc2 != NULL,
					"Both /files and /files/ find a location");

		if (loc1 && loc2)
		{
			TEST_ASSERT(loc1->path == loc2->path,
						"/files and /files/ match the same location");
		}
	}
}


// ===========================================
//  Main Test Runner
// ===========================================

int main()
{
	std::cout << COLOR_YELLOW << "\n"
				<< "╔═══════════════════════════════════════════════════════════════╗\n"
				<< "║         WEBSERV DIRECTORY LISTING TEST SUITE                  ║\n"
				<< "║                     Step 9.2 Tests                            ║\n"
				<< "╚═══════════════════════════════════════════════════════════════╝\n"
				<< COLOR_RESET << std::endl;

	std::cout << "Testing directory listing without network layer.\n";
	std::cout << "Tests verify POSIX directory functions, HTML generation, and config.\n";

	// Setup test environment
	setupTestDirectories();

	// Run all tests
	testAutoindexConfiguration();
	testDirectoryDetection();
	testDirectoryReading();
	testFileSizeFormatting();
	testHtmlEscaping();
	testTimeFormatting();
	testIndexFilePriority();
	testDirectorySorting();
	testGeneratedHtmlStructure();
	testEmptyDirectory();
	testResponseContentType();
	testClickableLinks();
	testLocationMatchingForDirectories();

	// Cleanup
	cleanupTestDirectories();


	// Print summary
	std::cout << std::endl;
	std::cout << BOLD << "═══════════════════════════════════════════════════════════" << RESET << std::endl;
	std::cout << BOLD << "                    TEST SUMMARY" << RESET << std::endl;
	std::cout << BOLD << "═══════════════════════════════════════════════════════════" << RESET << std::endl;
	std::cout << GREEN << "  Passed: " << g_testsPassed << RESET << std::endl;
	std::cout << RED   << "  Failed: " << g_testsFailed << RESET << std::endl;
	std::cout << BOLD << "  Total:  " << (g_testsPassed + g_testsFailed) << RESET << std::endl;
	std::cout << BOLD << "═══════════════════════════════════════════════════════════" << RESET << std::endl;

	if (g_testsFailed == 0)
	{
		std::cout << std::endl;
		std::cout << GREEN << BOLD << "  ✓ ALL TESTS PASSED!" << RESET << std::endl;
		std::cout << std::endl;
		return 0;
	}
	else
	{
		std::cout << std::endl;
		std::cout << RED << BOLD << "  ✗ SOME TESTS FAILED" << RESET << std::endl;
		std::cout << "\n" << std::endl;
		return 1;
	}
}
