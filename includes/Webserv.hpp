/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alechin <alechin@student.42kl.edu.my>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/17 10:58:08 by alechin           #+#    #+#             */
/*   Updated: 2026/05/07 15:27:41 by alechin          ###   ########.fr       */
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
std::string errorResponse(int code, const std::string &msg, const ServerConfig *cfg);
std::string buildResponse(int statusCode,
                                 const std::string &statusText,
                                 const std::string &contentType,
                                 const std::string &body);




#endif