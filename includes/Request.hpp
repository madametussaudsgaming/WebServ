/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alechin <alechin@student.42kl.edu.my>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/31 20:07:11 by alechin           #+#    #+#             */
/*   Updated: 2026/04/08 16:19:11 by alechin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HPP
#define REQUEST_HPP

#include "Webserv.hpp"
#include <sstream>
#include <cstdlib>

class Request {
	public:
		std::string method;
		std::string path;
		std::string version;
		
		std::map<std::string, std::string> headers;
		std::string body;

		size_t contentLength;
		//Request(): contentLength(0) {};
		//Request(const Request& other);
		//Request& operator=(const Request& other);
		//~Request();
};

void parseRequest(const std::string& raw, Request& request);

#endif