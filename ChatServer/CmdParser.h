#pragma once
#include <regex>
#include <string>
#include <functional>
#include <vector>
#include "User.h"

/**
*@brief 채팅 명령어 목록
*/
enum CMD_TYPE
{
	CMD_CHAT = -1,		///< 채팅
	CMD_HELP,			///< 도움말
	CMD_QUIT,			///< 퇴장
	CMD_ROOMLIST,		///< 방 목록
	CMD_USERLIST,		///< 방 내 유저 목록
	CMD_ALLUSERLIST,	///< 서버 내 유저 목록
	CMD_JOIN,			///< 입장
	CMD_LOGIN,			///< 로그인
	CMD_MSG,			///< 귓속말
	CMD_CREATEROOM,		///< 방 생성
	CMD_ERROR,			///< 오류
	CMD_COUNT			///< 명령어 총 갯수
};
/**
*@brief 문자열에서 명령 타입, 인수를 파싱하는 객체.
*/
class CmdParser
{
	using UserPtr = std::shared_ptr<User>;

public:
	CmdParser();

	/**
	*@brief 문자열을 받아서 명령 타입, 인수로 나눠준다.
	*@param[in] data 파싱할 문자열.
	*@param[out] 인수가 저장될 smatch 객체.
	*@return 명령에 대한 CMD_TYPE값을 반환.
	*/
	int Parse(const std::string &data, std::smatch& result);

private:
	std::vector<std::regex> m_regexs;	///> 명령어 파싱용 정규표현식 객체

	/**
	*@brief regex객체들을 초기화함.
	*/
	void Initialize();
};