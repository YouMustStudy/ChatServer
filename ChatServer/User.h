#pragma once
#include <WinSock2.h>
#include <string>
#include <map>
#include <memory>

class Room;
using RoomPtr = std::shared_ptr<Room>;
/**
*@brief ���� �����͸� �����Ѵ�.
*/
class User
{
public:
	User() : m_socket(INVALID_SOCKET), m_name(), m_data(), m_room(nullptr), m_login(false) {};
	User(SOCKET socket) : m_socket(socket), m_name(), m_data(), m_room(nullptr), m_login(false) {};
	~User() {};

	/**
	*@brief �������� �޼����� �����Ѵ�.
	*@param[in] msg ������ �޼���.
	*@return ������ ����Ʈ ��.
	*/
	int SendChat(const std::string& msg);

	/**
	*@brief ������ �ִ� ���� �����͸� ��ȯ�Ѵ�.
	*@return ������ �ִ� ���� ������, ������ nullptr.
	*/
	RoomPtr GetRoom();

	/**
	*@brief ������ �ִ� ���� �����͸� �����Ѵ�.
	*@param[in] roomPtr ������ ���� ���� ������.
	*/
	void SetRoom(const RoomPtr &roomPtr);

	/**
	*@brief ���� �̸��� ��ȯ�Ѵ�.
	*@return ���� �̸�.
	*/
	std::string GetName();

	/**
	*@brief ������ �̸��� �����Ѵ�.
	*@param[in] name ������ �̸�.
	*/
	void SetName(const std::string& name);
	

	/**
	*@brief ���� ������ ��ȯ�Ѵ�.
	*@return ���� ����.
	*/
	SOCKET GetSocket();

	/**
	*@brief ������ ������ �����Ѵ�.
	*@param[in] socket ������ ����.
	*/
	void SetSocket(SOCKET socket);

	/**
	*@brief ������ �α����ߴ� �� �ƴ��� �˷��ش�.
	*@return �α������� �� true, �ƴ� �� false.
	*/
	bool GetIsLogin();
	void SetLogin(const std::string& name);

	/**
	*@brief ���� ���ۿ� �����͸� ���������� �����Ѵ�. �齺���̽��� ó���Ѵ�.
	*@param[in] data �Է��� ������ ����.
	*@param[in] length �Է��� �������� ����.
	*/
	void PushData(const char* data, int length);
	std::string m_data;		///< ���� ����, 'select thread'������ �����Ѵ�.
private:
	std::string m_name;		///< ���� �г���
	RoomPtr m_room;			///< ������ �� ������
	bool m_login;			///< �α��� ����
	SOCKET m_socket;
	
};

/**
*breif '�α�����' ���� ���� Ŭ����.
*/
class UserManager
{
	using UserPtr = std::shared_ptr<User>;
	using UserTable = std::map<std::string, UserPtr>;

public:
	/**
	*@brief �̱��� ȣ��� �Լ�. g_userManager�� ��ũ�ΰ� �Ǿ��ִ�.
	*@return �̱��� ��ü�� ���۷���.
	*/
	static UserManager& Instance();

	/**
	*@brief ������ �̸��� �˻�, �����͸� ��ȯ�Ѵ�.
	*@param[in] userName �˻��� ������ �̸�.
	*@return ���� �� �ش� ������ ������, ���� �� nullptr.
	*/
	UserPtr GetUser(const std::string& userName);
	
	/**
	*@brief ������ ���̺� �߰��Ѵ�.
	*@param[in] user �߰��� ������ ������.
	*@param[in] userName �߰��� ������ �̸�
	*@return ���� �� true, �г��� �ߺ� �� false.
	*/
	bool AddUser(UserPtr& user, const std::string& userName);

	/**
	*@brief ������ ���� ���̺��� �����Ѵ�.
	*@param[in] user ������ ������ �̸�
	*@return ������ ��ü�� ��, ���� �� 1, ���� �� 0.
	*/
	size_t EraseUser(const UserPtr& user);
	
	/**
	*@brief ���� �� ��� �������� �̸��� ��ȯ�Ѵ�.
	*@return �������� �̸��� ��� string ��ü.
	*/
	std::string GetUserList();
private:
	UserManager();
	UserTable m_userTable;	///���� ����� �����̳�
};
//@brief UserManager ȣ�� ��ũ��.
#define g_userManager (UserManager::Instance())