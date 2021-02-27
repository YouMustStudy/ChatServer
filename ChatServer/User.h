#pragma once
#include <ws2tcpip.h>
#include <string>
#include <unordered_map>
#include <memory>

class Room;
/**
*@brief ���� �����͸� �����Ѵ�.
*/
class User
{
public:
	User(SOCKET soc, std::string addr);
	~User() {};

	/**
	*@brief �������� �޼����� �����Ѵ�.
	*@param[in] msg ������ �޼���.
	*/
	void SendChat(const std::string& msg);

public:
	std::string m_name;					///< ���� �̸�
	size_t m_room;						///< ������ ���ȣ
	bool m_login{false};				///< �α��� ����
	SOCKET m_socket;					///< ������ ����
	std::string m_addr;					///< ������ �ּ� ���� - [IP:PORT]
	int m_refCnt{ 1 };					///< ���� �������ϰ� �ִ� ����, ����, send �� �����ϰ�, send ����, ���� �� �����Ѵ�.
};

/**
*breif ���� ���� Ŭ����.
*/
class UserManager
{
	using UserTable = std::unordered_map<std::string, size_t>;
	using UserList = std::vector<User>;
	using SocketTable = std::unordered_map<SOCKET, size_t>;

public:
	/**
	*@brief �̱��� ȣ��� �Լ�. g_userManager�� ��ũ�ΰ� �Ǿ��ִ�.
	*@return �̱��� ��ü�� ���۷���.
	*/
	static UserManager& Instance();

	/**
	*@brief ������ ���̺� �߰��Ѵ�.
	*@param[in] user �߰��� ������ ������.
	*@param[in] userName �߰��� ������ �̸�
	*/
	void AddUser(SOCKET socket, const std::string& addr);

	/**
	*@brief ������ ���� ī��Ʈ�� ����. send �Ϸ� Ȥ�� ������ ���� �� ȣ��ȴ�. ����ī��Ʈ�� 0�� �Ǹ� ��ü�� �����Ѵ�.
	*@param[in] socket ���ҽ�ų ������ ����.
	*/
	void DecreaseUser(SOCKET socket);
	
	/**
	*@brief ������ �α��� ��Ų��. �Է��� �̸��� �ߺ��� ��� false ����.
	*@param[in] socket �α��� ��û�� ������ ����.
	*@param[in] name ������ �̸�.
	*@return ���� �� true, �Է��� �̸��� �ߺ��� ��� false ����.
	*/
	bool Login(SOCKET socket, const std::string& name);

	/**
	*@brief ���� �� ��� �������� �̸��� ��ȯ�Ѵ�.
	*@return �������� �̸��� ��� string ��ü.
	*/
	std::string GetUserList();

	/**
	*@brief ���ϰ� ��Ī�Ǵ� ������ ������ ��ȯ.
	*@param[in] socket �˻��� ������ ����.
	*@return ����������, �˻� ���� �� nullptr.
	*/
	User* GetUser(SOCKET socket);

	/**
	*@brief �̸��� ��Ī�Ǵ� ������ ������ ��ȯ.
	*@param[in] socket �˻��� ������ ����.
	*@return ����������, �˻� ���� �� nullptr.
	*/
	User* GetUser(const std::string& name);

	/**
	*@brief ���ϰ� ���εǴ� �������� �޼��� ����.
	*@param[in] socket ������ ������ ����.
	*@param[in] msg ������ ���ڿ�
	*/
	void SendMsg(SOCKET socket, const std::string& msg);

public:
	friend class UserProcesser;
private:
	void DisconnectUser(SOCKET socket);

	UserManager() {};
	UserTable m_userTable;										///< �̸��� �ε����� �����ϴ� ���̺�.
	UserList m_userList;										///< ���� ������� �����̳�.
	SocketTable m_SocketTable;									///< ���ϰ� ������ �����ϴ� ���̺�
};
//@brief UserManager ȣ�� ��ũ��.
#define g_userManager (UserManager::Instance())