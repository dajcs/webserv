To support virtual hosting (multiple server blocks on the same port with different hostnames), you need to:

1. **Update Config validation** to allow same port with different `server_name`
2. **Update server/connection handling** to route based on `Host` header
3. **Update Router** to find server by hostname, not just port

Here are the required changes:

## 1. Update Config Validation

````cpp
// In validateConfig(), replace the duplicate address check logic:

void Config::validateConfig() const
{
    // Track host:port -> list of server_names for virtual hosting
    std::map<std::pair<std::string, int>, std::set<std::string> > usedAddresses;

    for (size_t i = 0; i < _servers.size(); ++i)
    {
        const ServerConfig& server = _servers[i];

        // Validate port number
        if (server.port <= 0 || server.port > 65535)
        {
            std::ostringstream oss;
            oss << "Invalid port number: " << server.port;
            throw ConfigException(oss.str());
        }

        std::pair<std::string, int> hostPort(server.host, server.port);

        // Get server names (use empty string for default server)
        std::vector<std::string> names = server.server_names;
        if (names.empty())
            names.push_back("");  // Default server marker

        for (size_t j = 0; j < names.size(); ++j)
        {
            if (usedAddresses[hostPort].count(names[j]))
            {
                std::ostringstream oss;
                if (names[j].empty())
                    oss << "Duplicate default server for " << server.host << ":" << server.port;
                else
                    oss << "Duplicate server_name '" << names[j] << "' for "
                        << server.host << ":" << server.port;
                throw ConfigException(oss.str());
            }
            usedAddresses[hostPort].insert(names[j]);
        }

        // ...existing location validation code...
    }
}
````

## 2. Update Router to Find Server by Host Header

````cpp
// ...existing code...

/*
    findServer() - Find server block matching port AND hostname (virtual hosting)

    Virtual hosting allows multiple websites on the same IP:port,
    distinguished by the Host header.

    Example:
        curl --resolve example.com:8080:127.0.0.1 http://example.com/
        -> Host header will be "example.com"
        -> We find the server block with server_name example.com on port 8080
*/
const ServerConfig* Router::findServer(int port, const std::string& hostname) const
{
    if (!_config)
        return NULL;

    const std::vector<ServerConfig>& servers = _config->getServers();
    const ServerConfig* defaultServer = NULL;

    // Extract hostname without port (Host header might be "example.com:8080")
    std::string host = hostname;
    size_t colonPos = host.find(':');
    if (colonPos != std::string::npos)
        host = host.substr(0, colonPos);

    for (size_t i = 0; i < servers.size(); ++i)
    {
        if (servers[i].port != port)
            continue;

        // Check if this server matches the hostname
        const std::vector<std::string>& names = servers[i].server_names;

        for (size_t j = 0; j < names.size(); ++j)
        {
            if (names[j] == host)
                return &servers[i];  // Exact match found
        }

        // Remember first server on this port as default
        if (!defaultServer)
            defaultServer = &servers[i];
    }

    // No exact match - return default server for this port
    return defaultServer;
}

// Update the route() function to use Host header
Response Router::route(const Request& request, int serverPort)
{
    // ...existing code for config check...

    // Get Host header for virtual hosting
    std::string hostHeader = request.getHeader("Host");

    // Find server using both port AND hostname
    const ServerConfig* server = findServer(serverPort, hostHeader);

    if (!server)
    {
        return createErrorResponse(500, NULL);
    }

    // ...rest of existing route() code...
}
````

## 3. Update Router Header

````cpp
// ...existing code...

class Router
{
public:
    // ...existing public methods...

private:
    // Update findServer signature
    const ServerConfig* findServer(int port, const std::string& hostname = "") const;

    // ...rest of existing private members...
};
````

## 4. Test with curl

After implementing, test with:

```bash
# Test first virtual host (localhost)
curl http://localhost:8080/

# Test second virtual host (example.com resolved to 127.0.0.1)
curl --resolve example.com:8080:127.0.0.1 http://example.com:8080/
```

The `--resolve` flag tells curl to resolve `example.com:8080` to `127.0.0.1`, so it connects to your local server but sends `Host: example.com` in the request header.
