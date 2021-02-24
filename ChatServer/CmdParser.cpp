#include "CmdParser.h"
CmdParser::CmdParser() : m_regexs(CMD_COUNT-1)
{
	Initialize();
}

CmdParser::~CmdParser()
{
}

int CmdParser::Parse(std::string &data, std::smatch& param)
{
	if (data[0] != '/')
	{
		return CMD_CHAT;
	}
	for (int cmd = 0; cmd != CMD_COUNT - 1; ++cmd)
	{
		if (true == std::regex_match(data, param, m_regexs[cmd]))
		{
			return cmd;
		}
	}
	return CMD_ERROR;
}

void CmdParser::Initialize()
{
	m_regexs[CMD_HELP] = std::regex(R"(^\/help$)");
	m_regexs[CMD_QUIT] = std::regex(R"(^\/quit$)");
	m_regexs[CMD_ROOMLIST] = std::regex(R"(^\/roomlist$)");
	m_regexs[CMD_USERLIST] = std::regex(R"(^\/alluserlist$)");
	m_regexs[CMD_ALLUSERLIST] = std::regex(R"(^\/userlist$)");
	m_regexs[CMD_JOIN] = std::regex(R"(^\/join +([0-9]{1,})$)");
	m_regexs[CMD_LOGIN] = std::regex(R"(^\/login +(\S{1,10})$)");
	m_regexs[CMD_MSG] = std::regex(R"(^\/msg +(\S{1,}) +(.{1,})$)");
	m_regexs[CMD_CREATEROOM] = std::regex(R"(^\/create +(\S{1,}) +([0-9]{1,})$)");
}