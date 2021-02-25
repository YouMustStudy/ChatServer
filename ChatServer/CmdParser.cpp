#include "CmdParser.h"
CmdParser::CmdParser() : m_regexs(CMD_COUNT-1)
{
	Initialize();
}

int CmdParser::Parse(const std::string &data, std::smatch& param)
{
	//명령어는 '/'으로 시작한다.
	if (data[0] != '/')
	{
		return CMD_CHAT;
	}

	//이후 각 정규표현식으로 검사, 일치하는 명령어 코드 반환
	for (int cmd = 0; cmd != CMD_COUNT - 1; ++cmd)
	{
		if (true == std::regex_match(data, param, m_regexs[cmd]))
		{
			return cmd;
		}
	}

	//해당사항이 없으면 오류코드를 반환한다.
	return CMD_ERROR;
}

void CmdParser::Initialize()
{
	//각 명령어에 대응하는 정규표현식
	m_regexs[CMD_HELP] = std::regex(R"(^\/help$)");
	m_regexs[CMD_QUIT] = std::regex(R"(^\/quit$)");
	m_regexs[CMD_ROOMLIST] = std::regex(R"(^\/roomlist$)");
	m_regexs[CMD_USERLIST] = std::regex(R"(^\/userlist$)");
	m_regexs[CMD_ALLUSERLIST] = std::regex(R"(^\/alluserlist$)");
	m_regexs[CMD_JOIN] = std::regex(R"(^\/join +([0-9]{1,})$)");
	m_regexs[CMD_LOGIN] = std::regex(R"(^\/login +(\S+)$)");
	m_regexs[CMD_MSG] = std::regex(R"(^\/msg +(\S{1,}) +(.{1,})$)");
	m_regexs[CMD_CREATEROOM] = std::regex(R"(^\/create +(\S{1,}) +([0-9]{1,})$)");
}