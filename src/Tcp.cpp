/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Tcp.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpadasia <ryanpadasian@gmail.com>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/08 17:15:44 by rpadasia          #+#    #+#             */
/*   Updated: 2026/04/08 18:33:25 by rpadasia         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"
#include "Request.hpp"

/* ─────────────────────────────────────────────
   CONSTANTS
   ───────────────────────────────────────────── */

static const std::string SERVER_ROOT   = "./frontend";   // folder that holds your website files
static const std::string UPLOAD_DIR    = "./frontend/uploads"; // where uploaded files are saved
static const int         MAX_CLIENTS   = 128;
static const int         CLIENT_TIMEOUT_SEC = 30;    // disconnect idle clients after 30s

/* ─────────────────────────────────────────────
   HELPER: MIME TYPES
   Returns the Content-Type string for a file extension.
   ───────────────────────────────────────────── */

static std::string getMimeType(const std::string &path)
{
    // Find the last '.' in the path to get the extension
    size_t dot = path.rfind('.');
    if (dot == std::string::npos)
        return "application/octet-stream";

    std::string ext = path.substr(dot);
    // Convert extension to lowercase for consistent matching
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css")                   return "text/css";
    if (ext == ".js")                    return "application/javascript";
    if (ext == ".json")                  return "application/json";
    if (ext == ".png")                   return "image/png";
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

static std::string buildResponse(int statusCode,
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

static std::string errorResponse(int code, const std::string &msg)
{
    std::ostringstream oss;
    oss << code;
    std::string body = "<html><body><h1>" + oss.str() +
                       " " + msg + "</h1></body></html>";
    return buildResponse(code, msg, "text/html", body);
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
    if (dir)
    {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            std::string name = entry->d_name;
            if (name == ".")
                continue;
            html << "<li><a href=\"" << uriPath;
            // Make sure there's exactly one slash between path and name
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

static std::string handleGET(const Request &req)
{
    // Strip query string (everything after '?') — we don't handle it yet
    std::string uriPath = req.path;
    size_t qmark = uriPath.find('?');
    if (qmark != std::string::npos)
        uriPath = uriPath.substr(0, qmark);

    // Build the filesystem path: SERVER_ROOT + URI
    std::string filePath = SERVER_ROOT + uriPath;

    // If the path ends in '/' or is a directory, try to serve index.html
    if (isDirectory(filePath))
    {
        // Make sure there's a trailing slash for the directory
        if (!filePath.empty() && filePath[filePath.size() - 1] != '/')
            filePath += "/";
        std::string indexPath = filePath + "index.html";
        std::string content;
        if (readFile(indexPath, content))
            return buildResponse(200, "OK", "text/html", content);
        // No index.html — show directory listing instead
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
    return buildResponse(200, "OK", getMimeType(filePath), content);
}

/* ─────────────────────────────────────────────
   HANDLER: POST
   Saves the request body as a file in UPLOAD_DIR.
   A real server would parse multipart here.
   ───────────────────────────────────────────── */

static std::string handlePOST(const Request &req)
{
    // Generate a filename based on the current timestamp so uploads don't overwrite each other
    std::time_t now = std::time(NULL);
    std::ostringstream oss;
    oss << now;
    std::string filename = UPLOAD_DIR + "/upload_" + oss.str() + ".bin";

    // Make sure the upload directory exists
    mkdir(UPLOAD_DIR.c_str(), 0755);

    std::ofstream outFile(filename.c_str(), std::ios::binary);
    if (!outFile.is_open())
    {
        std::cerr << "[POST] Could not open file for writing: " << filename << std::endl;
        return errorResponse(500, "Internal Server Error");
    }

    outFile.write(req.body.c_str(), req.body.size());
    outFile.close();

    std::cout << "[POST] Saved " << req.body.size()
              << " bytes to " << filename << std::endl;

    std::ostringstream bodyOss;
    bodyOss << "<html><body>"
            << "<h2>Upload received</h2>"
            << "<p>Saved " << req.body.size()
            << " bytes to <code>" << filename << "</code></p>"
            << "</body></html>";
    return buildResponse(200, "OK", "text/html", bodyOss.str());
}

/* ─────────────────────────────────────────────
   HANDLER: DELETE
   Deletes a file from SERVER_ROOT given a URI path.
   ───────────────────────────────────────────── */

static std::string handleDELETE(const Request &req)
{
    std::string filePath = SERVER_ROOT + req.path;

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

/* ─────────────────────────────────────────────
   DISPATCH: ROUTE REQUEST TO THE RIGHT HANDLER
   ───────────────────────────────────────────── */

static std::string handleRequest(const Request &req)
{
    std::cout << "[REQUEST] " << req.method << " " << req.path
              << " " << req.version << std::endl;

    if (req.method == "GET")
        return handleGET(req);
    if (req.method == "POST")
        return handlePOST(req);
    if (req.method == "DELETE")
        return handleDELETE(req);

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

static int setupServerSocket()
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
    addr.sin_port        = htons(PORT);     // htons converts port to network byte order

    if (bind(serverFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        std::ostringstream portOss;
        portOss << PORT;
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

    std::cout << "Server running on port " << PORT << std::endl;
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
                         time_t *lastActivity)
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
}

/* ─────────────────────────────────────────────
   READ FROM A CLIENT AND SEND A RESPONSE
   Called when a client socket is readable.
   ───────────────────────────────────────────── */

static void handleClient(struct pollfd *fds, int &nfds, int index,
                          time_t *lastActivity)
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
        removeClient(fds, nfds, index);
        return;
    }

    // Parse the raw bytes into a structured Request object
    Request req;
    parseRequest(rawRequest, req);

    // Build and send the response
    std::string response = handleRequest(req);
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
    removeClient(fds, nfds, index);
}

/* ─────────────────────────────────────────────
   TIMEOUT CHECK
   Disconnects clients that haven't sent anything
   for CLIENT_TIMEOUT_SEC seconds.
   ───────────────────────────────────────────── */

static void checkTimeouts(struct pollfd *fds, int &nfds,
                           time_t *lastActivity, int serverFd)
{
    time_t now = std::time(NULL);
    for (int i = nfds - 1; i >= 1; i--) // skip index 0 (server socket)
    {
        if (fds[i].fd == serverFd)
            continue;
        if (now - lastActivity[i] > CLIENT_TIMEOUT_SEC)
        {
            std::cout << "[TIMEOUT] Closing idle client fd=" << fds[i].fd << std::endl;
            removeClient(fds, nfds, i);
        }
    }
}

/* ─────────────────────────────────────────────
   MAIN ENTRY POINT
   Call runServer() from main() to start everything.
   ───────────────────────────────────────────── */

void runServer()
{
    int serverFd = setupServerSocket();
    if (serverFd < 0)
        return;

    // poll() needs an array of pollfd structs — one per fd we're watching
    struct pollfd fds[MAX_CLIENTS];
    time_t        lastActivity[MAX_CLIENTS]; // parallel array tracking last recv time
    int           nfds = 0;

    // Register the server socket as the first entry (index 0)
    fds[0].fd      = serverFd;
    fds[0].events  = POLLIN; // notify us when a new connection arrives
    fds[0].revents = 0;
    lastActivity[0] = std::time(NULL);
    nfds = 1;

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
        checkTimeouts(fds, nfds, lastActivity, serverFd);

        if (ready == 0)
            continue; // timeout — nothing to do this iteration

        // Walk every fd to see which ones are ready
        // We iterate backwards so that removeClient() (which swaps with last element)
        // doesn't cause us to skip or double-process an fd
        for (int i = nfds - 1; i >= 0; i--)
        {
            if (fds[i].revents == 0)
                continue; // nothing happened on this fd

            if (fds[i].fd == serverFd)
            {
                // Server socket readable = new client wants to connect
                if (fds[i].revents & POLLIN)
                    acceptClient(serverFd, fds, nfds, lastActivity);
            }
            else
            {
                // Client socket: either data arrived or an error occurred
                if (fds[i].revents & (POLLIN | POLLERR | POLLHUP))
                    handleClient(fds, nfds, i, lastActivity);
            }
        }
    }
    close(serverFd);
}