#include "Client.hpp"
#include <exception>
#include <format>

Client::Client(const std::string& name, const std::string& hostIP) : _name(name)
{
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM; //TCP
	hints.ai_protocol = IPPROTO_TCP;

	int r = getaddrinfo(hostIP.c_str(), DefaultPort, &hints, &result);
	if(r != 0) 
		throw std::exception(std::format("getaddrinfo failed with code {}",r).c_str());

	for(ptr = result; ptr != nullptr; ptr = ptr->ai_next)
	{
		connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if(connectSocket == INVALID_SOCKET)
			throw std::exception(std::format("socket() failed with code {}",WSAGetLastError()).c_str());

		r = connect(connectSocket, ptr->ai_addr, ptr->ai_addrlen);
		if(r == SOCKET_ERROR)
		{
			closesocket(connectSocket);
			connectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}
	freeaddrinfo(result);

	if(connectSocket == INVALID_SOCKET) throw std::exception("Connection failed!");
	else _alive = true;
}

void Client::SendMessage(const std::string& msg)
{
	std::string nmsg(msg);

	size_t size = nmsg.length();
	
	char* data = new char[size + 2]; // [expected message lenght - size_t] [message - char[] ]
	memset(data, 0, size + 1);

	data[0] = size+1; //
	memcpy_s(data + 1, size, nmsg.c_str(), size); //MSVC warns if using the plain CRT memcpy().

	int r = send(connectSocket, data, size + 1, 0);

	if(r == SOCKET_ERROR)
	{
		throw std::exception(std::format("send() failed with code {}",WSAGetLastError()).c_str());
	}
}

std::string Client::WaitForMessage()
{
	if(!_alive) return std::string("");

	char data[MaxMessageLen]{ 0 };

	int r = recv(connectSocket, data, MaxMessageLen, 0);
	if(r == 0) return "";
	if(r < 0) throw std::exception(std::format("recv() failed with code {}", WSAGetLastError()).c_str());

	size_t msgLen = data[0] - 1;
	int expectedLenght = std::max<int>(0, msgLen - 1);

	std::string message;
	if(expectedLenght < msgLen)
	{
		message += data;
		expectedLenght -= strlen(data);
		if(expectedLenght > 0) message += WaitForMessage(message,expectedLenght);
	}
	return std::string(message);
}

std::string Client::WaitForMessage(std::string& currMsg,int expectedLenght)
{
	char data[MaxMessageLen]{ 0 };

	int r = recv(connectSocket, data, MaxMessageLen, 0);
	if(r == 0) return "";
	if(r < 0) throw std::exception(std::format("recv() failed with code {}", WSAGetLastError()).c_str());

	if(r < expectedLenght)
	{
		currMsg += data;
		expectedLenght -= strlen(data);
		if(expectedLenght > 0) currMsg += WaitForMessage(currMsg, expectedLenght);
	}
	return currMsg;
}

bool Client::Valid()
{
	return _alive;
}

Client::~Client()
{
	if(connectSocket != INVALID_SOCKET)
	{
		shutdown(connectSocket, SD_SEND);
		closesocket(connectSocket);
	}
}