Here is a complete implementation guide to adding Cookies and Session Management to your `webserv`.

### Concepts: Web Server Basics

1.  **Statelessness:** HTTP is a **stateless** protocol. This means the server treats every request as a completely new transaction. It doesn't remember who you are from one request to the next.
2.  **Cookies:** To solve statelessness, we use Cookies. A Cookie is a small piece of data sent from the Server (via `Set-Cookie` header) and stored in the Client's browser. The browser automatically sends this data back in the `Cookie` header for every subsequent request to that domain.
3.  **Sessions:** Cookies shouldn't store sensitive data (like "is_admin=true") because users can modify them. Instead, we use **Sessions**.
    *   The server generates a unique, random string called a **Session ID**.
    *   The server sends this ID to the client as a Cookie.
    *   The server keeps a map in memory: `SessionID -> { UserData }`.
    *   When the client sends the ID back, the server looks up the data.

---

### Step 1: Create a Session Manager (`Session.hpp` / `Session.cpp`)

We need a centralized place to store session data in memory. Since your server handles connections in a single-threaded loop (epoll), we can use a Singleton or Static approach safely without mutexes.

**File: `Session.hpp`**
```cpp
#ifndef SESSION_HPP
#define SESSION_HPP

#include <string>
#include <map>
#include <cstdlib>
#include <ctime>
#include <sstream>

/*
    SessionManager
    --------------
    A singleton class to manage server-side session data.
    It maps a SessionID (string) to a map of Key-Value pairs.
*/
class SessionManager {
public:
    // Singleton access
    static SessionManager& getInstance();

    // Generate a new session ID and create an empty entry
    std::string createSession();

    // Check if a session ID exists
    bool isValid(const std::string& sessionId);

    // Set data for a specific session
    void setData(const std::string& sessionId, const std::string& key, const std::string& value);

    // Get data for a specific session
    std::string getData(const std::string& sessionId, const std::string& key);

    // Remove a session (logout)
    void killSession(const std::string& sessionId);

    // Clean up old sessions (optional implementation for timeout)
    void cleanup();

private:
    SessionManager(); // Private constructor
    ~SessionManager();

    // Map: SessionID -> (DataKey -> DataValue)
    std::map<std::string, std::map<std::string, std::string> > _sessions;

    // Helper to generate random string
    std::string generateID();
};

#endif
```

**File: `Session.cpp`**
```cpp
#include "Session.hpp"

SessionManager::SessionManager() {
    // Seed random number generator
    std::srand(std::time(0));
}

SessionManager::~SessionManager() {}

SessionManager& SessionManager::getInstance() {
    static SessionManager instance;
    return instance;
}

std::string SessionManager::generateID() {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    
    std::string tmp_s;
    tmp_s.reserve(32);

    for (int i = 0; i < 32; ++i) {
        tmp_s += alphanum[std::rand() % (sizeof(alphanum) - 1)];
    }
    return tmp_s;
}

std::string SessionManager::createSession() {
    std::string id = generateID();
    // Ensure uniqueness (extremely unlikely to collide, but good practice)
    while (_sessions.find(id) != _sessions.end()) {
        id = generateID();
    }
    // Create empty map for this session
    _sessions[id] = std::map<std::string, std::string>();
    return id;
}

bool SessionManager::isValid(const std::string& sessionId) {
    return _sessions.find(sessionId) != _sessions.end();
}

void SessionManager::setData(const std::string& sessionId, const std::string& key, const std::string& value) {
    if (isValid(sessionId)) {
        _sessions[sessionId][key] = value;
    }
}

std::string SessionManager::getData(const std::string& sessionId, const std::string& key) {
    if (isValid(sessionId)) {
        if (_sessions[sessionId].find(key) != _sessions[sessionId].end()) {
            return _sessions[sessionId][key];
        }
    }
    return "";
}

void SessionManager::killSession(const std::string& sessionId) {
    _sessions.erase(sessionId);
}

void SessionManager::cleanup() {
    // In a real server, you would store timestamps and delete old sessions here
}
```

---

### Step 2: Update `Request.hpp` / `.cpp` to Parse Cookies

We need to extract the `Cookie` header sent by the browser. The format is usually `Cookie: session_id=abc; user=bob`.

**Update `Request.hpp`**: Add a method to get a specific cookie.
```cpp
// Inside Request class public section:
std::string getCookie(const std::string& name) const;
```

**Update `Request.cpp`**: Implement the parsing logic.
```cpp
std::string Request::getCookie(const std::string& name) const {
    // 1. Get the raw Cookie header
    std::string cookieHeader = getHeader("Cookie");
    if (cookieHeader.empty()) {
        return "";
    }

    // 2. Cookies are separated by "; "
    size_t pos = 0;
    while (pos < cookieHeader.length()) {
        size_t end = cookieHeader.find(';', pos);
        if (end == std::string::npos) {
            end = cookieHeader.length();
        }

        // Extract "key=value" string
        std::string pair = cookieHeader.substr(pos, end - pos);
        
        // Trim leading spaces
        size_t startName = pair.find_first_not_of(" \t");
        if (startName != std::string::npos) {
            pair = pair.substr(startName);
        }

        // Find the equals sign
        size_t eqPos = pair.find('=');
        if (eqPos != std::string::npos) {
            std::string key = pair.substr(0, eqPos);
            if (key == name) {
                return pair.substr(eqPos + 1);
            }
        }

        pos = end + 1;
    }
    return "";
}
```

---

### Step 3: Update `Response.hpp` / `.cpp` to Set Cookies

We need to send the `Set-Cookie` header. Note that your `_headers` is a `std::map`. Maps overwrite keys. Technically, HTTP allows multiple `Set-Cookie` headers. For this 42 project, sending one cookie (the session ID) is usually enough. If you need multiple, you'll need to change `_headers` to a `std::multimap` or handle `Set-Cookie` specially in `build()`.

For this example, we assume we only set the Session Cookie.

**Update `Response.hpp`**:
```cpp
// Inside public section:
void setCookie(const std::string& name, const std::string& value, int maxAge = 3600);
```

**Update `Response.cpp`**:
```cpp
void Response::setCookie(const std::string& name, const std::string& value, int maxAge) {
    std::stringstream ss;
    // Basic Cookie Format: name=value; Path=/; Max-Age=seconds
    // HttpOnly: JavaScript cannot access this cookie (Security)
    ss << name << "=" << value << "; Path=/; HttpOnly; Max-Age=" << maxAge;
    setHeader("Set-Cookie", ss.str());
}
```

---

### Step 4: Implement Logic in `Router.cpp`

Now we create specific routes to demonstrate the functionality.

**1. Create a Login Handler:**
This checks credentials (hardcoded for demo), creates a session, and sets the cookie.

**2. Create a Dashboard Handler:**
This checks the cookie. If valid, it shows private data. If invalid, it redirects to login.

**Modify `Router.cpp`**:

First, include the Session Manager:
```cpp
#include "Session.hpp"
```

Add these methods to `Router.hpp` (private) and implement in `Router.cpp`:
```cpp
// In Router.hpp private:
Response handleLogin(const Request& request);
Response handleDashboard(const Request& request);
Response handleLogout(const Request& request);
```

**Implementation in `Router.cpp`**:

Inside `Router::route`, add logic to detect these paths. Typically, you would add these to your `findLocation` logic or check specific paths before generic handling.

```cpp
// Inside Router::route method, before general GET/POST handling:
// Note: In a real config, these would be specific location blocks.
// We are hardcoding specific paths here for the DEMO.

std::string path = request.getPath();

if (path == "/login") {
    return handleLogin(request);
}
else if (path == "/dashboard") {
    return handleDashboard(request);
}
else if (path == "/logout") {
    return handleLogout(request);
}
// ... existing logic follows ...
```

**The Handlers:**

```cpp
/*
    handleLogin
    -----------
    1. If GET: Show a simple HTML form.
    2. If POST: Check username/password.
       - If valid: Create Session, Set Cookie, Redirect to dashboard.
       - If invalid: Show error.
*/
Response Router::handleLogin(const Request& request) {
    if (request.getMethod() == "GET") {
        std::string html = 
            "<html><body>"
            "<h1>Login</h1>"
            "<form method='POST' action='/login'>"
            "User: <input type='text' name='user'><br>"
            "<input type='submit' value='Login'>"
            "</form>"
            "</body></html>";
        return Response::ok(html);
    }
    else if (request.getMethod() == "POST") {
        // Parse form data (assuming you have Utils::parseFormUrlEncoded)
        std::map<std::string, std::string> formData = Utils::parseFormUrlEncoded(request.getBody());
        
        // Simple hardcoded credential check
        if (formData["user"] == "admin") {
            // 1. Create Session
            std::string sessionId = SessionManager::getInstance().createSession();
            
            // 2. Store data in session
            SessionManager::getInstance().setData(sessionId, "user", "admin");
            SessionManager::getInstance().setData(sessionId, "role", "superuser");

            // 3. Create Response
            Response res = Response::redirect(302, "/dashboard");
            
            // 4. Set the Cookie header
            res.setCookie("webserv_session", sessionId);
            
            return res;
        } else {
            return Response::error(403, "Invalid Credentials");
        }
    }
    return Response::error(405); // Method Not Allowed
}

/*
    handleDashboard
    ---------------
    1. Check if 'webserv_session' cookie exists.
    2. Check if that session ID is valid in SessionManager.
    3. If yes: Show protected content.
    4. If no: Redirect to /login.
*/
Response Router::handleDashboard(const Request& request) {
    // 1. Extract Cookie
    std::string sessionId = request.getCookie("webserv_session");

    // 2. Validate Session
    if (sessionId.empty() || !SessionManager::getInstance().isValid(sessionId)) {
        return Response::redirect(302, "/login");
    }

    // 3. Retrieve Data
    std::string user = SessionManager::getInstance().getData(sessionId, "user");
    std::string role = SessionManager::getInstance().getData(sessionId, "role");

    // 4. Render Page
    std::stringstream ss;
    ss << "<html><body>";
    ss << "<h1>Welcome, " << user << "!</h1>";
    ss << "<p>Your role is: " << role << "</p>";
    ss << "<p>Session ID: " << sessionId << "</p>";
    ss << "<a href='/logout'>Logout</a>";
    ss << "</body></html>";

    return Response::ok(ss.str());
}

/*
    handleLogout
    ------------
    1. Get Session ID.
    2. Remove from SessionManager.
    3. Expire the cookie (Set-Cookie with Max-Age=0).
    4. Redirect to login.
*/
Response Router::handleLogout(const Request& request) {
    std::string sessionId = request.getCookie("webserv_session");
    
    if (!sessionId.empty()) {
        SessionManager::getInstance().killSession(sessionId);
    }

    Response res = Response::redirect(302, "/login");
    // Overwrite cookie with immediate expiration
    res.setCookie("webserv_session", "", 0);
    
    return res;
}
```

### Summary of what was done

1.  **SessionManager**: Created a central memory store (`std::map`) to hold user states, identified by a random string ID.
2.  **Request**: Added ability to parse the raw `Cookie: ...` string into usable values.
3.  **Response**: Added ability to format and set the `Set-Cookie` header.
4.  **Router**: Implemented a logic flow where:
    *   **Login** creates the state and gives the client the "key" (cookie).
    *   **Dashboard** demands the "key" to show content.
    *   **Logout** destroys the state and throws away the "key".

This fits perfectly within your existing C++98 architecture and satisfies the project requirements for implementing cookies and session management.
