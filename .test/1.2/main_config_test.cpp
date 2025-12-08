/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main_config_test.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:54:34 by anemet            #+#    #+#             */
/*   Updated: 2025/12/08 23:54:53 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Config.hpp"
#include <iostream>

int main(int argc, char* argv[])
{
	try
	{
		// Determine config file path
		std::string configPath = "config/default.conf";
		if (argc > 1)
		{
			configPath = argv[1];
		}

		std::cout << "loading configuration from: " << configPath << std::endl;

		// Parse the configuration file
		Config config(configPath);

		// Display parsed configuration for verification
		config.printConfig();

		std::cout << "\nConfiugration parsed successfully!" << std::endl;

		// Example: find a location
		const std::vector<ServerConfig>& servers = config.getServers();
		if (!servers.empty())
		{
			const LocationConfig* loc = servers[0].findLocation("/cgi-bin/test.py");
			if (loc)
			{
				std::cout << "\nLocation for /cgi-bin/test.py: " << loc->path << std::endl;
			}
		}

		return 0;
	}
	catch(const std::exception& e)
	{
		std::cerr << "Configuration error: " << e.what() << '\n';
		return 1;
	}
}
