/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alechin <alechin@student.42kl.edu.my>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/31 20:15:15 by alechin           #+#    #+#             */
/*   Updated: 2026/04/01 15:05:21 by alechin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"

void parseRequest(const std::string& raw, Request& request) {
	size_t		headerEnd = raw.find("\r\n\r\n");
	if (headerEnd == std::string::npos)
		return;
	std::string	headerPart = raw.substr(0, headerEnd);
	std::string bodyPart = raw.substr(headerEnd + 4);
	std::vector<std::string> lines;
	size_t start = 0, end;
	
	while ((end = headerPart.find("\r\n", start)) != std::string::npos) {
		lines.push_back(headerPart.substr(start, end - start));
		start = end + 2;
	}
	lines.push_back(headerPart.substr(start));
	if (lines.size() == 0)
		return;

	std::istringstream iss(lines[0]);
	if (!(iss >> request.method >> request.path >> request.version)) {
		Error2exit("Invalid Request Line\n", 1);
		return;
	}

	for (size_t i = 1; i < lines.size(); i++) {
		size_t colon = lines[i].find(":");
		if (colon == std::string::npos)
			continue;

		std::string key = lines[i].substr(0, colon);
        std::string value = lines[i].substr(colon + 1);

		while (!value.empty() && value[0] == ' ')
			value.erase(0, 1);
		request.headers[key] = value;
	}
	if (request.headers.count("Content-Length"))
		request.contentLength = std::atoi(request.headers["Content-Length"].c_str());
	if (bodyPart.size() >= request.contentLength)
		request.body = bodyPart.substr(0, request.contentLength);
}