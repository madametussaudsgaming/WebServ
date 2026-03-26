/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alechin <alechin@student.42kl.edu.my>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/23 12:33:44 by alechin           #+#    #+#             */
/*   Updated: 2026/03/26 15:17:00 by alechin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"

class ConfigParser {
	public:
		std::string method;
		std::string path;
		std::string version;
		std::map<std::string, std::string> header;
};