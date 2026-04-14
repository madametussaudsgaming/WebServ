/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   handlePOST.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpadasia <ryanpadasian@gmail.com>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/11 22:41:23 by rpadasia          #+#    #+#             */
/*   Updated: 2026/04/14 16:49:46 by rpadasia         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"
#include "Request.hpp"
#include "Config.hpp"

static std::string extractHeaderValue(const std::string &line, const std::string &key)
{
    std::string search = key + "=\"";
    size_t start = line.find(search);
    if (start == std::string::npos)
        return "";
    start += search.size();
    size_t end = line.find("\"", start);
    if (end == std::string::npos)
        return "";
    return line.substr(start, end - start);
}

std::string handlePOST(const Request &req, const LocationConfig *loc)
{
    if (loc->upload_store.empty())
        return errorResponse(405, "Method Not Allowed");
    mkdir(loc->upload_store.c_str(), 0755);

    // Check if this is a multipart upload by looking at Content-Type header
    std::map<std::string, std::string>::const_iterator ctIt = req.headers.find("Content-Type");
    if (ctIt == req.headers.end())
        return errorResponse(400, "Bad Request");

    std::string contentType = ctIt->second;
    size_t boundaryPos = contentType.find("boundary=");

    // ── PLAIN TEXT / JSON POST (no boundary = not multipart) ──────────────
    if (boundaryPos == std::string::npos)
    {
        std::time_t now = std::time(NULL);
        std::ostringstream oss;
        oss << now;
        std::string filename = loc->upload_store + "/upload_" + oss.str() + ".txt";

        std::ofstream outFile(filename.c_str(), std::ios::binary);
        if (!outFile.is_open())
            return errorResponse(500, "Internal Server Error");

        outFile.write(req.body.c_str(), req.body.size());
        outFile.close();

        std::cout << "[POST] Plain body saved to " << filename << std::endl;

        std::ostringstream resp;
        resp << "<html><body><h2>Received</h2><p>Saved "
             << req.body.size() << " bytes to <code>" << filename
             << "</code></p></body></html>";
        return buildResponse(200, "OK", "text/html", resp.str());
    }

    // ── MULTIPART FORM DATA ────────────────────────────────────────────────

    // Extract boundary string from Content-Type header
    // The actual boundary used in the body has "--" prepended to it
    std::string boundary = "--" + contentType.substr(boundaryPos + 9);

    // Trim any trailing whitespace/carriage return from boundary
    while (!boundary.empty() && (boundary[boundary.size() - 1] == '\r' || boundary[boundary.size() - 1] == ' '))
        boundary.erase(boundary.size() - 1);

    std::vector<std::string> savedFiles;
    std::string body = req.body;
    size_t pos = 0;

    while (true)
    {
        // Find the next boundary marker in the body
        size_t boundStart = body.find(boundary, pos);
        if (boundStart == std::string::npos)
            break;

        // Move past the boundary line (boundary + \r\n)
        size_t partStart = boundStart + boundary.size();
        if (body.substr(partStart, 2) == "--")
            break; // this is the final boundary marker (boundary + "--"), we're done
        if (body.substr(partStart, 2) == "\r\n")
            partStart += 2;

        // Find where this part ends (next boundary)
        size_t partEnd = body.find(boundary, partStart);
        if (partEnd == std::string::npos)
            break;

        // The part ends just before "\r\n--boundary", so subtract 2 for the \r\n
        if (partEnd >= 2)
            partEnd -= 2;

        std::string part = body.substr(partStart, partEnd - partStart);

        // Split part into its own mini headers and its content
        // The blank line (\r\n\r\n) separates part-headers from part-body
        size_t partHeaderEnd = part.find("\r\n\r\n");
        if (partHeaderEnd == std::string::npos)
        {
            pos = boundStart + boundary.size();
            continue;
        }

        std::string partHeaders = part.substr(0, partHeaderEnd);
        std::string partBody    = part.substr(partHeaderEnd + 4);

        // Look for a filename in the Content-Disposition header of this part
        std::string filename = "";
        std::istringstream headerStream(partHeaders);
        std::string headerLine;
        while (std::getline(headerStream, headerLine))
        {
            // Strip trailing \r if present
            if (!headerLine.empty() && headerLine[headerLine.size() - 1] == '\r')
                headerLine.erase(headerLine.size() - 1);

            if (headerLine.find("Content-Disposition") != std::string::npos)
                filename = extractHeaderValue(headerLine, "filename");
        }

        // Only save parts that have a filename (i.e. actual file uploads)
        // Parts without a filename are regular form text fields — skip them
        if (!filename.empty())
        {
            std::string savePath = loc->upload_store + "/" + filename;

            std::ofstream outFile(savePath.c_str(), std::ios::binary);
            if (!outFile.is_open())
            {
                std::cerr << "[POST] Could not write: " << savePath << std::endl;
            }
            else
            {
                outFile.write(partBody.c_str(), partBody.size());
                outFile.close();
                savedFiles.push_back(filename);
                std::cout << "[POST] Saved file: " << savePath
                          << " (" << partBody.size() << " bytes)" << std::endl;
            }
        }

        pos = boundStart + boundary.size();
    }

    if (savedFiles.empty())
        return errorResponse(400, "Bad Request: no files found in upload");

    // Build a summary response listing every saved file
    std::ostringstream resp;
    resp << "<html><body><h2>Upload successful</h2><ul>";
    for (size_t i = 0; i < savedFiles.size(); i++)
        resp << "<li><code>" << savedFiles[i] << "</code></li>";
    resp << "</ul></body></html>";
    return buildResponse(200, "OK", "text/html", resp.str());
}