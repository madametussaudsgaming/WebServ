/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpadasia <ryanpadasian@gmail.com>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/13 09:30:46 by rpadasia          #+#    #+#             */
/*   Updated: 2026/04/14 16:50:34 by rpadasia         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_HPP
#define CONFIG_HPP
#include "Webserv.hpp"

struct LocationConfig {
    std::string							path;	// the URL prefix e.g. "/upload"
    std::vector<std::string>			methods;	// {"GET", "POST"}
    std::string							root;	// filesystem root for this location
    std::string							index;	// default file e.g. "index.html"
    bool								autoindex;	// directory listing on/off
    std::string							upload_store;	// where to save uploads
    int									redirect_code;	// 301, 302, etc. (-1 if none)
    std::string							redirect_url;
    std::map<std::string, std::string>	cgi_handlers;	// {".py" -> "/usr/bin/python3"}

    LocationConfig() : autoindex(false), redirect_code(-1) {
        methods.push_back("GET");
        index = "index.html";
    }
};

struct ServerConfig {
    std::vector<int>			ports;          // ports to listen on
    std::string					host;           // "0.0.0.0" default
    std::map<int, std::string>	error_pages;    // {404 -> "/errors/404.html"}
    size_t						max_body_size;  // bytes
    std::vector<LocationConfig>	locations;

    ServerConfig() : host("0.0.0.0"), max_body_size(1000000) {
    }
};

std::vector<ServerConfig> parseConfigFile(const std::string &filename);

#endif