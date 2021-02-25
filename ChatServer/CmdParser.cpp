#include "CmdParser.h"
CmdParser::CmdParser() : m_regexs(CMD_COUNT - 1)
{
	Initialize();
}

int CmdParser::Parse(const std::string& data, std::smatch& param)
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

void CmdParser::EraseBackWhiteSpace(std::string& data)
{
	while (false == data.empty() &&
		' ' == data.back())
	{
		data.pop_back();
	}
	return;
}

void CmdParser::Initialize()
{
	m_regexs[CMD_HELP] = std::regex(R"(^\/help *$)", std::regex_constants::syntax_option_type::icase);
	m_regexs[CMD_QUIT] = std::regex(R"(^\/quit *$)", std::regex_constants::syntax_option_type::icase);
	m_regexs[CMD_ROOMLIST] = std::regex(R"(^\/roomlist *$)", std::regex_constants::syntax_option_type::icase);
	m_regexs[CMD_USERLIST] = std::regex(R"(^\/userlist *$)", std::regex_constants::syntax_option_type::icase);
	m_regexs[CMD_ALLUSERLIST] = std::regex(R"(^\/alluserlist *$)", std::regex_constants::syntax_option_type::icase);
	m_regexs[CMD_JOIN] = std::regex(R"(^\/join +(-*[0-9]+) *$)", std::regex_constants::syntax_option_type::icase);
	m_regexs[CMD_LOGIN] = std::regex(R"(^\/login +(\S+) *$)", std::regex_constants::syntax_option_type::icase);
	m_regexs[CMD_MSG] = std::regex(R"(^\/msg +(\S+) +(.+)$)", std::regex_constants::syntax_option_type::icase);
	m_regexs[CMD_CREATEROOM] = std::regex(R"(^\/create +(\S+) +(-*[0-9]+) *$)", std::regex_constants::syntax_option_type::icase);
}