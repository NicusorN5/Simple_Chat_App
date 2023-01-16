#include "Server.hpp"
#include <exception>
#include <format>
#include "ConnectionClosedException.hpp"

Server::Server(const std::string& name) : _name(name)
{
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    int r = getaddrinfo(nullptr, DefaultPort, &hints, &result);
    if(r != 0) throw std::exception(std::format("getaddrinfo failed with {}", r).c_str());
    
    listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if(listenSocket == INVALID_SOCKET) throw std::exception(std::format("socket failed with {}", WSAGetLastError()).c_str());

    r = bind(listenSocket, result->ai_addr, result->ai_addrlen);
    if(r == SOCKET_ERROR) throw std::exception(std::format("bind failed with {}", WSAGetLastError()).c_str());
    freeaddrinfo(result);

    r = listen(listenSocket, SOMAXCONN);
    if(r == SOCKET_ERROR) throw std::exception(std::format("listen failed with {}",WSAGetLastError()).c_str());       
}

void Server::SendMessage(const std::string& msg)
{
    std::string nmsg(msg);

    size_t size = nmsg.length();

    char* data = new char[size + 2]; // [expected message lenght - size_t] [message - char[] ]
    memset(data, 0, size + 1);

    data[0] = size + 1; //
    memcpy_s(data + 1, size, nmsg.c_str(), size); //MSVC warns if using the plain CRT memcpy().

    int r = send(clientSocket, data, size + 1, 0);

    if(r == SOCKET_ERROR)
    {
        throw std::exception(std::format("send() failed with code {}", WSAGetLastError()).c_str());
    }
}

std::string Server::WaitForMessage(std::string& currentMsg, int expectedLenght)
{
    char data[MaxMessageLen]{ 0 };

    int r = recv(clientSocket, data, MaxMessageLen, 0);
    if(r == 0) return "";
    if(r < 0) throw std::exception(std::format("recv() failed with code {}", WSAGetLastError()).c_str());

    if(expectedLenght < r)
    {
        currentMsg += data + 1;
        expectedLenght -= strlen(data);
        if(expectedLenght > 0) currentMsg += WaitForMessage(currentMsg, expectedLenght);
    }
    return currentMsg;
}

void Server::WaitForClient()
{
    if(listenSocket == INVALID_SOCKET) throw std::exception("The listen socket is invalid.");
    if(clientSocket == INVALID_SOCKET)
    {
        clientSocket = accept(listenSocket, nullptr, 0);
        if(clientSocket == INVALID_SOCKET) throw std::exception(std::format("accept failed with {}", WSAGetLastError()).c_str());

        closesocket(listenSocket);
        listenSocket = INVALID_SOCKET; //use-after-free type of errors/exploits are possible if this isn't done. 
    }
}

std::string Server::WaitForMessage()
{
    char data[MaxMessageLen]{ 0 };

    int r = recv(clientSocket, data, MaxMessageLen, 0);
    if(r == 0) throw ConnectionClosedException();
    if(r < 0) throw std::exception(std::format("recv failed with {}", WSAGetLastError()).c_str());

    size_t msgLen = data[0] - 1;
    int expectedLenght = std::max<int>(0, msgLen - 1);

    std::string message;
    if(expectedLenght < msgLen)
    {
        message += data + 1;
        expectedLenght -= strlen(data);
        if(expectedLenght > 0) message += WaitForMessage(message, expectedLenght);
    }
    return std::string(message);
}

Server::~Server()
{
    if(clientSocket != INVALID_SOCKET)
    {
        shutdown(clientSocket, SD_SEND);
        closesocket(clientSocket);
    }
    if(listenSocket != INVALID_SOCKET) closesocket(listenSocket);
}
