#pragma once
#include <regex>
enum class CMD
{
	JOIN,
	QUIT,
	ROOMLIST,
	USERLIST,
	MSG,
	COUNT
};

class CmdParser
{
public:
	CmdParser();
	~CmdParser();
};