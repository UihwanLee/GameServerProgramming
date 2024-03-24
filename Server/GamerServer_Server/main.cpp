#include "pch.h"
#include "Figure.h"

constexpr short PORT = 4000;
constexpr char SERVER_ADDR[] = "127.0.0.1";
constexpr int BUFSIZE = 256;

void print_error(const char* msg, int err_no);

void CALLBACK recv_callback(DWORD err, DWORD recv_size, LPWSAOVERLAPPED pover, DWORD recv_flag);
void CALLBACK send_callback(DWORD err, DWORD sent_size, LPWSAOVERLAPPED pover, DWORD recv_flag);

void send_other_people(int id);

#pragma pack (push, 1)
struct packet {
	short		size;
	int			type;
	int			serverID;
	int			playerPosIDX;
	int			otherSeverID;
	glm::vec3	pos;
};
#pragma pack (pop)

GLvoid OverlappedIO();

int startPlayerPosIDX = 35;
int serverID = 0;
std::vector<int> playerList;

std::unordered_map<LPWSAOVERLAPPED, int> g_seesion_map;

class SESSION {
public:
	packet receivedPacket;
	char buf[BUFSIZE];
	WSABUF wsabuf;
	SOCKET client_s;
	WSAOVERLAPPED over;
public:
	SESSION(SOCKET s, int my_id) : client_s(s) {
		g_seesion_map[&over] = my_id;
	}
	SESSION() {
		std::cout << "ERROR";
		exit(-1);
	}
	~SESSION() { closesocket(client_s); }

	void do_recv()
	{
		wsabuf.len = sizeof(packet);
		wsabuf.buf = reinterpret_cast<CHAR*>(&receivedPacket);
		DWORD recv_size;
		DWORD recv_flag = 0;
		ZeroMemory(&over, sizeof(over));
		int res = WSARecv(client_s, &wsabuf, 1, &recv_size, &recv_flag, &over, recv_callback);

		if (0 != res) {
			int err_no = WSAGetLastError();
			if (WSA_IO_PENDING != err_no)
				print_error("WSARecv", WSAGetLastError());
		}
	}

	void check_type()
	{
		if (receivedPacket.type == 0)									createPlayer();
		else if (receivedPacket.type >= 1 && receivedPacket.type <= 4)	move_player(receivedPacket.type);
	}

	void createPlayer()
	{
		std::cout << "[Server] Ŭ���̾�Ʈ�� ���� �÷��̾� ���� ��û" << std::endl;
		receivedPacket.playerPosIDX = startPlayerPosIDX;
		receivedPacket.pos = Figure::Boards[receivedPacket.playerPosIDX];
		
		// �������� ���� id���� �����ϰ� id�� �ο�
		playerList.emplace_back(serverID);
		receivedPacket.serverID = serverID++;

		receivedPacket.otherSeverID = 0;

		wsabuf.len = sizeof(packet);
		wsabuf.buf = reinterpret_cast<CHAR*>(&receivedPacket);

		WSASend(client_s, &wsabuf, 1, nullptr, 0, &over, send_callback);
	}

	void createOtherPlayer()
	{
		int my_id = g_seesion_map[&over];
		std::cout << "[Server] Ŭ���̾�Ʈ " << my_id << "������ �ٸ� �÷��̾� �����ϵ��� ����" << std::endl;

		// �ٸ� �÷��̾� �����϶�� tpye
		receivedPacket.type = 5;

		receivedPacket.playerPosIDX = startPlayerPosIDX;
		receivedPacket.pos = Figure::Boards[receivedPacket.playerPosIDX];

		wsabuf.len = sizeof(packet);
		wsabuf.buf = reinterpret_cast<CHAR*>(&receivedPacket);

		WSASend(client_s, &wsabuf, 1, nullptr, 0, &over, send_callback);
	}

	void move_player(int type)
	{
		if (receivedPacket.type == 1)		goLeft();
		else if (receivedPacket.type == 2)	goRight();
		else if (receivedPacket.type == 3)	goUp();
		else if (receivedPacket.type == 4)	goDown();

		int my_id = g_seesion_map[&over];
		std::cout << "Client[" << my_id << "]���� ���� pos�� ����";
		std::cout << "Client pos: (" << receivedPacket.pos.x << ", " << receivedPacket.pos.y << ", " << receivedPacket.pos.z << ")" << std::endl;

		WSASend(client_s, &wsabuf, 1, nullptr, 0, &over, send_callback);
	}

	void moveOtherPlayer(int serverID)
	{

	}

	GLvoid goLeft()
	{
		if (receivedPacket.playerPosIDX % 8 - 1 < 0) return;

		// ���� �̵�
		receivedPacket.playerPosIDX = receivedPacket.playerPosIDX - 1;
		receivedPacket.pos = Figure::Boards[receivedPacket.playerPosIDX];
	}

	GLvoid goRight()
	{
		if (receivedPacket.playerPosIDX % 8 + 1 >= 8) return;

		// ������ �̵�
		receivedPacket.playerPosIDX = receivedPacket.playerPosIDX + 1;
		receivedPacket.pos = Figure::Boards[receivedPacket.playerPosIDX];
	}

	GLvoid goUp()
	{
		if (receivedPacket.playerPosIDX - 8 < 0) return;

		// ���� �̵�
		receivedPacket.playerPosIDX = receivedPacket.playerPosIDX - 8;
		receivedPacket.pos = Figure::Boards[receivedPacket.playerPosIDX];
	}

	GLvoid goDown()
	{
		if (receivedPacket.playerPosIDX + 8 >= Figure::Boards.size()) return;

		// �Ʒ��� �̵�
		receivedPacket.playerPosIDX = receivedPacket.playerPosIDX + 8;
		receivedPacket.pos = Figure::Boards[receivedPacket.playerPosIDX];
	}
};

std::unordered_map<int, SESSION> g_players;

void print_error(const char* msg, int err_no)
{
	WCHAR *msg_buf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPWSTR>(& msg_buf), 0, NULL);

	std::cout << msg;
	std::wcout << L": ���� : " << msg_buf;
	while (true);
	LocalFree(msg_buf);
}

bool b_shutdown = false;

void CALLBACK send_callback(DWORD err, DWORD sent_size, LPWSAOVERLAPPED pover, DWORD recv_flag)
{
	if (0 != err) {
		print_error("WSASent", WSAGetLastError());
	}

	g_players[g_seesion_map[pover]].do_recv();
}

void CALLBACK recv_callback(DWORD err, DWORD recv_size, LPWSAOVERLAPPED pover, DWORD recv_flag)
{
	int my_id = g_seesion_map[pover];

	if (0 != err) {
		print_error("WSARecv", WSAGetLastError());
	}
	if (0 == recv_size) {
		g_players.erase(my_id);
		return;
	}

	g_players[my_id].check_type();

	std::cout << "[Server] my_id:" << my_id << std::endl;

	// �ٸ� Ŭ���̾�Ʈ���� �ش� �÷��̾��� ���� �����̶�� ����
	//send_other_people(my_id);
}

void send_other_people(int id)
{
	for (auto& player : g_players)
	{
		if (player.first != id)
		{
			player.second.createOtherPlayer();
		}
	}
}

int main(void)
{
	OverlappedIO();
}

GLvoid OverlappedIO()
{
	std::wcout.imbue(std::locale("korean"));

	// window ��Ʈ��ũ ���α׷��� �� ������ ���� ���α׷����� ȣȯ���� ���� �ʿ�
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 0), &WSAData);

	// SOCKET ����
	SOCKET server_s = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (server_s == INVALID_SOCKET)
	{
		std::cerr << "Failed to create server socket." << std::endl;
		WSACleanup();
		return;
	}

	std::cout << "[Server] ���� Ŭ���̾�Ʈ ���� ���" << std::endl;

	// SOCK ADDR ����
	SOCKADDR_IN server_a;

	server_a.sin_family = AF_INET;
	server_a.sin_port = htons(PORT);
	server_a.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	bind(server_s, reinterpret_cast<sockaddr*>(&server_a), sizeof(server_a));
	listen(server_s, SOMAXCONN);
	int addr_size = sizeof(server_a);
	int id = 0;
	while (false == b_shutdown) {
		SOCKET client_s = WSAAccept(server_s, reinterpret_cast<sockaddr*>(&server_a), &addr_size, nullptr, 0);
		std::cout << "[Server] Ŭ���̾�Ʈ " << id << "�� ���� ����" << std::endl;
		g_players.try_emplace(id, client_s, id);
		g_players[id++].do_recv();
	}
	g_players.clear();
	std::cout << "[Server] ���� ����!" << std::endl;
	closesocket(server_s);
	WSACleanup();
}