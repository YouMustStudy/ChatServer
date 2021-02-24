#pragma once
#include <regex>
#include <string>
#include <functional>
#include <vector>
#include "User.h"

enum CMD
{
	CMD_CHAT = -1,
	CMD_HELP,
	CMD_QUIT,
	CMD_ROOMLIST,
	CMD_USERLIST,
	CMD_JOIN,
	CMD_LOGIN,
	CMD_MSG, 
	CMD_CREATEROOM,

	CMD_ERROR,
	CMD_COUNT
};
/*
CmdParser
������ ���� ���ڿ����� ��ɾ �Ľ�
*/

class CmdParser
{
	typedef std::shared_ptr<User> UserPtr;
	typedef std::function<void(const UserPtr& user, void* data)> UserCallback;

public:
	CmdParser();
	~CmdParser();
	/// ���ڿ��� �޾Ƽ� �Ľ� �� �����ϴ� �Լ�. ���� : user - ������ ����, data - ó���� ������
	int Parse(std::string &data, std::smatch& result);

private:
	std::vector<std::regex> m_regexs;			/// ��ɾ� �Ľ̿� ����ǥ���� ��ü

	void Initialize();
};