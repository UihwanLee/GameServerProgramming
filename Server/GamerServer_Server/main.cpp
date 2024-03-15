#include <iostream>
#include <WS2tcpip.h>
#pragma comment (lib, "WS2_32.LIB")

constexpr short PORT = 4000;
constexpr int BUFSIZE = 256;

#pragma pack (push, 1)
struct move_packet {
	short size;
	char  type;
	float x, y, z;
};
#pragma pack (pop)

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

int main(void)
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
		return 0;
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
		return 0;
	}

	// 연결 대기
	if (listen(server_s, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cerr << "Failed to listen on server socket.\n";
		closesocket(server_s);
		WSACleanup();
		return 0;
	}

	int addr_size = sizeof(server_a);

	// 클라이언트 소켓
	SOCKET client_s = WSAAccept(server_s, reinterpret_cast<sockaddr*>(&server_a), &addr_size, nullptr, 0);
	if (client_s == INVALID_SOCKET)
	{
		std::cerr << "Failed to create client socket." << std::endl;
		WSACleanup();
		return 0;
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


		std::cout << "Client Send : " << std::endl;
		std::cout << "Size: " << receivedPacket.size << std::endl;
		std::cout << "Type: " << receivedPacket.type << std::endl;

		DWORD sent_size;
		WSASend(client_s, &buffer, 1, &sent_size, 0, nullptr, nullptr);
	}
	std::cout << "[Server] 서버 클라이언트 접속 종료!" << std::endl;
	closesocket(server_s);
	closesocket(client_s);
	WSACleanup();
}