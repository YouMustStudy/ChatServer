#pragma once
#include <regex>
#include <string>

enum CMD
{
	CMD_QUIT,
	CMD_ROOMLIST,
	CMD_USERLIST,
	CMD_JOIN,
	CMD_MSG, 
	CMD_CREATEROOM,
	CMD_COUNT
};

/*
CmdParser
유저가 보낸 문자열에서 명령어를 파싱
*/
class CmdParser
{
public:
	CmdParser();
	~CmdParser();

private:
	std::regex m_regexs[CMD_COUNT];

	void Initialize();
};