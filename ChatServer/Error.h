#pragma once
#include <WinSock2.h>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>

/**
*@brief 현재 시간을 반환한다.
*@return [0000-00-00 HH:MM:SS] 형식의 문자열을 반환한다.
*/
std::string get_time();

/**
*@brief WSA관련 오류를 서버 콘솔에 출력 및 로그파일로 저장한다.
*@param[in] msg 오류 내용 앞에 추가할 접두사.
*@param[in] errNo 오류 코드.
*/
void error_display(const char* msg, int errNo);