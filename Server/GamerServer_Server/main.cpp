#include <iostream>
#include <WS2tcpip.h>
#pragma comment (lib, "WS2_32.LIB")

constexpr short PORT = 4000;
constexpr int BUFSIZE = 256;

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