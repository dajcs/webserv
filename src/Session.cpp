/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Session.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 11:04:02 by anemet            #+#    #+#             */
/*   Updated: 2026/01/09 13:40:53 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


/*
	1. HTTP is a stateless protocol: it doesn't remember the user data from one
		request to the next.
	2. Cookies: To solve statelessness, we use cookies. A cookie is a small
		piece of data sent from the Server (via `Set-Cookie` header) and stored
		in the Client's browser. The browser automatically sends this data back
		in the `Cookie` header for every subsequent request to that domain.
	3. Sessions:
		- The server generates a unique, random string called `SessionID`
		- The server sends this ID to the client as a Cookie
		- The server keeps a map in memory: `SessionID -> { UserData }`
		- When the client sends the ID back, the server looks up the data
*/

#include "Session.hpp"

SessionManager::SessionManager()
{
	// Seed random number generator
	std::srand(std::time(0));
}

SessionManager::~SessionManager() {}

SessionManager& SessionManager::getInstance()
{
	static SessionManager instance;
	return instance;
}

std::string SessionManager::generateId()
{
	static const char alphanum[] =
		"0123456789"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz";

	std::string tmp_s;
	tmp_s.reserve(32);

	for (int i = 0; i < 32; ++i)
	{
		tmp_s += alphanum[std::rand() % (sizeof(alphanum) - 1)];
	}
	return tmp_s;
}

std::string SessionManager::createSession()
{
	std::string id = generateId();
	// Ensure uniqueness (extremely unlikely to collide,
	// but good practice)
	while (_sessions.find(id) != _sessions.end())
	{
		id = generateId();
	}
	// Create empty map for this session
	_sessions[id] = std::map<std::string, std::string>();
	return id;
}

bool SessionManager::isValid(const std::string& sessionId)
{
	return _sessions.find(sessionId) != _sessions.end();
}

void SessionManager::setData(const std::string& sessionId,
						const std::string& key, const std::string& value)
{
	if (isValid(sessionId))
	{
		_sessions[sessionId][key] = value;
	}
}

std::string SessionManager::getData(const std::string& sessionId,
												const std::string& key)
{
	if (isValid(sessionId))
	{
		if (_sessions[sessionId].find(key) != _sessions[sessionId].end())
		{
			return _sessions[sessionId][key];
		}
	}
	return "";
}

void SessionManager::killSession(const std::string& sessionId)
{
	_sessions.erase(sessionId);
}

void SessionManager::cleanup()
{
	// In a real server we can store timestamps and delete old sessions here
}
