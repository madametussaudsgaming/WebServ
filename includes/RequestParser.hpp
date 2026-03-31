/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestParser.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alechin <alechin@student.42kl.edu.my>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/31 20:07:11 by alechin           #+#    #+#             */
/*   Updated: 2026/03/31 21:29:09 by alechin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUESTPARSER_HPP
#define REQUESTPARSER_HPP

#include "Webserv.hpp"
#include <sstream>
#include <cstdlib>

class RequestParser {
	public:
		std::string method;
		std::string path;
		std::string version;
		
		std::map<std::string, std::string> headers;
		std::string body;

		size_t contentLength;
		RequestParser(): contentLength(0) {};
		RequestParser(const RequestParser& other);
		RequestParser& operator=(const RequestParser& other);
		~RequestParser();
};

void parseRequest(const std::string& raw, RequestParser& request);

#endif