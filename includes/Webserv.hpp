/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alechin <alechin@student.42kl.edu.my>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/17 10:58:08 by alechin           #+#    #+#             */
/*   Updated: 2026/03/31 21:27:47 by alechin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERV_H
#define WEBSERV_H

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

class GET {
	
};

class POST {

};

class DELETE {

};

int	Error2exit(std::string errorMessage, int status);

#endif