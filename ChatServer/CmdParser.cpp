#include "CmdParser.h"
CmdParser::CmdParser() : m_regexs(CMD_COUNT - 1)
{
	Initialize();
}

int CmdParser::Parse(const std::string& data, std::smatch& param)
{
	//��ɾ�� '/'���� �����Ѵ�.
	if (data[0] != '/')
	{
		return CMD_CHAT;
	}
	//���� �� ����ǥ�������� �˻�, ��ġ�ϴ� ��ɾ� �ڵ� ��ȯ
	for (int cmd = 0; cmd != CMD_COUNT - 1; ++cmd)
	{
		if (true == std::regex_match(data, param, m_regexs[cmd]))
		{
			return cmd;
		}
	}
	//�ش������ ������ �����ڵ带 ��ȯ�Ѵ�.
	return CMD_ERROR;
}

void CmdParser::Initialize()
{
	m_regexs[CMD_HELP] = std::regex(R"((?i)^\/help$)");
	m_regexs[CMD_QUIT] = std::regex(R"((?i)^\/quit$)");
	m_regexs[CMD_ROOMLIST] = std::regex(R"((?i)^\/roomlist$)");
	m_regexs[CMD_USERLIST] = std::regex(R"((?i)^\/userlist$)");
	m_regexs[CMD_ALLUSERLIST] = std::regex(R"((?i)^\/alluserlist$)");
	m_regexs[CMD_JOIN] = std::regex(R"((?i)^\/join +([0-9]{1,})$)");
	m_regexs[CMD_LOGIN] = std::regex(R"((?i)^\/login +(\S{1,10})$)");
	m_regexs[CMD_MSG] = std::regex(R"((?i)^\/msg +(\S{1,}) +(.{1,})$)");
	m_regexs[CMD_CREATEROOM] = std::regex(R"((?i)^\/create +(\S{1,}) +([0-9]{1,})$)");
}