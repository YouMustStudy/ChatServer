#pragma once
#include <ws2tcpip.h>
#include <string>
#include <unordered_map>
#include <memory>

class Room;
using RoomPtr = std::shared_ptr<Room>;
/**
*@brief ���� �����͸� �����Ѵ�.
*/
class User
{
public:
	User(SOCKET socket, SOCKADDR_IN addr);
	~User() {};

	/**
	*@brief �������� �޼����� �����Ѵ�.
	*@param[in] msg ������ �޼���.
	*/
	void SendChat(const std::string& msg);

	/**
	*@brief ������ �ִ� ���� �����͸� ��ȯ�Ѵ�.
	*@return ������ �ִ� ���� ������, ������ nullptr.
	*/
	RoomPtr GetRoom() const;

	/**
	*@brief ������ �ִ� ���� �����͸� �����Ѵ�.
	*@param[in] roomPtr ������ ���� ���� ������.
	*/
	void SetRoom(const RoomPtr &roomPtr);

	/**
	*@brief ���� �̸��� ��ȯ�Ѵ�.
	*@return ���� �̸�.
	*/
	std::string GetName() const;

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
	*@brief ���� ������ ����ִ� �� Ȯ���Ѵ�.
	*@return ����ִٸ� true, �׾����� false.
	*/
	bool IsAlive();

	/**
	*@brief ���� ������ �����Ų �� ��ŷ�Ѵ�.
	*/
	void Kill();

	/**
	*@brief ������ �α����ߴ� �� �ƴ��� �˷��ش�.
	*@return �α������� �� true, �ƴ� �� false.
	*/
	bool GetIsLogin();
	void SetLogin(const std::string& name);

	/**
	*@brief ������ �ּҰ��� ��ȯ�Ѵ�.
	*@return [IP:PORT] ������ ���ڿ�.
	*/
	std::string GetAddr();

	/**
	*@brief ���� ���ۿ� �����͸� ���������� �����Ѵ�. �齺���̽��� ó���Ѵ�.
	*@param[in] data �Է��� ������ ����.
	*@param[in] length �Է��� �������� ����.
	*/
	void PushData(const char* data, int length);
	std::string m_data;					///< ���� ����, 'select thread'������ �����Ѵ�.
private:
	std::string m_name;					///< ���� �г���
	RoomPtr m_room{nullptr};			///< ������ �� ������
	bool m_login{false};				///< �α��� ����
	bool m_isAlive{true};				///< ���� ���� ����
	SOCKET m_socket{INVALID_SOCKET};	///< ������ ����
	std::string m_addr;					///< ������ �ּ� ���� - [IP:PORT]
	
};

/**
*breif '�α�����' ���� ���� Ŭ����.
*/
class UserManager
{
	using UserPtr = std::shared_ptr<User>;
	using UserTable = std::unordered_map<std::string, UserPtr>;

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

	/**
	*@brief ������ ����ó���Ѵ�.
	*@param[in] user ������ ������ �̸�.
	*/
	void DisconnectUser(const std::string& userName);

	/**
	*@brief ������ ����ó���Ѵ�.
	*@param[in] user ������ ������ ������.
	*/
	void DisconnectUser(UserPtr& user);
private:
	UserManager() {};
	UserTable m_userTable;	///< ���� ����� �����̳�
};
//@brief UserManager ȣ�� ��ũ��.
#define g_userManager (UserManager::Instance())