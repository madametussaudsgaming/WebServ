/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpadasia <rpadasia@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/31 20:15:15 by alechin           #+#    #+#             */
/*   Updated: 2026/05/17 18:06:12 by rpadasia         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Request.hpp"

static std::string toLower(const std::string& s) {
	std::string out = s;
	for (size_t i = 0; i < out.size(); i++)
		out[i] = std::tolower(static_cast<unsigned char>(out[i]));
	return (out);
}

static std::string trimOWO(const std::string& s) {
	size_t start = s.find_first_not_of("\t");
	if (start == std::string::npos) return "";
	size_t end = s.find_last_not_of("\t");
	return (s.substr(start, (end - start + 1)));
}

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
	iss >> request.method >> request.path >> request.version;
	if (request.version != "HTTP/1.1") {
	    Error2exit("Unsupported HTTP version", 1);
	    return;
	}

	for (size_t i = 1; i < lines.size(); i++) {
		size_t colon = lines[i].find(":");
		if (colon == std::string::npos)
			continue;

		std::string key = toLower(lines[i].substr(0, colon));
		std::string value = trimOWO(lines[i].substr(colon + 1));
		request.headers[key] = value;
	}
	if (request.headers.count("Content-Length"))
		request.contentLength = std::atoi(request.headers["Content-Length"].c_str());
	if (bodyPart.size() >= request.contentLength)
		request.body = bodyPart.substr(0, request.contentLength);
}