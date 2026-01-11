# Ubuntu Tester Configuration Fix

The tester expects specific behavior for `/directory`. Let me break down the requirements:

## Requirements from the Tester

1. **`/directory/`** must answer to GET request
2. **Root** should be the YoupiBanane repository
3. **If no file is requested**, it should search for `youpi.bad_extension` files (this means `index` should be `youpi.bad_extension`)

## The Problem

The test `GET http://localhost:8080/directory` (without trailing slash) is failing. This is likely because:
1. NGINX-style servers typically redirect `/directory` → `/directory/` for directories
2. The tester might expect either a redirect (301/302) or direct content

## Solution: Add `/directory` Location

Add this location block to your config:

````conf
# ...existing code...

# Ubuntu tester requirements
# /directory/ must serve from YoupiBanane with index youpi.bad_extension
location /directory {
    root www/YoupiBanane;
    index youpi.bad_extension;
    allowed_methods GET;
    autoindex off;
}

# ...existing code...
````

## Wait - There's a Path Resolution Issue!

Based on your earlier bug, there's a conflict. The tester expects:

| Request | Expected Filesystem Path |
|---------|-------------------------|
| `/directory` | `www/YoupiBanane/` (serve `youpi.bad_extension`) |
| `/directory/` | `www/YoupiBanane/` (serve `youpi.bad_extension`) |
| `/directory/nop/` | `www/YoupiBanane/nop/` |

But with NGINX-style `root + requestPath`:
- `/directory` → `www/YoupiBanane/directory` ❌ WRONG!

## The Real Fix: NGINX `root` vs `alias` Behavior

NGINX has two directives:
- **`root`**: `result = root + full_request_path`
- **`alias`**: `result = alias + remainder_after_location`

Your tester expects **`alias`-like behavior** for `/directory`:

```
location /directory → root = YoupiBanane
request /directory/nop/file → YoupiBanane/nop/file (NOT YoupiBanane/directory/nop/file)
```

## Two Options

### Option 1: Use `alias`-style Resolution (Recommended)

Modify your `resolvePath` to strip the location prefix:

````cpp
std::string Router::resolvePath(const std::string& requestPath,
                                 const LocationConfig& location) const
{
    std::string root = location.root;
    std::string locPath = location.path;

    // Remove trailing slashes for consistent comparison
    if (!root.empty() && root[root.length() - 1] == '/')
        root = root.substr(0, root.length() - 1);
    if (!locPath.empty() && locPath[locPath.length() - 1] == '/')
        locPath = locPath.substr(0, locPath.length() - 1);

    // Extract the remainder after the location path
    std::string remainder;
    if (requestPath.length() > locPath.length())
        remainder = requestPath.substr(locPath.length());
    else if (requestPath == locPath)
        remainder = "";
    else
        remainder = requestPath;  // Fallback (shouldn't happen if location matched)

    // Ensure remainder starts with / or is empty
    if (!remainder.empty() && remainder[0] != '/')
        remainder = "/" + remainder;

    // Build final path
    std::string result;
    if (remainder.empty() || remainder == "/")
        result = root;  // Just the root directory
    else
        result = root + remainder;

    // Handle trailing slash from request
    if (!requestPath.empty() && requestPath[requestPath.length() - 1] == '/'
        && (result.empty() || result[result.length() - 1] != '/'))
        result += "/";

    // Normalize double slashes
    size_t pos;
    while ((pos = result.find("//")) != std::string::npos)
        result.replace(pos, 2, "/");

    #if DEBUG >= 1
    std::cout << "  [resolvePath] location: " << location.path
              << ", root: " << location.root
              << ", request: " << requestPath
              << ", remainder: " << remainder
              << ", result: " << result << std::endl;
    #endif

    return result;
}
````

### Option 2: Adjust Root in Config

If you want to keep NGINX `root` behavior, set root to parent directory:

````conf
location /directory {
    root www/YoupiBanane/..;  # Hacky, not recommended
    index youpi.bad_extension;
    allowed_methods GET;
}
````

This is ugly. **Use Option 1.**

## Full Config for Ubuntu Tester

````conf
# Ubuntu tester configuration

server {
    listen 8080;
    server_name localhost;

    # / must answer to GET request ONLY
    # Should serve youpi.bad_extension as index
    location / {
        root www/YoupiBanane;
        index youpi.bad_extension;
        allowed_methods GET;
    }

    # /put_test/* must answer to PUT request
    location /put_test {
        root www/uploads;
        allowed_methods PUT;
        upload_path www/uploads;
    }

    # /post_body must answer to POST with maxBody of 100
    location /post_body {
        root www;
        allowed_methods POST;
        client_max_body_size 100;
    }

    # /directory/ - root is YoupiBanane, index is youpi.bad_extension
    location /directory {
        root www/YoupiBanane;
        index youpi.bad_extension;
        allowed_methods GET;
    }

    # *.bla extension must call CGI
    location *.bla {
        root www/YoupiBanane;
        allowed_methods POST;
        cgi_extension .bla;
        cgi_pass www/cgi_test;
    }
}
````

## Expected Path Resolution After Fix

| Request | Location | Root | Remainder | Result |
|---------|----------|------|-----------|--------|
| `/` | `/` | `www/YoupiBanane` | `/` | `www/YoupiBanane` → serve `youpi.bad_extension` |
| `/directory` | `/directory` | `www/YoupiBanane` | `` | `www/YoupiBanane` → serve `youpi.bad_extension` |
| `/directory/` | `/directory` | `www/YoupiBanane` | `/` | `www/YoupiBanane/` → serve `youpi.bad_extension` |
| `/directory/nop/` | `/directory` | `www/YoupiBanane` | `/nop/` | `www/YoupiBanane/nop/` |
