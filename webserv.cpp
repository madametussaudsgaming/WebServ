/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: alechin <alechin@student.42kl.edu.my>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/17 10:45:51 by alechin           #+#    #+#             */
/*   Updated: 2026/03/26 15:36:34 by alechin          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"
#include "Utilitys.hpp"

int setNonBlocking(int fd) {
	return fcntl(fd, F_SETFL, O_NONBLOCK);
}

//if (argumentCounter != 4)
	//	Error2exit("Error: Incorrect amount of arguments", 1);
int	main(int argumentCounter, char **argumentVector) {
	struct sockaddr_in address;
	int		serverFD;
	// Create the Socket
	(void)argumentCounter;
	(void)argumentVector;
	serverFD = socket(AF_INET, SOCK_STREAM, 0);
	if (serverFD < 0) {
		Error2exit("Socket Failed\n", 1);
		return 1;
	}

	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	if (bind(serverFD, (struct sockaddr *)&address, sizeof(address)) < 0)
		Error2exit("Could not bind\n", 1);
	if (listen(serverFD, 10) < 0)
		Error2exit("Couldn't listen\n", 1);
	setNonBlocking(serverFD);
	std::vector<struct pollfd>fds;
	struct pollfd serverPoll;
	serverPoll.fd = serverFD;
	serverPoll.events = POLLIN;
	fds.push_back(serverPoll);

	simplePrint("Server running in port", PORT);

	while (true) {
		poll(&fds[0], fds.size(), -1);
		for (size_t i = 0; i < fds.size(); i++) {
			if (fds[i].fd == serverFD && (fds[i].revents & POLLIN)) {
				int clientFD = accept(serverFD, NULL, NULL);
				if (clientFD >= 0) {
					setNonBlocking(clientFD);
					struct pollfd clientPoll;
					
					clientPoll.fd = clientFD;
					clientPoll.events = POLLIN;
					fds.push_back(clientPoll);

					std::cout << "New Client Connected\n";
				}
			}
			else if (fds[i].revents & POLLIN) {
				char	buffer[BUFFER_SIZE];
				int		bytes = recv(fds[i].fd, buffer, BUFFER_SIZE - 1, 0);
				std::memset(buffer, 0, BUFFER_SIZE);

				if (bytes <= 0) {
					close(fds[i].fd);
					fds.erase(fds.begin() + i);
					i--;
					std::cout << "Client Disconnected\n";
				} else {
					std::cout << "Request:\n" << buffer << std::endl;

                    const char *response =
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Length: 12\r\n"
                        "Content-Type: text/plain\r\n"
                        "\r\n"
                        "Hello World";
                    send(fds[i].fd, response, strlen(response), 0);
                    close(fds[i].fd);
                    fds.erase(fds.begin() + i);
                    i--;
				}
			}
		}
	}
	return (0);
}
