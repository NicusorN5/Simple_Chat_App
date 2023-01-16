#pragma once
#include <string>
#define NOMINMAX
#include <WinSock2.h>
#include <ws2tcpip.h>

#include "Globals.hpp"

#undef SendMessage //Win32 API is dumb

class Client
{
	std::string _name;
	
	SOCKET connectSocket = INVALID_SOCKET;
	addrinfo* result = nullptr;
	addrinfo* ptr = nullptr;
	addrinfo hints{};

	bool _alive;

	std::string WaitForMessage(std::string& currMsg,int expectedLength);
public:
	Client(const std::string& name,const std::string& hostIP);

	void SendMessage(const std::string& msg);
	std::string WaitForMessage();

	bool Valid();

	~Client();
};