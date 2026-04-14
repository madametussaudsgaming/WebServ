/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handleDELETE.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpadasia <ryanpadasian@gmail.com>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/11 22:41:19 by rpadasia          #+#    #+#             */
/*   Updated: 2026/04/14 11:44:46 by rpadasia         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"
#include "Request.hpp"
#include "Config.hpp"

std::string handleDELETE(const Request &req, const LocationConfig *loc)
{
    std::string filePath = loc->root + req.path;

    // Safety check: only allow deletion inside SERVER_ROOT
    // (a minimal guard — a real server needs more robust path traversal checks)
    if (req.path.find("..") != std::string::npos)
        return errorResponse(403, "Forbidden");

    if (std::remove(filePath.c_str()) != 0)
    {
        std::cerr << "[DELETE] Could not delete: " << filePath << std::endl;
        return errorResponse(404, "Not Found");
    }

    std::cout << "[DELETE] Deleted: " << filePath << std::endl;
    std::string body = "<html><body><p>Deleted: <code>" +
                       filePath + "</code></p></body></html>";
    return buildResponse(200, "OK", "text/html", body);
}