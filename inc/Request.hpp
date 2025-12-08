/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: anemet <anemet@student.42luxembourg.lu>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/07 15:57:13 by anemet            #+#    #+#             */
/*   Updated: 2025/12/08 15:02:21 by anemet           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <map>

class Request {
public:
    // Parsing states
    enum ParseState {
        PARSE_REQUEST_LINE,
        PARSE_HEADERS,
        PARSE_BODY,
        PARSE_CHUNKED_BODY,
        PARSE_COMPLETE,
        PARSE_ERROR
    };

    Request();
    ~Request();
    Request(const Request& other);
    Request& operator=(const Request& other);

    // Feed raw data to the parser, returns true when request is complete
    bool parse(const std::string& data);

    // Reset for reuse (keep-alive connections)
    void reset();

    // Getters for parsed data
    const std::string& getMethod() const;
    const std::string& getUri() const;
    const std::string& getPath() const;           // URI without query string
    const std::string& getQueryString() const;
    const std::string& getHttpVersion() const;
    const std::string& getBody() const;

    // Header access (case-insensitive)
    std::string getHeader(const std::string& name) const;
    const std::map<std::string, std::string>& getHeaders() const;

    // State queries
    ParseState getState() const;
    bool isComplete() const;
    bool hasError() const;
    int getErrorCode() const;  // HTTP error code (400, 413, etc.)

    // Body size for limit checking
    size_t getContentLength() const;
    size_t getBodySize() const;

private:
    std::string _method;
    std::string _uri;
    std::string _path;
    std::string _queryString;
    std::string _httpVersion;
    std::string _body;
    std::map<std::string, std::string> _headers;

    ParseState _state;
    int _errorCode;
    std::string _buffer;
    size_t _contentLength;
    size_t _bodyBytesRead;

    bool parseRequestLine(const std::string& line);
    bool parseHeader(const std::string& line);
    bool parseChunkedBody();
};

#endif
