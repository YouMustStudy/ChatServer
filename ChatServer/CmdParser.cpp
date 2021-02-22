#include "CmdParser.h"
CmdParser::CmdParser()
{
}


CmdParser::~CmdParser()
{
}

void CmdParser::Initialize()
{
	m_regexs[CMD_QUIT] = std::regex(R"(^\/quit$)");
	m_regexs[CMD_ROOMLIST] = std::regex(R"(^\/roomlist$)");
	m_regexs[CMD_USERLIST] = std::regex(R"(^\/userlist$)");
	m_regexs[CMD_JOIN] = std::regex(R"(^\/join +([0-9]{1,})$)");
	m_regexs[CMD_MSG] = std::regex(R"(^\/msg +(\S{1,}) +(.{1,})$)");
	m_regexs[CMD_CREATEROOM] = std::regex(R"(^\/create +(\S{1,}) +(.{1,})$)");
}
