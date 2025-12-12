### a very commented handleDelete implementation

```cpp
/*
    ============================================================================
    handleDelete() - Handle HTTP DELETE requests
    ============================================================================

    DELETE Method Overview (RFC 7231, Section 4.3.5):
    -------------------------------------------------
    The DELETE method requests that the origin server remove the association
    between the target URI and its current functionality.

    In simple terms: DELETE removes a file from the server's filesystem.

    How DELETE fits into HTTP semantics:
    - GET:    Read a resource     (safe, idempotent)
    - POST:   Create a resource   (not safe, not idempotent)
    - PUT:    Replace a resource  (not safe, idempotent)
    - DELETE: Remove a resource   (not safe, idempotent)

    "Idempotent" means: calling DELETE twice on the same resource has the same
    effect as calling it once. The file is deleted, and subsequent DELETEs
    just return 404 (resource no longer exists).


    Security Considerations:
    ------------------------
    DELETE is a DANGEROUS operation! We must implement multiple layers of security:

    1. Method Allowlist:
       - DELETE must be explicitly allowed in the location config
       - Default behavior: DELETE is NOT allowed
       - Config example: allowed_methods DELETE;

    2. Path Validation:
       - Prevent directory traversal attacks (../)
       - Ensure deletion only happens within allowed directories
       - Never allow deleting files outside the document root

    3. File Type Restrictions:
       - Don't allow deleting directories (could wipe entire folders)
       - Some implementations allow directory deletion with special flags
       - We follow the conservative approach: files only

    4. Permission Checks:
       - Check Unix permissions before attempting deletion
       - Return appropriate error codes for permission issues


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

    Input:
        request:  The parsed HTTP request object
        location: The matching location configuration

    Returns:
        Response object with appropriate status code
*/
Response Router::handleDelete(const Request& request, const LocationConfig& location)
{
    /*
        Step 1: Resolve the filesystem path
        -----------------------------------
        Convert the URI (e.g., "/uploads/file.txt") to an absolute filesystem
        path (e.g., "/var/www/uploads/file.txt").

        The resolvePath() function handles:
        - Combining location root with request path
        - Sanitizing path (removing ".." for security)
        - Ensuring the path stays within the document root
    */
    std::string path = resolvePath(request.getPath(), location);

    /*
        Step 2: Security Check - Verify path is within allowed directory
        -----------------------------------------------------------------
        CRITICAL: Ensure we're not deleting files outside the configured root.

        Attack scenario (Directory Traversal):
          DELETE /uploads/../../../etc/passwd HTTP/1.1

        Without this check, an attacker could delete system files!

        We verify that the resolved path starts with the location root.
        This is a defense-in-depth measure - resolvePath() should already
        sanitize the path, but we double-check here.
    */
    std::string allowedRoot = location.root;

    // Normalize: ensure root ends without slash for consistent comparison
    if (!allowedRoot.empty() && allowedRoot[allowedRoot.length() - 1] == '/')
    {
        allowedRoot = allowedRoot.substr(0, allowedRoot.length() - 1);
    }

    // Check if resolved path starts with the allowed root
    // This prevents accessing files outside the configured directory
    if (path.compare(0, allowedRoot.length(), allowedRoot) != 0)
    {
        /*
            Path is outside allowed directory!
            Return 403 Forbidden - we understand the request but refuse to process it.

            Why 403 and not 404?
            - 404 would leak information (attacker knows path exists)
            - 403 is honest: "I won't do that" (security best practice)
        */
        return errorResponse(403);
    }

    /*
        Step 3: Check if the file exists
        ---------------------------------
        Use stat() to get information about the file.

        stat() is a POSIX system call that returns:
        - File existence
        - File type (regular file, directory, symlink, etc.)
        - File permissions
        - File size, timestamps, etc.

        If stat() fails, the file doesn't exist (or we can't access it).
    */
    struct stat pathStat;
    if (stat(path.c_str(), &pathStat) != 0)
    {
        /*
            File not found - return 404 Not Found

            HTTP Semantics: DELETE on non-existent resource should return 404.

            Some APIs return 204 for "already deleted" (idempotency),
            but standard behavior is 404 - the resource is not there.

            Note: errno could tell us more (ENOENT vs EACCES), but the
            subject forbids checking errno after I/O operations.
        */
        return errorResponse(404);
    }

    /*
        Step 4: Verify it's a regular file, not a directory
        ----------------------------------------------------
        S_ISDIR() macro checks if the stat result indicates a directory.

        Why not allow directory deletion?
        1. Directories might contain other files - deleting is recursive
        2. Accidental directory deletion could wipe entire websites
        3. POSIX unlink() doesn't work on directories anyway
        4. Deleting directories requires rmdir() (empty) or recursive deletion

        Return 409 Conflict because:
        - The request is understood and valid
        - But it conflicts with the current state (it's a directory)
        - Client could potentially fix this (send recursive delete flag)
    */
    if (S_ISDIR(pathStat.st_mode))
    {
        /*
            409 Conflict - request conflicts with resource state

            Alternative interpretations:
            - 400 Bad Request: "you can't DELETE a directory"
            - 403 Forbidden:   "not allowed to delete directories"

            We use 409 because the conflict is with the resource type,
            not with permissions or request format.
        */
        return errorResponse(409);
    }

    /*
        Step 5: Check if we have write permission
        -----------------------------------------
        Before attempting deletion, check if the file is deletable.

        To delete a file, we need write permission on the PARENT DIRECTORY
        (not the file itself). This is because deletion modifies the directory
        entry, not the file content.

        However, we use access() on the file itself as a simpler check.
        The actual unlink() call will fail if we don't have permission.

        R_OK = read permission
        W_OK = write permission
        X_OK = execute permission
        F_OK = file exists

        Note: access() checks permissions for the real UID/GID, not effective.
        For setuid programs this matters, but our server runs as the user.
    */
    if (access(path.c_str(), W_OK) != 0)
    {
        /*
            No write permission - return 403 Forbidden

            This is a permission issue, not a "file not found" issue.
            The client knows the file exists but can't delete it.
        */
        return errorResponse(403);
    }

    /*
        Step 6: Attempt to delete the file
        -----------------------------------
        unlink() removes the directory entry for the file.

        How unlink() works:
        1. Removes the name from the directory
        2. Decrements the link count of the inode
        3. If link count reaches 0 AND no process has file open, free space
        4. If file is still open by a process, deletion is delayed

        This means: a file can be "deleted" but still exist until all
        processes close it. This is normal Unix behavior.

        Return values:
        - 0:  Success
        - -1: Error (reason in errno, but we can't check it)
    */
    if (unlink(path.c_str()) != 0)
    {
        /*
            Deletion failed!

            Possible reasons (we can't check errno):
            - EACCES: Permission denied (directory not writable)
            - EBUSY:  File is in use (some systems)
            - EIO:    I/O error
            - ENOENT: File was deleted between stat() and unlink()
            - EPERM:  File has immutable flag
            - EROFS:  Read-only filesystem

            We return 403 Forbidden as a general "can't do that" response.
            A more sophisticated server might return different codes based
            on errno, but the subject forbids checking errno.
        */
        return errorResponse(403);
    }

    /*
        Step 7: Success! Return 204 No Content
        --------------------------------------
        The file has been successfully deleted.

        Why 204 No Content?
        - The operation succeeded
        - There's nothing meaningful to return in the body
        - 204 specifically means "success, no body"

        Alternative: 200 OK with a body confirming deletion
          {"status": "deleted", "path": "/uploads/file.txt"}

        But 204 is cleaner and more RESTful for DELETE operations.

        204 Response characteristics:
        - MUST NOT include a message body
        - Content-Length should be absent or 0
        - Still includes standard headers (Date, Server, Connection)
    */
    return Response::noContent();
}
```
