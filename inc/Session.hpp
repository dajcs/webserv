/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Session.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 10:39:45 by anemet            #+#    #+#             */
/*   Updated: 2026/01/09 11:04:20 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SESSION_HPP
#define SESSION_HPP

#include <string>
#include <map>
#include <cstdlib>
#include <ctime>
#include <sstream>

/*
	SessionManager Class
	--------------
	A singleton class to manage server-side session data.
	It maps a SessionID (string) to a map of Key-Value pairs.
*/
class SessionManager
{
	public:
		// Singleton access
		static SessionManager& getInstance();

		// Generate a new session ID and create and empty entry
		std::string createSession();

		// Check if a session ID exists
		bool isValid(const std::string& sessionId);

		// Set data for a specific session
		void setData(const std::string& sessionId, const std::string& key,
			const std::string& value);

		// Get data for a specific session
		std::string getData(const std::string& sessionId, const std::string& key);

		// Remove a session (logout)
		void killSession(const std::string& sessionId);

		// Clean up old sessions (optional implementation for timeout)
		void cleanup();

	private:
		SessionManager(); // Private constructor
		~SessionManager();

		// Map: SessionId -> (Map: DataKey - DataValue)
		std::map<std::string, std::map<std::string, std::string> > _sessions;

		// Helper to generate random string
		std::string generateId();
};












#endif
