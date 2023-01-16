#pragma once
#include <exception>

class ConnectionClosedException : public std::exception
{
public:
	ConnectionClosedException() {};
	ConnectionClosedException(const char* msg) : std::exception(msg) {}
};

