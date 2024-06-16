#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <unordered_map>
#include <Windows.h>
#include <chrono>
#include <random>
#include <thread>

using namespace std;

#include "..\..\Server\GamerServer_Server\protocol.h"

sf::TcpSocket s_socket;

constexpr auto SCREEN_WIDTH = 16;
constexpr auto SCREEN_HEIGHT = 16;

constexpr auto TOTAL_MAP_WIDTH = 2000;
constexpr auto TOTAL_MAP_HEIGHT = 2000;

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

float volume = 100.0f;

bool chatMode = false;
std::string m_chat;

int tile_map[TOTAL_MAP_WIDTH][TOTAL_MAP_HEIGHT] = { 0, };

void play_bgm(const std::string& filename)
{
	std::thread([filename]()
		{
			sf::SoundBuffer buffer;

			if (!buffer.loadFromFile(filename))
			{
				std::cout << "load 에러" << std::endl;
				return;
			}

			// 재생
			sf::Sound sound(buffer);
			sound.play();

			// 무한 루프
			while (sound.getStatus() == sf::Sound::Playing)
			{
				sf::sleep(sf::milliseconds(100));
			}
		}).detach();
}

void play_sound_effect(const std::string& filename)
{
	std::thread([filename]()
		{
			sf::SoundBuffer buffer;

			if (!buffer.loadFromFile(filename))
			{
				std::cout << "load 에러" << std::endl;
				return;
			}

			// 재생
			sf::Sound sound(buffer);
			sound.setVolume(volume);
			sound.play();

			// 소리가 끝날 때까지 대기
			sf::sleep(sf::seconds(buffer.getDuration().asSeconds()));
		}).detach();
}

class OBJECT {
private:
	bool m_showing;
	sf::Sprite m_sprite;

	sf::Text m_name;
	sf::Text m_chat;
	sf::Text t_hp;
	sf::Text t_level;
	sf::Text t_damge;
	sf::Text m_log;
	chrono::system_clock::time_point m_mess_end_time;
	chrono::system_clock::time_point m_damge_end_time;
	chrono::system_clock::time_point m_log_end_time;
public:
	bool is_pc;
	int id;
	int m_x, m_y;
	char name[NAME_SIZE];
	int m_hp;
	int m_level;
	int m_exp;
	int m_max_exp;
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

		t_damge.setFont(g_font);
		char buf[100];
		sprintf_s(buf, "-%d", atk);
		t_damge.setString(buf);
		t_damge.setFillColor(sf::Color(255, 0, 0));
		t_damge.setStyle(sf::Text::Bold);
		m_damge_end_time = chrono::system_clock::now() + chrono::seconds(1);
	}
	
	void set_log(const char str[])
	{
		m_log.setFont(g_font);
		m_log.setString(str);
		m_log.setFillColor(sf::Color(255, 255, 255));
		m_log.setStyle(sf::Text::Bold);
		m_log_end_time = chrono::system_clock::now() + chrono::seconds(2);
	}
};

bool is_pc(int object_id)
{
	return object_id < MAX_USER;
}

OBJECT avatar;
unordered_map <int, OBJECT> objects;

sf::Texture* chat;
sf::Sprite m_chat_sprite;

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

	if (m_damge_end_time >= chrono::system_clock::now())
	{
		t_damge.setPosition(rx + 70 - size.width / 2, ry + 5);
		g_window->draw(t_damge);
	}

	if (m_log_end_time >= chrono::system_clock::now())
	{
		m_log.setPosition(200.0f, 900.0f);
		g_window->draw(m_log);
	}

	if (chatMode)
	{
		sf::Text text;
		text.setFont(g_font);
		text.setFillColor(sf::Color(255, 255, 0));
		char buf[100];
		sprintf_s(buf, "ChatMode");
		text.setPosition(200, 650);
		text.setString(buf);
		g_window->draw(text);
		m_chat_sprite.setTexture(*chat);
		m_chat_sprite.setPosition(200, 700);
		g_window->draw(m_chat_sprite);
	}
}

OBJECT tile_grass;
OBJECT tile_wall;

sf::Sprite m_ui_info;
sf::Texture* board;
sf::Texture* pieces;
sf::Texture* info;

void client_initialize()
{
	// 랜덤 시드 설정
	std::random_device rd;  // 시드 생성기
	std::mt19937 gen(rd()); // 메르센 트위스터 난수 생성기
	std::uniform_int_distribution<> dis(10, 20); // 10에서 20 사이의 균등 분포

	board = new sf::Texture;
	pieces = new sf::Texture;
	chat = new sf::Texture;
	info = new sf::Texture;
	board->loadFromFile("fieldmap.bmp");
	pieces->loadFromFile("character.png");
	chat->loadFromFile("chat.png");
	info->loadFromFile("ui_info.png");
	if (false == g_font.loadFromFile("Maplestory Bold.ttf")) {
		cout << "Font Loading Error!\n";
		exit(-1);
	}
	tile_grass = OBJECT{ *board, 5, 5, TILE_WIDTH, TILE_WIDTH };
	tile_wall = OBJECT{ *board, 69, 5, TILE_WIDTH, TILE_WIDTH };
	avatar = OBJECT{ *pieces, 0, 0, 64, 64 };

	CENTRAL_TILE_SPACING_WIDTH = dis(gen);
	CENTRAL_TILE_SPACING_HEIGHT = dis(gen);

	avatar.move(4, 4);

	// tile_map 초기화
	for (int i = 0; i < TOTAL_MAP_WIDTH; ++i) {
		for (int j = 0; j < TOTAL_MAP_HEIGHT; ++j) {
			if (i < TILE_BORDER || i >= TOTAL_MAP_WIDTH - TILE_BORDER ||
				j < TILE_BORDER || j >= TOTAL_MAP_HEIGHT - TILE_BORDER) {
				tile_map[i][j] = 1;
			}
			else {
				tile_map[i][j] = 0;
			}
		}
	}
}

void client_finish()
{
	objects.clear();
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
		avatar.set_name(packet->name);
		g_myid = packet->id;
		avatar.id = g_myid;
		avatar.m_hp = packet->hp;
		avatar.m_level = packet->level;
		avatar.m_exp = packet->exp;
		avatar.m_max_exp = packet->max_exp;
		avatar.is_pc = is_pc(packet->id);
		avatar.move(packet->x, packet->y);
		g_left_x = packet->x - SCREEN_WIDTH / 2;
		g_top_y = packet->y - SCREEN_HEIGHT / 2;
		avatar.show();
		play_bgm("bgm.wav");
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
			objects[id] = OBJECT{ *pieces, 0, 0, 64, 64 };
			objects[id].id = id;
			objects[id].is_pc = true;
			objects[id].move(my_packet->x, my_packet->y);
			objects[id].m_hp = my_packet->hp;
			objects[id].m_level = my_packet->level;
			objects[id].set_name(my_packet->name);
			objects[id].show();
		}
		else {
			objects[id] = OBJECT{ *pieces, 256, 0, 64, 64 };
			objects[id].id = id;
			objects[id].m_hp = 100;
			objects[id].is_pc = false;
			objects[id].move(my_packet->x, my_packet->y);
			objects[id].set_name(my_packet->name);
			objects[id].show();
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
			objects[other_id].move(my_packet->x, my_packet->y);
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
			objects.erase(other_id);
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
			objects[other_id].set_chat(my_packet->mess);
		}

		break;
	}
	case SC_ATTACK:
	{
		SC_ATTACK_OBJECT_PACKET* my_packet = reinterpret_cast<SC_ATTACK_OBJECT_PACKET*>(ptr);
		int npc = my_packet->npc;

		int other_id = my_packet->id;
		if (other_id == g_myid) {
			// 효과음 재생
			volume = 50.0f;
			play_sound_effect("SE_Slash.wav");

			objects[npc].set_damage(my_packet->atk);
			avatar.set_log("The hero hit the monster and inflicted 10 damage.");
		}
		else
		{
			objects[npc].set_damage(my_packet->atk);
		}
		break;
	}
	case SC_MONSTER_DEAD:
	{
		SC_DEAD_MONSTER_PACKET* my_packet = reinterpret_cast<SC_DEAD_MONSTER_PACKET*>(ptr);
		int npc = my_packet->npc;

		int other_id = my_packet->id;
		if (other_id == g_myid) {
			// 효과음 재생
			volume = 100.0f;
			play_sound_effect("SE_MonsterDead.wav");

			// 레벨업 했는지 체크
			if (avatar.m_level < my_packet->level)
			{
				play_sound_effect("SE_LevelUp.wav");
			}
			avatar.m_level = my_packet->level;
			avatar.m_exp = my_packet->exp;
			avatar.m_max_exp = my_packet->max_exp;
			avatar.set_log("Obtained the 20th experience by hunting monsters.");
		}
		else
		{
			objects[other_id].m_level = my_packet->level;
			objects[other_id].m_exp = my_packet->exp;
			objects[other_id].m_max_exp = my_packet->max_exp;
		}

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

	for (int i = 0; i < SCREEN_WIDTH; ++i)
		for (int j = 0; j < SCREEN_HEIGHT; ++j)
		{
			int tile_x = i + g_left_x;
			int tile_y = j + g_top_y;
			if ((tile_x < 0) || (tile_y < 0)) continue;

			// 가장자리 조건을 타일의 절대 좌표로 결정
			bool is_edge = tile_x < TILE_BORDER || tile_x >= TOTAL_MAP_WIDTH - TILE_BORDER ||
				tile_y < TILE_BORDER || tile_y >= TOTAL_MAP_HEIGHT - TILE_BORDER;

			if (is_edge) {
				tile_map[i][j] = 0;
				tile_wall.a_move(TILE_WIDTH * i, TILE_WIDTH * j);
				tile_wall.a_draw();
			}
			else
			{
				tile_map[i][j] = 1;
				tile_grass.a_move(TILE_WIDTH * i, TILE_WIDTH * j);
				tile_grass.a_draw();
			}
		}
	avatar.draw();

	for (auto& pl : objects) pl.second.draw();

	m_ui_info.setTexture(*info);
	g_window->draw(m_ui_info);

	sf::Text text;
	text.setFont(g_font);
	text.setFillColor(sf::Color(255, 255, 255));
	char buf[100];

	sprintf_s(buf, "player exp: (%d/%d)", avatar.m_exp, avatar.m_max_exp);
	text.setPosition(10.0f, 5.0f);
	text.setString(buf);
	g_window->draw(text);

	sprintf_s(buf, "player Pos: (%d, %d)", avatar.m_x, avatar.m_y);
	text.setString(buf);
	text.setPosition(10.0f, 30.0f);
	g_window->draw(text);

	sprintf_s(buf, "operation keys", avatar.m_x, avatar.m_y);
	text.setString(buf);
	text.setPosition(10.0f, 70.0f);
	g_window->draw(text);

	sprintf_s(buf, "move: w, a, s, d", avatar.m_x, avatar.m_y);
	text.setString(buf);
	text.setPosition(10.0f, 120.0f);
	g_window->draw(text);

	sprintf_s(buf, "attack: a", avatar.m_x, avatar.m_y);
	text.setString(buf);
	text.setPosition(10.0f, 150.0f);
	g_window->draw(text);

	sprintf_s(buf, "chat: c", avatar.m_x, avatar.m_y);
	text.setString(buf);
	text.setPosition(10.0f, 180.0f);
	g_window->draw(text);

	text.setFillColor(sf::Color(255, 255, 255));
	text.setString(m_chat);
	text.setPosition(210.0f, 700.0f);
	g_window->draw(text);

	/*sprintf_s(buf, "player exp: (%d, %d)", avatar.m_x, avatar.m_y);
	text.setString(buf);
	text.setPosition(0.0f, 50.0f);
	g_window->draw(text);*/

}

void send_packet(void* packet)
{
	unsigned char* p = reinterpret_cast<unsigned char*>(packet);
	size_t sent = 0;
	s_socket.send(packet, p[0], sent);
}

bool check_collision(int offset_x, int offset_y)
{
	int r_x = avatar.m_x + offset_x;
	int r_y = avatar.m_y + offset_y;

	if (r_x < 0 || r_x >= TOTAL_MAP_WIDTH) return false;
	if (r_y < 0 || r_y >= TOTAL_MAP_HEIGHT) return false;

	if (tile_map[r_x][r_y] == 1) return false;

	return true;
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

	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "LEGEND DUNGUN");
	g_window = &window;

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
			if (event.type == sf::Event::KeyPressed) {
				if (chatMode) {
					if (event.key.code == sf::Keyboard::Enter)
					{
						// chat 보내기
						CS_CHAT_PACKET p;
						p.size = sizeof(p);
						p.type = CS_CHAT;
						strncpy_s(p.mess, m_chat.c_str(), sizeof(p.mess) - 1);
						p.mess[sizeof(p.mess) - 1] = '\0';
						send_packet(&p);

						// chat 초기화
						m_chat.clear();
						chatMode = false;
					}
					else if (event.key.code == sf::Keyboard::Escape || event.key.code == sf::Keyboard::C)
					{
						m_chat.clear();
						chatMode = false;
					}
					else if (event.key.code == sf::Keyboard::Space)
					{
						m_chat += " ";
					}
					else
					{
						// 키보드 입력을 받아 m_chat에 저장하기
						if (event.key.code == sf::Keyboard::BackSpace) {
							if (!m_chat.empty()) {
								m_chat.pop_back();
							}
						}
						else {
							// SFML에서 직접 한글 입력을 처리하려면 추가 코드가 필요할 수 있음
							const sf::Keyboard::Key keycode = event.key.code;
							if (keycode >= sf::Keyboard::A && keycode <= sf::Keyboard::Z) {
								char chr = static_cast<char>(keycode - sf::Keyboard::A + 'a');
								m_chat += chr;
							}
						}
					}
				}
				else
				{
					int direction = -1;
					bool can_move = false;
					switch (event.key.code) {
					case sf::Keyboard::Left:
					{
						direction = 2;
						can_move = check_collision(-1, 0);
						break;
					}
					case sf::Keyboard::Right:
					{
						direction = 3;
						can_move = check_collision(1, 0);
						break;
					}
					case sf::Keyboard::Up:
					{
						direction = 0;
						can_move = check_collision(0, -1);
						break;
					}
					case sf::Keyboard::Down:
					{
						direction = 1;
						can_move = check_collision(0, 1);
						break;
					}
					case sf::Keyboard::A:
						direction = -1;
						CS_ATTACK_PACKET p;
						p.size = sizeof(p);
						p.type = CS_ATTACK;
						send_packet(&p);
						break;
					case sf::Keyboard::C:
						chatMode = true;
						break;
					case sf::Keyboard::Escape:
						window.close();
						break;
					}
					if (-1 != direction && can_move) {
						CS_MOVE_PACKET p;
						p.size = sizeof(p);
						p.type = CS_MOVE;
						p.direction = direction;
						send_packet(&p);
					}
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