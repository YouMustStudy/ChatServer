#pragma once
#include <regex>
#include <string>
#include <functional>
#include <vector>
#include "User.h"

/**
*@brief ä�� ��ɾ� ���
*/
enum CMD_TYPE
{
	CMD_CHAT = -1,		///< ä��
	CMD_HELP,			///< ����
	CMD_QUIT,			///< ����
	CMD_ROOMLIST,		///< �� ���
	CMD_USERLIST,		///< �� �� ���� ���
	CMD_ALLUSERLIST,	///< ���� �� ���� ���
	CMD_JOIN,			///< ����
	CMD_LOGIN,			///< �α���
	CMD_MSG,			///< �ӼӸ�
	CMD_CREATEROOM,		///< �� ����
	CMD_ERROR,			///< ����
	CMD_COUNT			///< ��ɾ� �� ����
};
/**
*@brief ���ڿ����� ��� Ÿ��, �μ��� �Ľ��ϴ� ��ü.
*/
class CmdParser
{
	using UserPtr = std::shared_ptr<User>;

public:
	CmdParser();

	/**
	*@brief ���ڿ��� �޾Ƽ� ��� Ÿ��, �μ��� �����ش�.
	*@param[in] data �Ľ��� ���ڿ�.
	*@param[out] �μ��� ����� smatch ��ü.
	*@return ��ɿ� ���� CMD_TYPE���� ��ȯ.
	*/
	int Parse(const std::string &data, std::smatch& result);

private:
	std::vector<std::regex> m_regexs;	///> ��ɾ� �Ľ̿� ����ǥ���� ��ü

	/**
	*@brief regex��ü���� �ʱ�ȭ��.
	*/
	void Initialize();
};