/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alechin <alechin@student.42kl.edu.my>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/16 14:04:13 by alechin           #+#    #+#             */
/*   Updated: 2026/05/04 13:20:07 by alechin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_HPP
#define CGI_HPP

#include <string>
#include "Request.hpp"

std::string executeCGI(const Request& request, const std::string& scriptPath);
std::string buildResponse(const Request& req, const ServerConfig& config);

#endif