/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:57:21 by anemet            #+#    #+#             */
/*   Updated: 2025/12/08 15:02:43 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>

class Response {
public:
    Response();
    ~Response();
    Response(const Response& other);
    Response& operator=(const Response& other);

    // Set response components
    void setStatus(int code);
    void setStatus(int code, const std::string& reason);
    void setHeader(const std::string& name, const std::string& value);
    void setBody(const std::string& body);
    void setBody(const char* data, size_t length);

    // Convenience methods
    void setContentType(const std::string& type);
    void setContentLength(size_t length);
    void setConnection(bool keepAlive);

    // Build the complete HTTP response string
    std::string build() const;

    // Get raw response data for sending
    const std::string& getData() const;
    size_t getSize() const;

    // Getters
    int getStatusCode() const;
    const std::string& getBody() const;
    bool shouldKeepAlive() const;

    // Static factory methods for common responses
    static Response ok(const std::string& body, const std::string& contentType = "text/html");
    static Response error(int code);
    static Response redirect(int code, const std::string& location);

    // Static helpers
    static std::string getReasonPhrase(int code);
    static std::string getMimeType(const std::string& extension);

private:
    int _statusCode;
    std::string _reasonPhrase;
    std::map<std::string, std::string> _headers;
    std::string _body;
    mutable std::string _builtResponse;
    mutable bool _dirty;
    bool _keepAlive;

    void buildIfNeeded() const;
};

#endif
