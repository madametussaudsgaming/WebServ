/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Tcp.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpadasia <ryanpadasian@gmail.com>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/08 17:15:44 by rpadasia          #+#    #+#             */
/*   Updated: 2026/04/14 17:04:06 by rpadasia         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"
#include "Request.hpp"
#include "Config.hpp"

/* ─────────────────────────────────────────────
   CONSTANTS
   ───────────────────────────────────────────── */

static const int         MAX_CLIENTS   = 128;
static const int         CLIENT_TIMEOUT_SEC = 30;               // disconnect idle clients after 30s

/* ─────────────────────────────────────────────
   HELPER: EXTENSION TYPES
   Returns the Content-Type string for a file extension.
   ───────────────────────────────────────────── */

static std::string getExtensionType(const std::string &path)
{
    // Find the last '.' in the path to get the extension
    size_t dot = path.rfind('.');
    if (dot == std::string::npos)
        return "application/octet-stream";

    std::string ext = path.substr(dot);
    // Convert from begining!
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css")                   return "text/css";
    if (ext == ".js")                    return "application/javascript";
    if (ext == ".json")                  return "application/json";
    if (ext == ".png")                   return "image/png";
    if (ext == ".jfif")                  return "image/jfif";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif")                   return "image/gif";
    if (ext == ".svg")                   return "image/svg+xml";
    if (ext == ".ico")                   return "image/x-icon";
    if (ext == ".txt")                   return "text/plain";
    if (ext == ".pdf")                   return "application/pdf";
    return "application/octet-stream";
}

/* ─────────────────────────────────────────────
   HELPER: BUILD HTTP RESPONSE STRING
   Assembles the status line + headers + body into
   one string ready to be sent over the socket.
   ───────────────────────────────────────────── */

std::string buildResponse(int statusCode,
                                 const std::string &statusText,
                                 const std::string &contentType,
                                 const std::string &body)
{
    std::ostringstream oss;
    oss << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n"
        << "Server: webserv/0.1\r\n"
        << "Content-Type: " << contentType << "\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Connection: close\r\n"
        << "\r\n"
        << body;
    return oss.str();
}

/* ─────────────────────────────────────────────
   HELPER: SIMPLE ERROR RESPONSES
   ───────────────────────────────────────────── */

std::string errorResponse(int code, const std::string &msg)
{
    std::ostringstream oss;
    oss << "<html><body><h1>" << code << " " << msg << "</h1></body></html>";
    return buildResponse(code, msg, "text/html", oss.str());
}

/* ─────────────────────────────────────────────
   HELPER: READ A FILE FROM DISK INTO A STRING
   Returns false if the file could not be opened.
   ───────────────────────────────────────────── */

static bool readFile(const std::string &filePath, std::string &outContent)
{
    // Open in binary mode so images and other binary files aren't corrupted
    std::ifstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open())
        return false;
    // Read entire file into string via stringstream
    std::ostringstream ss;
    ss << file.rdbuf();
    outContent = ss.str();
    return true;
}

/* ─────────────────────────────────────────────
   HELPER: CHECK IF PATH IS A DIRECTORY
   ───────────────────────────────────────────── */

static bool isDirectory(const std::string &path)
{
    struct stat s;
    if (stat(path.c_str(), &s) != 0)
        return false;
    return S_ISDIR(s.st_mode);
}

/* ─────────────────────────────────────────────
   HELPER: GENERATE A DIRECTORY LISTING PAGE
   When autoindex is on in a real server this is
   what gets shown. Useful for debugging.
   ───────────────────────────────────────────── */

static std::string directoryListing(const std::string &uriPath,
                                    const std::string &dirPath)
{
    std::ostringstream html;
    html << "<html><head><title>Index of " << uriPath << "</title></head>"
         << "<body><h2>Index of " << uriPath << "</h2><hr><ul>";

    DIR *dir = opendir(dirPath.c_str());
    //real ass class btw, literally designed for holding directories
    if (dir)
    {
        struct dirent *entry;
        // below is to iterate through the directory, readdir reads next entry
        while ((entry = readdir(dir)) != NULL)
        {
            std::string name = entry->d_name;
            if (name == ".")
                continue;
            html << "<li><a href=\"" << uriPath;
            // make sure there's exactly one slash between path and name
            if (!uriPath.empty() && uriPath[uriPath.size() - 1] != '/')
                html << "/";
            html << name << "\">" << name << "</a></li>";
        }
        closedir(dir);
    }
    html << "</ul><hr></body></html>";
    return html.str();
}

/* ─────────────────────────────────────────────
   HANDLER: GET
   Resolves the URI to a file path under SERVER_ROOT,
   then reads and returns the file.
   ───────────────────────────────────────────── */

static std::string handleGET(const Request &req, const LocationConfig *loc)
{
    // Strip query string (everything after '?') — we don't handle it yet
    std::string uriPath = req.path;
    size_t qmark = uriPath.find('?');
    if (qmark != std::string::npos)
        uriPath = uriPath.substr(0, qmark);

    // Build the filesystem pathh
    std::string filePath = loc->root + uriPath;
    if (req.path.find("..") != std::string::npos)
        return errorResponse(403, "Forbidden");

    // If the path ends in '/' or is a directory, try to serve index.html
    if (isDirectory(filePath))
    {
        // The trailing slash fix there because "./html" + "index.html" would give "./htmlindex.html" — BZZZT! wrong. We need "./html/" first.
        if (!filePath.empty() && filePath[filePath.size() - 1] != '/')
            filePath += "/";
        std::string indexPath = filePath + loc->index;
        std::string content;
        // Got index.html :>
        if (readFile(indexPath, content))
            return buildResponse(200, "OK", "text/html", content);
        // No index.html — show directory listing instead -_-
        return buildResponse(200, "OK", "text/html",
                             directoryListing(uriPath, filePath));
    }

    // Try to read the file
    std::string content;
    if (!readFile(filePath, content))
    {
        std::cout << "[GET] 404 Not Found: " << filePath << std::endl;
        return errorResponse(404, "Not Found");
    }

    std::cout << "[GET] 200 OK: " << filePath << std::endl;
    return buildResponse(200, "OK", getExtensionType(filePath), content);
}

/* ─────────────────────────────────────────────
   DISPATCH: ROUTE REQUEST TO THE RIGHT HANDLER
   ───────────────────────────────────────────── */

static std::string handleRequest(const Request &req,
                                  const LocationConfig *loc)
{
    if (!loc)
        return errorResponse(404, "Not Found");

    if (loc->redirect_code != -1)
    {
        std::string body = "<html><body>Redirecting...</body></html>";
        std::ostringstream oss;
        oss << "HTTP/1.1 " << loc->redirect_code << " Moved\r\n"
            << "Location: " << loc->redirect_url << "\r\n"
            << "Content-Length: " << body.size() << "\r\n"
            << "Connection: close\r\n\r\n" << body;
        return oss.str();
    }
    bool allowed = false;
    for (size_t i = 0; i < loc->methods.size(); i++)
        if (loc->methods[i] == req.method) { allowed = true; break; }
    if (!allowed)
        return errorResponse(405, "Method Not Allowed");
    std::cout << "[REQUEST] " << req.method << " " << req.path
              << " " << req.version << std::endl;

    if (req.method == "GET")
        return handleGET(req, loc);
    if (req.method == "POST")
        return handlePOST(req, loc);
    if (req.method == "DELETE")
        return handleDELETE(req, loc);

    // Any other method (PUT, PATCH, etc.) — not implemented
    return errorResponse(405, "Method Not Allowed");
}

/* ─────────────────────────────────────────────
   SOCKET SETUP
   Creates a TCP socket, sets options, binds to
   PORT, and starts listening.
   Returns the server file descriptor on success,
   or -1 on failure.
   ───────────────────────────────────────────── */

static int setupServerSocket(int port)
{
    // AF_INET  = IPv4
    // SOCK_STREAM = TCP (reliable, ordered, connection-based)
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0)
    {
        Error2exit("socket() failed", 1);
        return -1;
    }

    // SO_REUSEADDR lets us restart the server immediately without waiting
    // for the OS to release the port (avoids "Address already in use" errors)
    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        Error2exit("setsockopt() failed", 1);
        close(serverFd);
        return -1;
    }

    // Set server socket to non-blocking so poll() works correctly
    // Without this, accept() could block the whole server
    int flags = fcntl(serverFd, F_GETFL, 0);
    fcntl(serverFd, F_SETFL, flags | O_NONBLOCK);

    // Describe the address we want to bind to
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;         // IPv4
    addr.sin_addr.s_addr = INADDR_ANY;      // accept connections on any network interface
    addr.sin_port        = htons(port);     // htons converts port to network byte order

    if (bind(serverFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        std::ostringstream portOss;
        portOss << port;
        Error2exit("bind() failed — is port " + portOss.str() + " already in use?", 1);
        close(serverFd);
        return -1;
    }

    // SOMAXCONN is the maximum backlog of pending connections the OS will queue
    if (listen(serverFd, SOMAXCONN) < 0)
    {
        Error2exit("listen() failed", 1);
        close(serverFd);
        return -1;
    }

    std::cout << "Server running on port " << port << std::endl;
    return serverFd;
}

/* ─────────────────────────────────────────────
   REMOVE CLIENT FROM POLL ARRAY
   Closes the fd and shifts the array down so
   there are no gaps. Poll needs a contiguous array.
   ───────────────────────────────────────────── */

static void removeClient(struct pollfd *fds, int &nfds, int index)
{
    close(fds[index].fd);
    // Move the last entry into this slot to fill the gap
    fds[index] = fds[nfds - 1];
    nfds--;
}

/* ─────────────────────────────────────────────
   ACCEPT A NEW CLIENT CONNECTION
   Called when the server socket is readable.
   ───────────────────────────────────────────── */

static void acceptClient(int serverFd, struct pollfd *fds, int &nfds,
                         time_t *lastActivity, const std::map<int, const ServerConfig*> &serverFdToConfig,
                         std::map<int, const ServerConfig*> &clientToConfig)
{
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    int clientFd = accept(serverFd, (struct sockaddr *)&clientAddr, &clientLen);
    if (clientFd < 0)
    {
        // EAGAIN/EWOULDBLOCK just means no pending connection right now — not an error
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            std::cerr << "[ACCEPT] accept() failed: " << strerror(errno) << std::endl;
        return;
    }

    if (nfds >= MAX_CLIENTS)
    {
        std::cerr << "[ACCEPT] Too many clients, rejecting connection." << std::endl;
        close(clientFd);
        return;
    }

    // Set client socket to non-blocking as well
    int flags = fcntl(clientFd, F_GETFL, 0);
    fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);

    // Register in poll array — POLLIN means we want to know when data arrives
    fds[nfds].fd      = clientFd;
    fds[nfds].events  = POLLIN;
    fds[nfds].revents = 0;
    lastActivity[nfds] = std::time(NULL);
    nfds++;

    std::cout << "[ACCEPT] New client connected (fd=" << clientFd
              << ", total=" << nfds - 1 << ")" << std::endl;
    clientToConfig[clientFd] = serverFdToConfig.at(serverFd);
}

//lowkey dont know how this works hmmm
static const LocationConfig* findLocation(const ServerConfig &cfg,
                                           const std::string &uri)
{
    const LocationConfig *best    = NULL;
    size_t                bestLen = 0;

    for (size_t i = 0; i < cfg.locations.size(); i++)
    {
        const std::string &locPath = cfg.locations[i].path;
        // Check if the URI starts with this location's path
        if (uri.substr(0, locPath.size()) == locPath && locPath.size() > bestLen)
        {
            best    = &cfg.locations[i];
            bestLen = locPath.size();
        }
    }
    return best;  // NULL if no location matched
}

/* ─────────────────────────────────────────────
   READ FROM A CLIENT AND SEND A RESPONSE
   Called when a client socket is readable.
   ───────────────────────────────────────────── */

static void handleClient(struct pollfd *fds, int &nfds, int index,
                          time_t *lastActivity, std::map<int, const ServerConfig*> &clientToConfig)
{
    int clientFd = fds[index].fd;
    char buffer[BUFFER_SIZE];
    std::string rawRequest;

    // Read loop — keep reading until recv returns 0 or an error
    // because the request might be larger than one BUFFER_SIZE chunk
    while (true)
    {
        std::memset(buffer, 0, sizeof(buffer));
        ssize_t bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

        if (bytesRead < 0)
        {
            // EAGAIN means no more data right now (non-blocking) — we're done reading
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            std::cerr << "[RECV] recv() error on fd=" << clientFd << std::endl;
            removeClient(fds, nfds, index);
            return;
        }
        if (bytesRead == 0)
        {
            // Client closed the connection
            clientToConfig.erase(clientFd);
            std::cout << "[RECV] Client disconnected (fd=" << clientFd << ")" << std::endl;
            removeClient(fds, nfds, index);
            return;
        }

        rawRequest.append(buffer, bytesRead);

        // If we've received the end of the headers (\r\n\r\n) and have
        // all the body bytes declared by Content-Length, stop reading
        size_t headerEnd = rawRequest.find("\r\n\r\n");
        if (headerEnd != std::string::npos)
        {
            // Check if we have the full body yet
            size_t contentLengthPos = rawRequest.find("Content-Length:");
            if (contentLengthPos == std::string::npos)
                break; // no body expected

            size_t valueStart = contentLengthPos + 15; // skip "Content-Length:"
            size_t valueEnd   = rawRequest.find("\r\n", valueStart);
            std::string clStr = rawRequest.substr(valueStart, valueEnd - valueStart);
            // Trim leading spaces
            size_t firstDigit = clStr.find_first_not_of(" \t");
            size_t declaredLength = (firstDigit != std::string::npos)
                                        ? (size_t)std::atoi(clStr.c_str() + firstDigit)
                                        : 0;
            size_t bodyReceived = rawRequest.size() - (headerEnd + 4);
            if (bodyReceived >= declaredLength)
                break; // have the full body
        }
    }

    if (rawRequest.empty())
    {
        clientToConfig.erase(clientFd);
        removeClient(fds, nfds, index);
        return;
    }

    // Parse the raw bytes into a structured Request object
    Request req;
    parseRequest(rawRequest, req);

    const ServerConfig *cfg = clientToConfig[clientFd];

    // Find which location matches the request URI
    const LocationConfig *loc = findLocation(*cfg, req.path);

    std::string response = handleRequest(req, loc);


    // Build and send the response
    ssize_t total = (ssize_t)response.size();
    ssize_t sent  = 0;
    while (sent < total)
    {
        ssize_t n = send(clientFd, response.c_str() + sent, total - sent, 0);
        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue; // try again
            std::cerr << "[SEND] send() failed on fd=" << clientFd << std::endl;
            break;
        }
        sent += n;
    }

    // Update activity timestamp
    lastActivity[index] = std::time(NULL);

    // We set Connection: close so disconnect after the response
    clientToConfig.erase(clientFd);  // clean up when done
    removeClient(fds, nfds, index);
    return ;
}

/* ─────────────────────────────────────────────
   TIMEOUT CHECK
   Disconnects clients that haven't sent anything
   for CLIENT_TIMEOUT_SEC seconds.
   ───────────────────────────────────────────── */

static void checkTimeouts(struct pollfd *fds, int &nfds,
                           time_t *lastActivity,
                           const std::map<int, const ServerConfig*> &serverFdToConfig,
                           std::map<int, const ServerConfig*> &clientToConfig)
{
    time_t now = std::time(NULL);
    for (int i = nfds - 1; i >= 0; i--)
    {
        // Skip server sockets — only timeout client sockets
        if (serverFdToConfig.count(fds[i].fd))
            continue;

        if (now - lastActivity[i] > CLIENT_TIMEOUT_SEC)
        {
            std::cout << "[TIMEOUT] Closing idle client fd=" << fds[i].fd << std::endl;
            clientToConfig.erase(fds[i].fd);
            removeClient(fds, nfds, i);
        }
    }
}

/* ─────────────────────────────────────────────
   MAIN ENTRY POINT
   Call runServer() from main() to start everything.
   ───────────────────────────────────────────── */

void runServer(std::vector<ServerConfig> &configs)
{
    // poll() needs an array of pollfd structs — one per fd we're watching
    struct pollfd fds[MAX_CLIENTS];
    time_t        lastActivity[MAX_CLIENTS]; // parallel array tracking last recv time
    int           nfds = 0;

    std::map<int, const ServerConfig*> serverFdToConfig;
    //tracking
    std::map<int, const ServerConfig*> clientToConfig;


    for (size_t i = 0; i < configs.size(); i++)
    {
        for (size_t j = 0; j < configs[i].ports.size(); j++)
        {
            int port = configs[i].ports[j];
            int fd = setupServerSocket(port);
            if (fd < 0)
            {
                std::cerr << "[INIT] Failed to bind port " << port << std::endl;
                continue;
            }
            fds[nfds].fd = fd;
            fds[nfds].events = POLLIN;
            fds[nfds].revents = 0;
            lastActivity[nfds] = std::time(NULL);
            nfds++;
            serverFdToConfig[fd] = &configs[i];
            std::cout << "[INIT] Listening on port " << port << std::endl;
        }
    }
    if (nfds == 0)
    {
        std::cerr << "[INIT] No ports bound, exiting." << std::endl;
        return;
    }
    while (true)
    {
        // poll() blocks until at least one fd is ready, or timeout (1000ms here
        // so we can also run the timeout check regularly)
        int ready = poll(fds, nfds, 1000);

        if (ready < 0)
        {
            std::cerr << "[POLL] poll() error: " << strerror(errno) << std::endl;
            break;
        }

        // Check for idle clients even when poll() times out (ready == 0)
        checkTimeouts(fds, nfds, lastActivity, serverFdToConfig, clientToConfig);

        if (ready == 0)
            continue; // timeout — nothing to do this iteration

        for (int i = nfds - 1; i >= 0; i--)
        {
            if (fds[i].revents == 0) continue;

            if (serverFdToConfig.count(fds[i].fd))
            {
        // It's a server socket — accept a new client
            if (fds[i].revents & POLLIN)
                acceptClient(fds[i].fd, fds, nfds, lastActivity,
                            serverFdToConfig, clientToConfig);
            }
            else
            {
                // Client socket: either data arrived or an error occurred
                if (fds[i].revents & (POLLIN | POLLERR | POLLHUP))
                    handleClient(fds, nfds, i, lastActivity, clientToConfig);
            }
        }
    }
    for (std::map<int, const ServerConfig*>::iterator it = serverFdToConfig.begin(); it != serverFdToConfig.end(); ++it)
        close(it->first);
}
