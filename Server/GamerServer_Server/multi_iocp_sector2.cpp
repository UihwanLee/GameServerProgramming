#include <iostream>
#include <array>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <vector>
#include <mutex>
#include <unordered_set>
#include "protocol.h"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
using namespace std;

enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND };

constexpr int VIEW_RANGE = 5;		// ���� Ŭ���̾�Ʈ �þߺ��� �ణ �۰�

class OVER_EXP {
public:
	WSAOVERLAPPED _over;
	WSABUF _wsabuf;
	char _send_buf[BUF_SIZE];
	COMP_TYPE _comp_type;
	OVER_EXP()
	{
		_wsabuf.len = BUF_SIZE;
		_wsabuf.buf = _send_buf;
		_comp_type = OP_RECV;
		ZeroMemory(&_over, sizeof(_over));
	}
	OVER_EXP(char* packet)
	{
		_wsabuf.len = packet[0];
		_wsabuf.buf = _send_buf;
		ZeroMemory(&_over, sizeof(_over));
		_comp_type = OP_SEND;
		memcpy(_send_buf, packet, packet[0]);
	}
};

enum S_STATE { ST_FREE, ST_ALLOC, ST_INGAME };
class SESSION {
	OVER_EXP _recv_over;

public:
	mutex _s_lock;
	S_STATE _state;
	int _id;
	SOCKET _socket;
	float	x, y;
	float	cx, cy;
	char	_name[NAME_SIZE];
	int		sector_x;
	int		sector_y;
	unordered_set<int> view_list;
	mutex	_vl_l;

	int		_prev_remain;
	int		_last_move_time;
public:
	SESSION()
	{
		_id = -1;
		_socket = 0;
		x = y = cx = cy = 0.0f;
		_name[0] = 0;
		_state = ST_FREE;
		_prev_remain = 0;
	}

	~SESSION() {}

	void do_recv()
	{
		DWORD recv_flag = 0;
		memset(&_recv_over._over, 0, sizeof(_recv_over._over));
		_recv_over._wsabuf.len = BUF_SIZE - _prev_remain;
		_recv_over._wsabuf.buf = _recv_over._send_buf + _prev_remain;
		WSARecv(_socket, &_recv_over._wsabuf, 1, 0, &recv_flag,
			&_recv_over._over, 0);
	}

	void do_send(void* packet)
	{
		OVER_EXP* sdata = new OVER_EXP{ reinterpret_cast<char*>(packet) };
		WSASend(_socket, &sdata->_wsabuf, 1, 0, 0, &sdata->_over, 0);
	}
	void send_login_info_packet()
	{
		SC_LOGIN_INFO_PACKET p;
		p.id = _id;
		p.size = sizeof(SC_LOGIN_INFO_PACKET);
		p.type = SC_LOGIN_INFO;
		p.x = x;
		p.y = y;
		p.cx = cx;
		p.cy = cy;
		do_send(&p);
	}
	void send_move_packet(int c_id);
	void send_add_player_packet(int c_id);
	void send_remove_player_packet(int c_id)
	{
		_vl_l.lock();
		view_list.erase(c_id);
		_vl_l.unlock();
		SC_REMOVE_PLAYER_PACKET p;
		p.id = c_id;
		p.size = sizeof(p);
		p.type = SC_REMOVE_PLAYER;
		do_send(&p);
	}
};

array<SESSION, MAX_USER> clients;

SOCKET g_s_socket, g_c_socket;
OVER_EXP g_a_over;

const int MAX_ROW = 10; // �ִ� SESSION ��
const int MAX_COL = 10;  // �ִ� USER ��

mutex g_sector;
unordered_set<int> g_ObjectListSector[MAX_ROW][MAX_COL];

void get_best_sector(int* w, int* h)
{
	// sector �� client�� ���� ���� sector w, h ��ȯ
	int min_client = MAX_USER;
	for (int i = 0; i < MAX_ROW; i++)
	{
		for (int j = 0; j < MAX_COL; j++)
		{
			if (g_ObjectListSector[i][j].size() < min_client)
			{
				std::cout << g_ObjectListSector[i][j].size() << std::endl;
				min_client = g_ObjectListSector[i][j].size();
				*w = i * 10;
				*h = j * 10;
			}
		}
	}
}

unordered_set<int>& find_sector(int row, int col) {
	// sector �� �����ϴ� client ����Ʈ ��ȯ
	if (row < 0) row = 0;
	if (row >= MAX_ROW) row = MAX_ROW - 1;
	if (col < 0) col = 0;
	if (col >= MAX_COL) col = MAX_COL - 1;

	return g_ObjectListSector[row][col];
}

void init_sector(int id, int x, int y)
{
	// �ʱ�ȭ �� position�� ���� g_ObjectListSector ������Ʈ
	int idx_x = x / MAX_ROW;
	int idx_y = y / MAX_COL;

	if (idx_x < 0) idx_x = 0;
	if (idx_x >= MAX_ROW) idx_x = MAX_ROW - 1;

	if (idx_y < 0) idx_y = 0;
	if (idx_y >= MAX_COL) idx_y = MAX_COL - 1;

	clients[id].sector_x = idx_x;
	clients[id].sector_y = idx_y;

	g_ObjectListSector[idx_x][idx_y].insert(id);
}

void update_sector(int id, int x, int y)
{
	// �̵� �� position�� ���� g_ObjectListSector ������Ʈ
	int idx_x = x / MAX_ROW;
	int idx_y = y / MAX_COL;

	if (idx_x < 0) idx_x = 0;
	if (idx_x >= MAX_ROW) idx_x = MAX_ROW - 1;

	if (idx_y < 0) idx_y = 0;
	if (idx_y >= MAX_COL) idx_y = MAX_COL - 1;

	// sector ��ȯ�� ���ٸ� �״�� �ΰ� �ִٸ� ����
	// ������ �ȴٸ� ���� sector���� �ڱ� ������ ���� / ���ο� sector�� �ڱ� �ڽ� ������Ʈ
	if (idx_x != clients[id].sector_x)
	{
		g_ObjectListSector[clients[id].sector_x][idx_y].erase(id);
		clients[id].sector_x = idx_x;
		g_ObjectListSector[idx_x][idx_y].insert(id);
	}
	else if (idx_y != clients[id].sector_y)
	{
		g_ObjectListSector[idx_x][clients[id].sector_y].erase(id);
		clients[id].sector_y = idx_y;

		g_ObjectListSector[idx_x][idx_y].insert(id);
	}
}

bool can_see(int a, int b)
{
	int dist = (clients[a].x - clients[b].x) * (clients[a].x - clients[b].x) +
		(clients[a].y - clients[b].y) * (clients[a].y - clients[b].y);
	return dist <= VIEW_RANGE * VIEW_RANGE;

	//if (abs(clients[a].x - clients[b].x) > VIEW_RANGE) return false;
	//return (abs(clients[a].y - clients[b].y) <= VIEW_RANGE);
}

void SESSION::send_move_packet(int c_id)
{
	SC_MOVE_PLAYER_PACKET p;
	p.id = c_id;
	p.size = sizeof(SC_MOVE_PLAYER_PACKET);
	p.type = SC_MOVE_PLAYER;
	p.x = clients[c_id].x;
	p.y = clients[c_id].y;
	p.cx = clients[c_id].cx;
	p.cy = clients[c_id].cy;
	p.move_time = clients[c_id]._last_move_time;
	do_send(&p);
}

void SESSION::send_add_player_packet(int c_id)
{
	_vl_l.lock();
	view_list.insert(c_id);
	_vl_l.unlock();
	SC_ADD_PLAYER_PACKET add_packet;
	add_packet.id = c_id;
	strcpy_s(add_packet.name, clients[c_id]._name);
	add_packet.size = sizeof(add_packet);
	add_packet.type = SC_ADD_PLAYER;
	add_packet.x = clients[c_id].x;
	add_packet.y = clients[c_id].y;
	do_send(&add_packet);
}

int get_new_client_id()
{
	for (int i = 0; i < MAX_USER; ++i) {
		lock_guard <mutex> ll{ clients[i]._s_lock };
		if (clients[i]._state == ST_FREE)
			return i;
	}
	return -1;
}

void process_packet(int c_id, char* packet)
{
	switch (packet[1]) {
	case CS_LOGIN: {
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
		strcpy_s(clients[c_id]._name, p->name);
		int sector_x = 10;
		int sector_y = 10;
		get_best_sector(&sector_x, &sector_y);
		int pos_x = sector_x;
		int pos_y = sector_y;
		clients[c_id].x = pos_x;
		clients[c_id].y = pos_y;
		clients[c_id].y = clients[c_id].y * -1.0f;
		clients[c_id].cx = clients[c_id].x * -1.0f;
		clients[c_id].cy = clients[c_id].y * -1.0f;
		clients[c_id].send_login_info_packet();
		{
			lock_guard<mutex> ll{ clients[c_id]._s_lock };
			clients[c_id]._state = ST_INGAME;
		}
		for (auto& pl : clients) {
			{
				lock_guard<mutex> ll(pl._s_lock);
				if (ST_INGAME != pl._state) continue;
			}
			if (pl._id == c_id) continue;
			pl.send_add_player_packet(c_id);
			clients[c_id].send_add_player_packet(pl._id);
		}

		// �α��� �� sector ����
		init_sector(c_id, pos_x, pos_y);

		break;
	}
	case CS_MOVE: {
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		clients[c_id]._last_move_time = p->move_time;
		float x = clients[c_id].x;
		float y = clients[c_id].y;
		float cx = clients[c_id].cx;
		float cy = clients[c_id].cy;
		cx = 0.0f;
		cy = 0.0f;
		switch (p->direction) {
		case 0: { if (y < 0) { y += 1.0f; cy = -1.0f; } break; }
		case 1: { if (y > -W_HEIGHT + 1) { y -= 1.0f; cy = 1.0f; } break; }
		case 2: { if (x > 0) { x -= 1.0f; cx = 1.0f; } break; }
		case 3: { if (x < W_WIDTH - 1) { x += 1.0f; cx = -1.0f; } break; }
		}
		clients[c_id].x = x;
		clients[c_id].y = y;
		clients[c_id].cx = cx;
		clients[c_id].cy = cy;

		clients[c_id]._vl_l.lock();
		// ������ client sector ����
		unordered_set<int> old_viewlist = clients[c_id].view_list;
		clients[c_id]._vl_l.unlock();
		unordered_set<int> new_viewlist;

		update_sector(c_id, x, y * -1.0f);

		// �ڱⰡ ���� sector�� �����ϴ� client�� �˻�
		unordered_set<int>& sector = find_sector(clients[c_id].sector_x, clients[c_id].sector_y);

		for (int id : sector) {
			if (clients[id]._state != ST_INGAME) continue;
			if (false == can_see(c_id, id)) continue;
			if (id == c_id) continue;
			new_viewlist.insert(id);
		}

		/*for (auto& pl : clients) {
			if (pl._state != ST_INGAME) continue;
			if (false == can_see(c_id, pl._id)) continue;
			if (pl._id == c_id) continue;
			new_viewlist.insert(pl._id);
		}*/
		clients[c_id].send_move_packet(c_id);

		for (int p_id : new_viewlist) {
			if (0 == old_viewlist.count(p_id)) {
				clients[c_id].send_add_player_packet(p_id);
				clients[p_id].send_add_player_packet(c_id);
			}
			else {
				clients[p_id].send_move_packet(c_id);
			}
		}

		for (int p_id : old_viewlist) {
			if (0 == new_viewlist.count(p_id)) {
				clients[c_id].send_remove_player_packet(p_id);
				clients[p_id].send_remove_player_packet(c_id);
			}
		}
	}
	}
}

void disconnect(int c_id)
{
	for (auto& pl : clients) {
		{
			lock_guard<mutex> ll(pl._s_lock);
			if (ST_INGAME != pl._state) continue;
		}
		if (pl._id == c_id) continue;
		pl.send_remove_player_packet(c_id);
	}
	closesocket(clients[c_id]._socket);

	lock_guard<mutex> ll(clients[c_id]._s_lock);
	clients[c_id]._state = ST_FREE;
}

void worker_thread(HANDLE h_iocp)
{
	while (true) {
		DWORD num_bytes;
		ULONG_PTR key;
		WSAOVERLAPPED* over = nullptr;
		BOOL ret = GetQueuedCompletionStatus(h_iocp, &num_bytes, &key, &over, INFINITE);
		OVER_EXP* ex_over = reinterpret_cast<OVER_EXP*>(over);
		if (FALSE == ret) {
			if (ex_over->_comp_type == OP_ACCEPT) cout << "Accept Error";
			else {
				cout << "GQCS Error on client[" << key << "]\n";
				disconnect(static_cast<int>(key));
				if (ex_over->_comp_type == OP_SEND) delete ex_over;
				continue;
			}
		}

		if ((0 == num_bytes) && ((ex_over->_comp_type == OP_RECV) || (ex_over->_comp_type == OP_SEND))) {
			disconnect(static_cast<int>(key));
			if (ex_over->_comp_type == OP_SEND) delete ex_over;
			continue;
		}

		switch (ex_over->_comp_type) {
		case OP_ACCEPT: {
			int client_id = get_new_client_id();
			if (client_id != -1) {
				{
					lock_guard<mutex> ll(clients[client_id]._s_lock);
					clients[client_id]._state = ST_ALLOC;
				}
				clients[client_id].x = 0.0f;
				clients[client_id].y = 0.0f;
				clients[client_id].cx = 0.0f;
				clients[client_id].cy = 0.0f;
				clients[client_id]._id = client_id;
				clients[client_id]._name[0] = 0;
				clients[client_id]._prev_remain = 0;
				clients[client_id]._socket = g_c_socket;
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_c_socket),
					h_iocp, client_id, 0);
				clients[client_id].do_recv();
				g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			}
			else {
				cout << "Max user exceeded.\n";
			}
			ZeroMemory(&g_a_over._over, sizeof(g_a_over._over));
			int addr_size = sizeof(SOCKADDR_IN);
			AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);
			break;
		}
		case OP_RECV: {
			int remain_data = num_bytes + clients[key]._prev_remain;
			char* p = ex_over->_send_buf;
			while (remain_data > 0) {
				int packet_size = p[0];
				if (packet_size <= remain_data) {
					process_packet(static_cast<int>(key), p);
					p = p + packet_size;
					remain_data = remain_data - packet_size;
				}
				else break;
			}
			clients[key]._prev_remain = remain_data;
			if (remain_data > 0) {
				memcpy(ex_over->_send_buf, p, remain_data);
			}
			clients[key].do_recv();
			break;
		}
		case OP_SEND:
			delete ex_over;
			break;
		}
	}
}

int main()
{
	HANDLE h_iocp;

	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	g_s_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	SOCKADDR_IN server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(PORT_NUM);
	server_addr.sin_addr.S_un.S_addr = INADDR_ANY;
	bind(g_s_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));
	listen(g_s_socket, SOMAXCONN);
	SOCKADDR_IN cl_addr;
	int addr_size = sizeof(cl_addr);
	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), h_iocp, 9999, 0);
	g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_a_over._comp_type = OP_ACCEPT;
	AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);

	vector <thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();
	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(worker_thread, h_iocp);
	for (auto& th : worker_threads)
		th.join();
	closesocket(g_s_socket);
	WSACleanup();
}