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
	std::wcout << L": ���� : " << msg_buf;
	while (true);
	LocalFree(msg_buf);
}

int main(void)
{
	std::wcout.imbue(std::locale("korean"));

	// window ��Ʈ��ũ ���α׷��� �� ������ ���� ���α׷����� ȣȯ���� ���� �ʿ�
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 0), &WSAData);

	// SOCKET ����
	SOCKET server_s = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, 0);
	if (server_s == INVALID_SOCKET)
	{
		std::cerr << "Failed to create server socket." << std::endl;
		WSACleanup();
		return 0;
	}

	std::cout << "[Server] ���� Ŭ���̾�Ʈ ���� ���" << std::endl;

	// SOCK ADDR ����
	SOCKADDR_IN server_a;

	server_a.sin_family = AF_INET;
	server_a.sin_port = htons(PORT);
	server_a.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	// ���� ���� ���ε�
	if (bind(server_s, reinterpret_cast<sockaddr*>(&server_a), sizeof(server_a)) == SOCKET_ERROR)
	{
		std::cerr << "Failed to bind server socket." << std::endl;
		closesocket(server_s);
		WSACleanup();
		return 0;
	}

	// ���� ���
	if (listen(server_s, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cerr << "Failed to listen on server socket.\n";
		closesocket(server_s);
		WSACleanup();
		return 0;
	}

	int addr_size = sizeof(server_a);

	// Ŭ���̾�Ʈ ����
	SOCKET client_s = WSAAccept(server_s, reinterpret_cast<sockaddr*>(&server_a), &addr_size, nullptr, 0);
	if (client_s == INVALID_SOCKET)
	{
		std::cerr << "Failed to create client socket." << std::endl;
		WSACleanup();
		return 0;
	}

	std::cout << "[Server] ���� Ŭ���̾�Ʈ ���� ����" << std::endl;

	while (true) {
		char buf[BUFSIZE];

		// �����͸� ���� move_packet ����ü ����
		move_packet receivedPacket;

		// ������ �ޱ�
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
	std::cout << "[Server] ���� Ŭ���̾�Ʈ ���� ����!" << std::endl;
	closesocket(server_s);
	closesocket(client_s);
	WSACleanup();
}