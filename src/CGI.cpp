/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rpadasia <rpadasia@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/16 13:01:11 by alechin           #+#    #+#             */
/*   Updated: 2026/05/17 18:06:47 by rpadasia         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"
#include "CGI.hpp"
#include <sys/wait.h>

#define CGI_TIMEOUT 5

static std::string intToStr(size_t num) {
	std::stringstream stringStream;

	stringStream << num;
	return stringStream.str();
}

static std::string extractQuery(const std::string& path) {
	size_t position = path.find("?");
	
	if (position == std::string::npos)
		return "";
	return path.substr(position + 1);
}

/*
static std::string cleanPath(const std::string& path) {
    size_t position = path.find("?");
    if (position == std::string::npos)
        return path;
    return path.substr(0, position);
}*/

static char** buildEnv(const Request& request, const std::string& scriptPath) {
	std::vector<std::string> env;

	env.push_back("REQUEST_METHOD=" + request.method);
	env.push_back("SCRIPT_FILENAME=" + scriptPath);
	env.push_back("QUERY_STRING=" + extractQuery(request.path));
	env.push_back("CONTENT_LENGTH=" + intToStr(request.body.size()));

	if (request.headers.count("content-type"))
		env.push_back("CONTENT_TYPE=" + request.headers.at("Content-Type"));
	
	env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	env.push_back("SERVER_PROTOCOL=HTTP/1.1");

	char **envp = new char*[env.size() + 1];
    for (size_t i = 0; i < env.size(); i++)
        envp[i] = strdup(env[i].c_str());
    envp[env.size()] = NULL;

	return envp;
}

static void freeEnv(char **envp) {
	for (int i = 0; envp[i]; i++)
		free(envp[i]);
	delete[] envp;
}

static std::string buildHttpResponse(const std::string& body) {
    std::stringstream result;

    result << "HTTP/1.1 200 OK\r\n";
    result << "Content-Length: " << body.size() << "\r\n";
    result << "Content-Type: text/html\r\n";
    result << "\r\n";
    result << body;

    return result.str();
}

std::string executeCGI(const Request& request, const std::string& scriptPath) {
	int		inPipe[2];
	int		outPipe[2];
	
	if (pipe(inPipe) < 0 || pipe(outPipe) < 0)
		return "HTTP/1.1 500 Internal Server Error\r\n\r\nPipe failed";
	pid_t	pid = fork();
	if (pid < 0)
		return "HTTP/1.1 500 Internal Server Error\r\n\r\nFork failed";
	if (pid == 0) {
		dup2(inPipe[0], STDIN_FILENO);
        dup2(outPipe[1], STDOUT_FILENO);

        close(inPipe[1]);
        close(outPipe[0]);

        char *argv[2];
        argv[0] = strdup(scriptPath.c_str());
        argv[1] = NULL;

		char **envp = buildEnv(request, scriptPath);

		execve(scriptPath.c_str(), argv, envp);

        freeEnv(envp);
		free(argv[0]);
        exit(1);
	} else {
		close(inPipe[0]);
        close(outPipe[1]);

        if (!request.body.empty())
            write(inPipe[1], request.body.c_str(), request.body.size());
        close(inPipe[1]);

        std::string output;
        char buffer[1024];

        fd_set readfds;
        struct timeval timeout;

        while (true) {
            FD_ZERO(&readfds);
            FD_SET(outPipe[0], &readfds);

            timeout.tv_sec = CGI_TIMEOUT;
            timeout.tv_usec = 0;

            int ret = select(outPipe[0] + 1, &readfds, NULL, NULL, &timeout);

            if (ret == 0) {
                kill(pid, SIGKILL);
                close(outPipe[0]);
                waitpid(pid, NULL, 0);
                return "HTTP/1.1 504 Gateway Timeout\r\n\r\nCGI Timeout";
            } else if (ret < 0) {
                close(outPipe[0]);
                waitpid(pid, NULL, 0);
                return "HTTP/1.1 500 Internal Server Error\r\n\r\nSelect failed";
            }

            int bytes = read(outPipe[0], buffer, sizeof(buffer));
            if (bytes <= 0)
                break;
            output.append(buffer, bytes);
        }
        close(outPipe[0]);
        waitpid(pid, NULL, 0);

        size_t position = output.find("\r\n\r\n");
        if (position != std::string::npos) {
            std::string headers = output.substr(0, position);
            std::string body = output.substr(position + 4);

            std::stringstream result;
            result << "HTTP/1.1 200 OK\r\n";
            result << headers << "\r\n";
            result << "Content-Length: " << body.size() << "\r\n";
            result << "\r\n";
            result << body;

            return result.str();
        }
        return buildHttpResponse(output);
	}
}