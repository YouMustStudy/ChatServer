#pragma once
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include "UtilFunc.h"


/**
*@brief WSA���� ������ ���� �ֿܼ� ��� �� �α����Ϸ� �����Ѵ�.
*@param[in] msg ���� ���� �տ� �߰��� ���λ�.
*@param[in] errNo ���� �ڵ�.
*/
void error_display(const char* msg, int errNo);