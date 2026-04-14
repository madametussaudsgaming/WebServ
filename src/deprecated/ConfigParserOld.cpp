/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParserOld.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpadasia <ryanpadasian@gmail.com>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/19 15:41:25 by alechin           #+#    #+#             */
/*   Updated: 2026/04/13 12:49:09 by rpadasia         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Config.hpp"

//Read the file into a string, strip comments (anything after #)
// Split into server { ... } blocks
// Inside each server block, split into location /path { ... } blocks
// Parse each directive line (key value;) into the appropriate struct field

static std::vector<std::string> extractServerBlocks(const std::string &text)
{
	std::vector<std::string> blocks;
	size_t i = 0;

	while (i < text.size())
	{
		size_t servPos = text.find("server", i);
		if (servPos == std::string::npos)
			break;
		size_t braceOpen = text.find('{', servPos);
		if (braceOpen == std::string::npos)
			throw std::runtime_error("Expected { brace.");
		int depth = 0;
		size_t j = braceOpen;
		for (j; j < text.size(); j++)
		{
			if (text[j] == '{') depth++;
			else if (text[j] == '}')
			{
				depth--;
				if (depth < 0)
					throw std::runtime_error("Not properly enclosed");
				if (depth == 0)
					break;
			}
			if (depth != 0)
					throw std::runtime_error("Not properly enclosed");
		}

		blocks.push_back((text.substr(braceOpen + 1, j - braceOpen - 1)));
		i = j + 1;
	}
	return (blocks);

}

std::vector<ServerConfig> parseConfigFile(const std::string &filename)
{
	std::vector<ServerConfig> servConfig;
	std::ifstream file(filename.c_str(), std::ios::binary);
	if (!file.is_open())
		throw std::runtime_error("Cannot open config file: " + filename);
	std::ostringstream ss;
	ss << file.rdbuf();
	std::string rawText = ss.str();
	std::vector<std::string> servers = extractServerBlocks(rawText);
	for (size_t i = 0; i < servers.size(); i++)
	{
		std::string rawString = servers[i];
		std::vector<int> rawPorts;
		size_t j = 0;
		while (true)
		{
			size_t listenPos = rawString.find("listen", j);
			if (listenPos == std::string::npos)
				break;
			size_t portStart = listenPos + 6;
			while (portStart < rawString.size() && rawString[portStart] == ' ')
				portStart++;
			size_t portEnd = rawString.find_first_not_of("0123456789", portStart);
			if (portEnd == std::string::npos)
				portEnd = rawString.size();

			std::string portStr = rawString.substr(portStart, portEnd - portStart);
			if (!portStr.empty())
				rawPorts.push_back(std::atoi(portStr.c_str()));
			j = portEnd;
		}
		servConfig[i].ports=rawPorts;
		std::map<int, std::string> rawErrorPages;
		j = 0;
		while (true)
		{
			int errorDirectories = 0;
			size_t errorcodePos = rawString.find("error_page", j);
			if (errorcodePos == std::string::npos)
				break;
			size_t codeStart = errorcodePos + 10;
			while (codeStart < rawString.size() && rawString[codeStart] == ' ')
				codeStart++;
			size_t codeEnd = rawString.find_first_not_of("0123456789", codeStart);
			if (codeEnd == std::string::npos)
				codeEnd = rawString.size();

			std::string codeStr = rawString.substr(codeStart, codeEnd - codeStart);
			j = codeEnd;
			size_t dirStart = rawString.find("/", j);
			size_t dirEnd = rawString.find(";", j);
			std::string dirPathStr = rawString.substr(dirStart, dirEnd - dirStart);
			rawErrorPages[errorDirectories] = (std::atoi(codeStr.c_str()), dirPathStr);
		}
	}

}