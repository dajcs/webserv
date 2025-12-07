#include "Config.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        // Determine config file path
        std::string configPath = "config/default.conf";
        if (argc > 1) {
            configPath = argv[1];
        }

        std::cout << "Loading configuration from: " << configPath << std::endl;

        // Parse the configuration file
        Config config(configPath);

        // Display parsed configuration for verification
        config.printConfig();

        std::cout << "\nConfiguration parsed successfully!" << std::endl;

        // Example: find a location
        const std::vector<ServerConfig>& servers = config.getServers();
        if (!servers.empty()) {
            const LocationConfig* loc = servers[0].findLocation("/cgi-bin/test.py");
            if (loc) {
                std::cout << "\nLocation for /cgi-bin/test.py: " << loc->path << std::endl;
            }
        }

        return 0;
    }
    catch (const ConfigException& e) {
        std::cerr << "Configuration error: " << e.what() << std::endl;
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
