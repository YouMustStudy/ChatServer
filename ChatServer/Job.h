#pragma once
#include <WS2tcpip.h>
#include <string>
#include <vector> 
#include <memory>
#include <regex>

class MainJob
{
public:
	MainJob() {};
	MainJob(int cmd, SOCKET socket, std::string* data) : cmd(cmd), socket(socket), data(data) {};
	~MainJob()
	{
		if (nullptr != data)
		{
			delete data;
		}
	}

	int cmd{0};
	SOCKET socket{INVALID_SOCKET};
	std::string* data{nullptr};

};

class UserJob
{
public:
	UserJob() {};
	UserJob(int cmd, SOCKET socket, int errCode, const std::smatch& sMatch) : cmd(cmd), socket(socket), errCode(errCode)
	{
		for (const auto& str : sMatch)
		{
			data.emplace_back(str.str());
		}
	}
	UserJob(int cmd, SOCKET socket, int errCode, const std::string* str) : cmd(cmd), socket(socket), errCode(errCode)
	{
		if (nullptr != str) {
			data.emplace_back(*str);
		}
	};
	~UserJob(){};

	int cmd{0};
	SOCKET socket;
	int errCode;
	std::vector<std::string> data;
};