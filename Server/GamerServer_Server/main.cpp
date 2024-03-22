#include "pch.h"
#include "Figure.h"
#include <unordered_map>

constexpr short PORT = 4000;
constexpr char SERVER_ADDR[] = "127.0.0.1";
constexpr int BUFSIZE = 256;

void print_error(const char* msg, int err_no);

void CALLBACK recv_callback(DWORD err, DWORD recv_size, LPWSAOVERLAPPED pover, DWORD recv_flag);
void CALLBACK send_callback(DWORD err, DWORD sent_size, LPWSAOVERLAPPED pover, DWORD recv_flag);

#pragma pack (push, 1)
struct move_packet {
	short size;
	char  type;
	int   idx;
	glm::vec3 pos;
};
#pragma pack (pop)

GLvoid OverlappedIO();
GLvoid Non_blocking();

std::unordered_map<LPWSAOVERLAPPED, int> g_seesion_map;

class SESSION {
	char buf[BUFSIZE];
	WSABUF wsabuf[1];
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
		wsabuf[0].buf = buf;
		wsabuf[0].len = BUFSIZE;
		DWORD recv_size;
		DWORD recv_flag = 0;
		ZeroMemory(&over, sizeof(over));
		int res = WSARecv(client_s, wsabuf, 1, &recv_size, &recv_flag, &over, recv_callback);

		if (0 != res) {
			int err_no = WSAGetLastError();
			if (WSA_IO_PENDING != err_no)
				print_error("WSARecv", WSAGetLastError());
		}
	}

	void print_message(DWORD recv_size)
	{
		int my_id = g_seesion_map[&over];
		std::cout << "Client[" << my_id << "] Sent : ";
		for (DWORD i = 0; i < recv_size; ++i)
			std::cout << buf[i];
		std::cout << std::endl;
	}

	void do_send(int recv_size)
	{
		wsabuf[0].len = recv_size;
		int res = WSASend(client_s, wsabuf, 1, nullptr, 0, &over, send_callback);

	}
};

std::unordered_map<int, SESSION> g_players;

GLvoid goLeft(move_packet& receivedPacket);
GLvoid goRight(move_packet &receivedPacket);
GLvoid goUp(move_packet& receivedPacket);
GLvoid goDown(move_packet& receivedPacket);

void print_error(const char* msg, int err_no)
{
	WCHAR *msg_buf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPWSTR>(& msg_buf), 0, NULL);

	std::cout << msg;
	std::wcout << L": 에러 : " << msg_buf;
	while (true);
	LocalFree(msg_buf);
}

GLvoid No();

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

	g_players[my_id].print_message(recv_size);
	g_players[my_id].do_send(recv_size);
}

int main(void)
{
	//No();
	OverlappedIO();
	//Non_blocking();
}

GLvoid OverlappedIO()
{
	std::wcout.imbue(std::locale("korean"));

	// window 네트워크 프로그래밍 시 옛날에 만든 프로그램과의 호환성을 위해 필요
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 0), &WSAData);

	// SOCKET 생성
	SOCKET server_s = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);

	// SOCK ADDR 생성
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
		g_players.try_emplace(id, client_s, id);
		g_players[id++].do_recv();
	}
	g_players.clear();
	closesocket(server_s);
	WSACleanup();
}

GLvoid No()
{
	std::wcout.imbue(std::locale("korean"));

	// window 네트워크 프로그래밍 시 옛날에 만든 프로그램과의 호환성을 위해 필요
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 0), &WSAData);

	// SOCKET 생성
	SOCKET server_s = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, 0);

	// SOCK ADDR 생성
	SOCKADDR_IN server_a;

	server_a.sin_family = AF_INET;
	server_a.sin_port = htons(PORT);
	server_a.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	bind(server_s, reinterpret_cast<sockaddr*>(&server_a), sizeof(server_a));
	listen(server_s, SOMAXCONN);
	int addr_size = sizeof(server_a);
	SOCKET client_s = WSAAccept(server_s, reinterpret_cast<sockaddr*>(&server_a), &addr_size, nullptr, 0);

	while (true) {
		char buf[BUFSIZE];

		WSABUF wsabuf[1];
		wsabuf[0].buf = buf;
		wsabuf[0].len = BUFSIZE;
		DWORD recv_size;
		DWORD recv_flag = 0;
		int res = WSARecv(client_s, wsabuf, 1, &recv_size, &recv_flag, nullptr, nullptr);
		if (0 != res) {
			print_error("WSARecv", WSAGetLastError());
		}
		if (0 == recv_size)
			break;
		std::cout << "Client Send : ";
		for (DWORD i = 0; i < recv_size; ++i)
			std::cout << buf[i];
		std::cout << std::endl;

		wsabuf[0].len = recv_size;
		DWORD sent_size;
		WSASend(client_s, wsabuf, 1, &sent_size, 0, nullptr, nullptr);
	}
	closesocket(server_s);
	closesocket(client_s);
	WSACleanup();
}

GLvoid Non_blocking()
{
	std::wcout.imbue(std::locale("korean"));

	// window 네트워크 프로그래밍 시 옛날에 만든 프로그램과의 호환성을 위해 필요
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 0), &WSAData);

	// SOCKET 생성
	SOCKET server_s = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, 0);
	if (server_s == INVALID_SOCKET)
	{
		std::cerr << "Failed to create server socket." << std::endl;
		WSACleanup();
		return;
	}

	std::cout << "[Server] 서버 클라이언트 접속 대기" << std::endl;

	// SOCK ADDR 생성
	SOCKADDR_IN server_a;

	server_a.sin_family = AF_INET;
	server_a.sin_port = htons(PORT);
	server_a.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	// 서버 소켓 바인딩
	if (bind(server_s, reinterpret_cast<sockaddr*>(&server_a), sizeof(server_a)) == SOCKET_ERROR)
	{
		std::cerr << "Failed to bind server socket." << std::endl;
		closesocket(server_s);
		WSACleanup();
		return;
	}

	// 연결 대기
	if (listen(server_s, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cerr << "Failed to listen on server socket.\n";
		closesocket(server_s);
		WSACleanup();
		return;
	}

	int addr_size = sizeof(server_a);

	// 클라이언트 소켓
	SOCKET client_s = WSAAccept(server_s, reinterpret_cast<sockaddr*>(&server_a), &addr_size, nullptr, 0);
	if (client_s == INVALID_SOCKET)
	{
		std::cerr << "Failed to create client socket." << std::endl;
		WSACleanup();
		return;
	}

	std::cout << "[Server] 서버 클라이언트 접속 성공" << std::endl;

	while (true) {
		char buf[BUFSIZE];

		// 데이터를 받을 move_packet 구조체 생성
		move_packet receivedPacket;

		// 데이터 받기
		WSABUF buffer;
		buffer.len = sizeof(move_packet);
		buffer.buf = reinterpret_cast<CHAR*>(&receivedPacket);
		DWORD bytesReceived;
		DWORD flags = 0;

		int res = WSARecv(client_s, &buffer, 1, &bytesReceived, &flags, nullptr, nullptr);
		if (0 != res) {
			print_error("WSARecv", WSAGetLastError());
		}
		if (0 == bytesReceived)
			break;

		DWORD sent_size;

		if (receivedPacket.type == '0') break;
		else if (receivedPacket.type == '1')	goLeft(receivedPacket);
		else if (receivedPacket.type == '2')	goRight(receivedPacket);
		else if (receivedPacket.type == '3')	goUp(receivedPacket);
		else if (receivedPacket.type == '4')	goDown(receivedPacket);

		WSASend(client_s, &buffer, 1, &sent_size, 0, nullptr, nullptr);
	}
	std::cout << "[Server] 서버 클라이언트 접속 종료!" << std::endl;
	closesocket(server_s);
	closesocket(client_s);
	WSACleanup();
}

GLvoid goLeft(move_packet& receivedPacket)
{
	std::cout << "[Server] 클라이언트 말 왼쪽 이동 요청!" << std::endl;

	if (receivedPacket.idx % 8 - 1 < 0) return;

	// 왼쪽 이동
	receivedPacket.idx = receivedPacket.idx - 1;
	receivedPacket.pos = Figure::Boards[receivedPacket.idx];
}

GLvoid goRight(move_packet& receivedPacket)
{
	std::cout << "[Server] 클라이언트 말 오른쪽 이동 요청!" << std::endl;

	if (receivedPacket.idx % 8 + 1 >= 8) return;

	// 오른쪽 이동
	receivedPacket.idx = receivedPacket.idx + 1;
	receivedPacket.pos = Figure::Boards[receivedPacket.idx];
}

GLvoid goUp(move_packet& receivedPacket)
{
	std::cout << "[Server] 클라이언트 말 위 이동 요청!" << std::endl;

	if (receivedPacket.idx - 8 < 0) return;

	// 위로 이동
	receivedPacket.idx = receivedPacket.idx - 8;
	receivedPacket.pos = Figure::Boards[receivedPacket.idx];
}

GLvoid goDown(move_packet& receivedPacket)
{
	std::cout << "[Server] 클라이언트 말 아래 이동 요청!" << std::endl;

	if (receivedPacket.idx + 8 >= Figure::Boards.size()) return;

	// 아래로 이동
	receivedPacket.idx = receivedPacket.idx + 8;
	receivedPacket.pos = Figure::Boards[receivedPacket.idx];
}