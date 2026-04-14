/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpadasia <ryanpadasian@gmail.com>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/19 15:41:25 by alechin           #+#    #+#             */
/*   Updated: 2026/04/14 17:11:38 by rpadasia         ###   ########.fr       */
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
		size_t j;
		for (j = braceOpen; j < text.size(); j++)
		{
			if (text[j] == '{') depth++;
			else if (text[j] == '}')
			{
				depth--;
				if (depth == 0)
					break;
			}
		}
		if (depth != 0)
					throw std::runtime_error("Not properly enclosed");
		blocks.push_back((text.substr(braceOpen + 1, j - braceOpen - 1)));
		i = j + 1;
	}
	return (blocks);
}

void parseLocationDirective(LocationConfig &loc, const std::vector<std::string> &words)
{
	if (words[0] == "methods" && words.size() >= 2)
	{
		loc.methods.clear();    // clear the default {"GET"} before setting from config
		for (size_t i = 1; i < words.size(); i++)
			loc.methods.push_back(words[i]);
	}
	else if (words[0] == "root" && words.size() >= 2)
		loc.root = words[1];

	else if (words[0] == "index" && words.size() >= 2)
		loc.index = words[1];

	else if (words[0] == "autoindex" && words.size() >= 2)
		loc.autoindex = (words[1] == "on");

	else if (words[0] == "upload_store" && words.size() >= 2)
		loc.upload_store = words[1];

	else if (words[0] == "return" && words.size() >= 3)
	{
		loc.redirect_code = std::atoi(words[1].c_str());
		loc.redirect_url  = words[2];
	}
	// else if (words[0] == "cgi_handler" && words.size() >= 3)
	// 	loc.cgi_handler[words[1]] = words[2];  // {".py" -> "/usr/bin/python3"}
}

static LocationConfig parseLocationBlock(std::istringstream& stream)
{
	LocationConfig config;
	std::string line;
	while (std::getline(stream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        // Strip comments
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos)
            line = line.substr(0, commentPos);

        std::istringstream lineStream(line);
        std::vector<std::string> words;
        std::string word;
        while (lineStream >> word)
            words.push_back(word);

        if (words.empty()) continue;

        // Closing brace = end of location block
        if (words[0] == "}") return (config);

        // Strip trailing semicolon from last word
        std::string &last = words[words.size() - 1];
        if (!last.empty() && last[last.size() - 1] == ';')
            last.erase(last.size() - 1);

        parseLocationDirective(config, words);
    }
	return (config);
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
		ServerConfig cfg;
		std::istringstream stream(servers[i]);
		std::string line;

		while (std::getline(stream, line))
		{
			// Strip comments
			size_t commentPos = line.find('#');
			if (commentPos != std::string::npos)
				line = line.substr(0, commentPos);

			// Split into words
			std::istringstream lineStream(line);
			std::vector<std::string> words;
			std::string word;
			while (lineStream >> word)
				words.push_back(word);

			if (words.empty())
				continue;

			std::string &last = words[words.size() - 1];
			if (!last.empty() && last[last.size() - 1] == ';')
				last.erase(last.size() - 1);

			if (words[0] == "listen" && words.size() >= 2)
			{
				size_t colon = words[1].find(':');
				if (colon != std::string::npos)
        		{
          			cfg.host = words[1].substr(0, colon);
          			cfg.ports.push_back(std::atoi(words[1].substr(colon + 1).c_str()));
     			}
				else
					cfg.ports.push_back(std::atoi(words[1].c_str()));
			}
			else if (words[0] == "server_name" && words.size() >= 2)
				cfg.host = words[1];
			else if (words[0] == "client_max_body_size" && words.size() >= 2)
				cfg.max_body_size = (size_t)std::atoi(words[1].c_str());
			else if (words[0] == "error_page" && words.size() >= 3)
				cfg.error_pages[std::atoi(words[1].c_str())] = words[2];
			else if (words[0] == "location" && words.size() >= 2)
			{
				//completely uncessary check in case of curlybrackets placed below
				bool foundBrace = false;
				for (size_t w = 2; w < words.size(); w++)
				{
					if (words[w] == "{") {foundBrace = true; break; }
				}
				while (!foundBrace)
				{
					std::string seekLine;
					if (!std::getline(stream, seekLine)) break;
					if (!seekLine.empty() && seekLine[seekLine.size() - 1] == '\r')
						seekLine.erase(seekLine.size() - 1);
					if (seekLine.find("{") != std::string::npos)
						foundBrace = true;
				}
				LocationConfig loc = parseLocationBlock(stream);
				loc.path = words[1];
				cfg.locations.push_back(loc);
			}
			// else
			// // std::cerr << "[CONFIG] Unknown directive: " << words[0] << std::endl;
		}
		servConfig.push_back(cfg);
	}
	return servConfig;
}