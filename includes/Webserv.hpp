/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpadasia <ryanpadasian@gmail.com>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/17 10:58:08 by alechin           #+#    #+#             */
/*   Updated: 2026/04/14 17:05:05 by rpadasia         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#define PORT 8080
#define BUFFER_SIZE 4096

#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>

#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <dirent.h>
#include <ctime>

#include "Config.hpp"

struct ServerConfig;

int		Error2exit(std::string errorMessage, int status);
void    runServer(std::vector<ServerConfig> &configs);

//TCP
std::string errorResponse(int code, const std::string &msg);
std::string buildResponse(int statusCode,
                                 const std::string &statusText,
                                 const std::string &contentType,
                                 const std::string &body);




#endif