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
유저가 보낸 문자열에서 명령어를 파싱
*/

class CmdParser
{
	typedef std::shared_ptr<User> UserPtr;
	typedef std::function<void(const UserPtr& user, void* data)> UserCallback;

public:
	CmdParser();
	~CmdParser();
	/// 문자열을 받아서 파싱 후 실행하는 함수. 인자 : user - 실행할 유저, data - 처리할 데이터
	int Parse(std::string &data, std::smatch& result);

private:
	std::vector<std::regex> m_regexs;			/// 명령어 파싱용 정규표현식 객체

	void Initialize();
};