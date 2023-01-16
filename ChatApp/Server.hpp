#pragma once
#include "Client.hpp"

class Server
{
	std::string _name;

	SOCKET clientSocket = INVALID_SOCKET;
	SOCKET listenSocket = INVALID_SOCKET;

	addrinfo* result = nullptr;
	addrinfo hints{};

	std::string WaitForMessage(std::string& currentMsg, int expectedLenght);
public:
	Server(const std::string &name);

	void SendMessage(const std::string& msg);

	void WaitForClient();
	std::string WaitForMessage();

	~Server();
}; 