#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <unordered_map>
#include <Windows.h>
#include <chrono>
#include <random>
using namespace std;

#include "..\..\Server\GamerServer_Server\protocol.h"

sf::TcpSocket s_socket;

constexpr auto SCREEN_WIDTH = 16;
constexpr auto SCREEN_HEIGHT = 16;

constexpr auto TOTAL_MAP_WIDTH = 2000;
constexpr auto TOTAL_MAP_HEIGHT = 2000;

constexpr auto TILE_BORDER = 3;

int CENTRAL_TILE_SPACING_WIDTH = 5;
int CENTRAL_TILE_SPACING_HEIGHT = 5;

constexpr auto TILE_WIDTH = 65;
constexpr auto WINDOW_WIDTH = SCREEN_WIDTH * TILE_WIDTH;   // size of window
constexpr auto WINDOW_HEIGHT = SCREEN_WIDTH * TILE_WIDTH;

int g_left_x;
int g_top_y;
int g_myid;

sf::RenderWindow* g_window;
sf::Font g_font;

class OBJECT {
private:
	bool m_showing;
	sf::Sprite m_sprite;

	sf::Text m_name;
	sf::Text m_chat;
	sf::Text t_hp;
	sf::Text t_level;
	chrono::system_clock::time_point m_mess_end_time;
public:
	bool is_pc;
	int id;
	int m_x, m_y;
	char name[NAME_SIZE];
	int m_hp;
	int m_level;
	OBJECT(sf::Texture& t, int x, int y, int x2, int y2) {
		m_showing = false;
		m_sprite.setTexture(t);
		m_sprite.setTextureRect(sf::IntRect(x, y, x2, y2));
		set_name("NONAME");
		m_mess_end_time = chrono::system_clock::now();
	}
	OBJECT() {
		m_showing = false;
	}
	void show()
	{
		m_showing = true;
	}
	void hide()
	{
		m_showing = false;
	}

	void a_move(int x, int y) {
		m_sprite.setPosition((float)x, (float)y);
	}

	void a_draw() {
		g_window->draw(m_sprite);
	}

	void move(int x, int y) {
		m_x = x;
		m_y = y;
	}
	void draw();
	void set_name(const char str[]) {
		m_name.setFont(g_font);
		m_name.setString(str);
		if (id < MAX_USER) m_name.setFillColor(sf::Color(255, 255, 255));
		else m_name.setFillColor(sf::Color(255, 255, 0));
		m_name.setStyle(sf::Text::Bold);
	}

	void set_hp(int hp) {
		t_hp.setFont(g_font);
		t_hp.setFillColor(sf::Color(255, 0, 0));
		char buf[100];
		sprintf_s(buf, "HP:%d", hp);
		t_hp.setString(buf);
	}

	void set_level(int level) {
		t_level.setFont(g_font);
		t_level.setFillColor(sf::Color(255, 255, 0));
		char buf[100];
		sprintf_s(buf, "Level:%d", level);
		t_level.setString(buf);
	}

	void set_chat(const char str[]) {
		m_chat.setFont(g_font);
		m_chat.setString(str);
		m_chat.setFillColor(sf::Color(255, 255, 255));
		m_chat.setStyle(sf::Text::Bold);
		m_mess_end_time = chrono::system_clock::now() + chrono::seconds(3);
	}
	void set_damage(int atk)
	{
		m_hp -= atk;
	}
};

bool is_pc(int object_id)
{
	return object_id < MAX_USER;
}

OBJECT avatar;
unordered_map <int, OBJECT> players;

void OBJECT::draw()
{
	if (false == m_showing) return;
	float rx = (m_x - g_left_x) * 65.0f + 1;
	float ry = (m_y - g_top_y) * 65.0f + 1;
	m_sprite.setPosition(rx, ry);
	g_window->draw(m_sprite);
	auto size = m_name.getGlobalBounds();
	if (m_mess_end_time < chrono::system_clock::now()) {
		m_name.setPosition(rx + 32 - size.width / 2, ry - 10);
		g_window->draw(m_name);

		set_hp(m_hp);
		t_hp.setPosition(rx + 32 - size.width / 2, ry - 30);
		g_window->draw(t_hp);

		// player만 level 표시
		if (is_pc)
		{
			set_level(avatar.m_level);
			t_level.setPosition(rx + 32 - size.width / 2, ry - 50);
			g_window->draw(t_level);
		}
	}
	else {
		m_chat.setPosition(rx + 32 - size.width / 2, ry - 10);
		g_window->draw(m_chat);
	}
}

OBJECT white_tile;
OBJECT black_tile;

sf::Texture* board;
sf::Texture* pieces;

void client_initialize()
{
	// 랜덤 시드 설정
	std::random_device rd;  // 시드 생성기
	std::mt19937 gen(rd()); // 메르센 트위스터 난수 생성기
	std::uniform_int_distribution<> dis(10, 20); // 10에서 20 사이의 균등 분포

	board = new sf::Texture;
	pieces = new sf::Texture;
	board->loadFromFile("fieldmap.bmp");
	pieces->loadFromFile("character.png");
	if (false == g_font.loadFromFile("cour.ttf")) {
		cout << "Font Loading Error!\n";
		exit(-1);
	}
	white_tile = OBJECT{ *board, 5, 5, TILE_WIDTH, TILE_WIDTH };
	black_tile = OBJECT{ *board, 69, 5, TILE_WIDTH, TILE_WIDTH };
	avatar = OBJECT{ *pieces, 0, 0, 64, 64 };

	CENTRAL_TILE_SPACING_WIDTH = dis(gen);
	CENTRAL_TILE_SPACING_HEIGHT = dis(gen);

	avatar.move(4, 4);
}

void client_finish()
{
	players.clear();
	delete board;
	delete pieces;
}

void ProcessPacket(char* ptr)
{
	static bool first_time = true;
	switch (ptr[1])
	{
	case SC_LOGIN_FAIL:
	{
		SC_LOGIN_FAIL_PACKET* packet = reinterpret_cast<SC_LOGIN_FAIL_PACKET*>(ptr);

		std::cout << "[로그인 실패] 입력한 id가 유효하지 않습니다.\n";
		g_window->close();
	}
	break;

	case SC_LOGIN_INFO:
	{
		std::cout << "[로그인 성공] 로그인 하였습니다.\n";
		SC_LOGIN_INFO_PACKET* packet = reinterpret_cast<SC_LOGIN_INFO_PACKET*>(ptr);
		std::cout << "name: " << packet->name << std::endl;
		avatar.set_name(packet->name);
		g_myid = packet->id;
		avatar.id = g_myid;
		avatar.m_hp = packet->hp;
		avatar.m_level = packet->level;
		avatar.is_pc = is_pc(packet->id);
		avatar.move(packet->x, packet->y);
		g_left_x = packet->x - SCREEN_WIDTH / 2;
		g_top_y = packet->y - SCREEN_HEIGHT / 2;
		avatar.show();
	}
	break;

	case SC_ADD_OBJECT:
	{
		SC_ADD_OBJECT_PACKET* my_packet = reinterpret_cast<SC_ADD_OBJECT_PACKET*>(ptr);
		int id = my_packet->id;

		if (id == g_myid) {
			avatar.move(my_packet->x, my_packet->y);
			g_left_x = my_packet->x - SCREEN_WIDTH / 2;
			g_top_y = my_packet->y - SCREEN_HEIGHT / 2;
			avatar.show();
		}
		else if (id < MAX_USER) {
			players[id] = OBJECT{ *pieces, 0, 0, 64, 64 };
			players[id].id = id;
			players[id].is_pc = true;
			players[id].move(my_packet->x, my_packet->y);
			players[id].set_name(my_packet->name);
			players[id].show();
		}
		else {
			players[id] = OBJECT{ *pieces, 256, 0, 64, 64 };
			players[id].id = id;
			players[id].m_hp = 100;
			players[id].is_pc = false;
			players[id].move(my_packet->x, my_packet->y);
			players[id].set_name(my_packet->name);
			players[id].show();
		}
		break;
	}
	case SC_MOVE_OBJECT:
	{
		SC_MOVE_OBJECT_PACKET* my_packet = reinterpret_cast<SC_MOVE_OBJECT_PACKET*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.move(my_packet->x, my_packet->y);
			g_left_x = my_packet->x - SCREEN_WIDTH / 2;
			g_top_y = my_packet->y - SCREEN_HEIGHT / 2;
		}
		else {
			players[other_id].move(my_packet->x, my_packet->y);
		}
		break;
	}

	case SC_REMOVE_OBJECT:
	{
		SC_REMOVE_OBJECT_PACKET* my_packet = reinterpret_cast<SC_REMOVE_OBJECT_PACKET*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.hide();
		}
		else {
			players.erase(other_id);
		}
		break;
	}
	case SC_CHAT:
	{
		SC_CHAT_PACKET* my_packet = reinterpret_cast<SC_CHAT_PACKET*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			avatar.set_chat(my_packet->mess);
		}
		else {
			players[other_id].set_chat(my_packet->mess);
		}

		break;
	}
	case SC_ATTACK:
	{
		SC_ATTACK_OBJECT_PACKET* my_packet = reinterpret_cast<SC_ATTACK_OBJECT_PACKET*>(ptr);
		int npc = my_packet->npc;
		
		players[npc].m_hp -= my_packet->atk;
		break;
	}
	default:
		printf("Unknown PACKET type [%d]\n", ptr[1]);
	}
}

void process_data(char* net_buf, size_t io_byte)
{
	char* ptr = net_buf;
	static size_t in_packet_size = 0;
	static size_t saved_packet_size = 0;
	static char packet_buffer[BUF_SIZE];

	while (0 != io_byte) {
		if (0 == in_packet_size) in_packet_size = ptr[0];
		if (io_byte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			io_byte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
			saved_packet_size += io_byte;
			io_byte = 0;
		}
	}
}

void client_main()
{
	char net_buf[BUF_SIZE];
	size_t	received;

	auto recv_result = s_socket.receive(net_buf, BUF_SIZE, received);
	if (recv_result == sf::Socket::Error)
	{
		wcout << L"Recv 에러!";
		exit(-1);
	}
	if (recv_result == sf::Socket::Disconnected) {
		wcout << L"Disconnected\n";
		exit(-1);
	}
	if (recv_result != sf::Socket::NotReady)
		if (received > 0) process_data(net_buf, received);

	/*for (int i = 0; i < SCREEN_WIDTH; ++i)
		for (int j = 0; j < SCREEN_HEIGHT; ++j)
		{
			int tile_x = i + g_left_x;
			int tile_y = j + g_top_y;
			if ((tile_x < 0) || (tile_y < 0)) continue;
			if (0 == (tile_x / 3 + tile_y / 3) % 2) {
				white_tile.a_move(TILE_WIDTH * i, TILE_WIDTH * j);
				white_tile.a_draw();
			}
			else
			{
				black_tile.a_move(TILE_WIDTH * i, TILE_WIDTH * j);
				black_tile.a_draw();
			}
		}*/

	for (int i = 0; i < SCREEN_WIDTH; ++i)
		for (int j = 0; j < SCREEN_HEIGHT; ++j)
		{
			int tile_x = i + g_left_x;
			int tile_y = j + g_top_y;
			if ((tile_x < 0) || (tile_y < 0)) continue;

			// 가장자리 조건을 타일의 절대 좌표로 결정
			bool is_edge = tile_x < TILE_BORDER || tile_x >= TOTAL_MAP_WIDTH - TILE_BORDER ||
				tile_y < TILE_BORDER || tile_y >= TOTAL_MAP_HEIGHT - TILE_BORDER;

			// 중앙에 간격을 두고 검은 타일을 배치하는 조건
			bool is_central_black_tile = (tile_x % CENTRAL_TILE_SPACING_WIDTH == 0) && (tile_y % CENTRAL_TILE_SPACING_HEIGHT == 0);

			if (is_edge || is_central_black_tile) {
				black_tile.a_move(TILE_WIDTH * i, TILE_WIDTH * j);
				black_tile.a_draw();
			}
			else
			{
				white_tile.a_move(TILE_WIDTH * i, TILE_WIDTH * j);
				white_tile.a_draw();
			}
		}
	avatar.draw();
	for (auto& pl : players) pl.second.draw();
	sf::Text text;
	text.setFont(g_font);
	text.setFillColor(sf::Color(0, 0, 0));
	char buf[100];

	sprintf_s(buf, "player HP: %d", avatar.m_hp);
	text.setString(buf);
	g_window->draw(text);

	sprintf_s(buf, "player Level: %d", avatar.m_level);
	text.setString(buf);
	text.setPosition(0.0f, 25.0f);
	g_window->draw(text);

	sprintf_s(buf, "player pos: (%d, %d)", avatar.m_x, avatar.m_y);
	text.setString(buf);
	text.setPosition(0.0f, 50.0f);
	g_window->draw(text);

}

void send_packet(void* packet)
{
	unsigned char* p = reinterpret_cast<unsigned char*>(packet);
	size_t sent = 0;
	s_socket.send(packet, p[0], sent);
}

int main()
{
	wcout.imbue(locale("korean"));
	char SERVER_ADDR[10] = "127.0.0.1";
	/*std::cout << "서버 주소: ";
	std::cin.getline(SERVER_ADDR, 10);*/

	sf::Socket::Status status = s_socket.connect(SERVER_ADDR, PORT_NUM);
	s_socket.setBlocking(false);

	if (status != sf::Socket::Done) {
		wcout << L"서버와 연결할 수 없습니다.\n";
		exit(-1);
	}

	char DB_ID[18];
	int id;

	std::cout << "[로그인]\n";
	std::cout << "접속할 id를 입력해주세요: ";
	std::cin.getline(DB_ID, 18);

	try {
		id = std::stoi(DB_ID);
	}
	catch (const std::invalid_argument& e) {
		std::cerr << "[ERROR] id를 잘못 입력하였습니다.\n";
		return 0;
	}
	catch (const std::out_of_range& e) {
		std::cerr << "[ERROR] id를 잘못 입력하였습니다.\n";
		return 0;
	}

	client_initialize();
	CS_LOGIN_PACKET p;
	p.size = sizeof(p);
	p.id = id;
	p.type = CS_LOGIN;

	string player_name{ "P" };
	player_name += to_string(GetCurrentProcessId());

	strcpy_s(p.name, player_name.c_str());
	send_packet(&p);
	avatar.set_name(p.name);

	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "2D CLIENT");
	g_window = &window;

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::KeyPressed) {
				int direction = -1;
				switch (event.key.code) {
				case sf::Keyboard::Left:
					direction = 2;
					break;
				case sf::Keyboard::Right:
					direction = 3;
					break;
				case sf::Keyboard::Up:
					direction = 0;
					break;
				case sf::Keyboard::Down:
					direction = 1;
					break;
				case sf::Keyboard::A:
					direction = -1;
					CS_ATTACK_PACKET p;
					p.size = sizeof(p);
					p.type = CS_ATTACK;
					send_packet(&p);
					break;
				case sf::Keyboard::Escape:
					window.close();
					break;
				}
				if (-1 != direction) {
					CS_MOVE_PACKET p;
					p.size = sizeof(p);
					p.type = CS_MOVE;
					p.direction = direction;
					send_packet(&p);
				}

			}
		}

		window.clear();
		client_main();
		window.display();
	}
	client_finish();

	return 0;
}