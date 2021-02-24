#pragma once
#include <WinSock2.h>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>

/**
*@brief ���� �ð��� ��ȯ�Ѵ�.
*@return [0000-00-00 HH:MM:SS] ������ ���ڿ��� ��ȯ�Ѵ�.
*/
std::string get_time();

/**
*@brief WSA���� ������ ���� �ֿܼ� ��� �� �α����Ϸ� �����Ѵ�.
*@param[in] msg ���� ���� �տ� �߰��� ���λ�.
*@param[in] errNo ���� �ڵ�.
*/
void error_display(const char* msg, int errNo);