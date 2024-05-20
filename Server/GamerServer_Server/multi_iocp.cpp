#include <iostream>
#include <array>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <vector>
#include <mutex>
#include <unordered_set>
#include <queue>
#include "protocol.h"

#include "include/lua.hpp"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
#pragma comment(lib, "lua54.lib")
using namespace std;

enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND, OP_RANDOM_MOVE, OP_AI_MOVE };

constexpr int VIEW_RANGE = 5;		// 실제 클라이언트 시야보다 약간 작게
constexpr int NCP_START = 0;
constexpr int USER_START = MAX_NPC;

bool is_npc(int a)
{
	return a < MAX_NPC;
}

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
	atomic_bool _active;
	int _id;
	SOCKET _socket;
	float	x, y;
	float	cx, cy;
	char	_name[NAME_SIZE];
	int		sector_x;
	int		sector_y;
	chrono::system_clock::time_point _rm_time;
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
		if (true == is_npc(_id)) {
			cout << "Send to NPC!! Error";
			return;
		}
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
	void send_add_object_packet(int c_id);
	void send_chat_packet(int c_id, const char* mess);
	void send_remove_object_packet(int c_id)
	{
		if (true == is_npc(_id))
			return;

		_vl_l.lock();
		view_list.erase(c_id);
		_vl_l.unlock();
		SC_REMOVE_OBJECT_PACKET p;
		p.id = c_id;
		p.size = sizeof(p);
		p.type = SC_REMOVE_OBJECT;
		do_send(&p);
	}
	void do_random_move();
	bool _is_npc()
	{
		return (_id >= NCP_START && _id < MAX_NPC);
	}
};

enum EVENT_TYPE { EV_RANDOM_MOVE, EV_HEAL, EV_ATTACK };
struct EVENT {
	int obj_id;
	chrono::system_clock::time_point wakeup_time;
	EVENT_TYPE e_type;
	int target_id;

	constexpr bool operator < (const EVENT& _Left) const
	{
		return (wakeup_time > _Left.wakeup_time);
	}
};

priority_queue<EVENT> g_event_queue;
mutex eql;

array<SESSION, MAX_NPC + MAX_USER> objects;

SOCKET g_s_socket, g_c_socket;
OVER_EXP g_a_over;

bool can_see(int a, int b)
{
	int dist = (objects[a].x - objects[b].x) * (objects[a].x - objects[b].x) +
		(objects[a].y - objects[b].y) * (objects[a].y - objects[b].y);
	return dist <= VIEW_RANGE * VIEW_RANGE;
}

void SESSION::send_move_packet(int c_id)
{
	if (true == is_npc(_id))
		return;

	SC_MOVE_OBJECT_PACKET p;
	p.id = c_id;
	p.size = sizeof(SC_MOVE_OBJECT_PACKET);
	p.type = SC_MOVE_OBJECT;
	p.x = objects[c_id].x;
	p.y = objects[c_id].y;
	p.cx = objects[c_id].cx;
	p.cy = objects[c_id].cy;
	p.move_time = objects[c_id]._last_move_time;
	do_send(&p);
}

void SESSION::send_add_object_packet(int c_id)
{
	if (true == is_npc(_id))
		return;

	_vl_l.lock();
	view_list.insert(c_id);
	_vl_l.unlock();
	SC_ADD_OBJECT_PACKET add_packet;
	add_packet.id = c_id;
	strcpy_s(add_packet.name, objects[c_id]._name);
	add_packet.size = sizeof(add_packet);
	add_packet.type = SC_ADD_OBJECT;
	add_packet.x = objects[c_id].x;
	add_packet.y = objects[c_id].y;
	do_send(&add_packet);
}

void SESSION::send_chat_packet(int p_id, const char* mess)
{
	SC_CHAT_PACKET packet;
	packet.id = p_id;
	packet.size = sizeof(packet);
	packet.type = SC_CHAT;
	strcpy_s(packet.mess, mess);
	do_send(&packet);
}


void SESSION::do_random_move()
{
	unordered_set<int> old_vl;
	for (int i = MAX_NPC; i < MAX_NPC + MAX_USER; ++i) {
		if (objects[i]._state != ST_INGAME) continue;
		if (true == can_see(i, _id)) old_vl.insert(i);
	}

	switch (rand() % 4)
	{
	case 0: { if (y < 0) { y += 1.0f; } break; }
	case 1: { if (y > -W_HEIGHT + 1) { y -= 1.0f; } break; }
	case 2: { if (x > 0) { x -= 1.0f; } break; }
	case 3: { if (x < W_WIDTH - 1) { x += 1.0f; } break; }
	}

	unordered_set<int> new_vl;
	for (int i = MAX_NPC; i < MAX_NPC + MAX_USER; ++i) {
		if (objects[i]._state != ST_INGAME) continue;
		if (true == can_see(i, _id)) new_vl.insert(i);
	}

	for (auto pl : new_vl) {
		if (0 == old_vl.count(pl))
			objects[pl].send_add_object_packet(_id);
		else
			objects[pl].send_move_packet(_id);
	}
	for (auto pl : old_vl) {
		if (0 == new_vl.count(pl))
			objects[pl].send_remove_object_packet(_id);
	}
}

int get_new_client_id()
{
	for (int i = MAX_NPC; i < MAX_NPC + MAX_USER; ++i) {
		lock_guard <mutex> ll{ objects[i]._s_lock };
		if (objects[i]._state == ST_FREE)
			return i;
	}
	return -1;
}

void add_timer(int _id, EVENT_TYPE type, int time)
{
	EVENT ev;
	ev.obj_id = _id;
	ev.e_type = type;
	ev.wakeup_time = chrono::system_clock::now() + chrono::milliseconds(time);

	eql.lock();
	g_event_queue.push(ev);
	eql.unlock();
}

void process_packet(int c_id, char* packet)
{
	switch (packet[1]) {
	case CS_LOGIN: {
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
		strcpy_s(objects[c_id]._name, p->name);
		int sector_x = 10;
		int sector_y = 10;
		//get_best_sector(&sector_x, &sector_y);
		int pos_x = rand() % W_WIDTH;
		int pos_y = rand() & W_HEIGHT;
		objects[c_id].x = pos_x;
		objects[c_id].y = pos_y;
		objects[c_id].y = objects[c_id].y * -1.0f;
		objects[c_id].cx = objects[c_id].x * -1.0f;
		objects[c_id].cy = objects[c_id].y * -1.0f;
		objects[c_id].send_login_info_packet();
		{
			lock_guard<mutex> ll{ objects[c_id]._s_lock };
			objects[c_id]._state = ST_INGAME;
		}
		for (auto& pl : objects) {
			{
				lock_guard<mutex> ll(pl._s_lock);
				if (ST_INGAME != pl._state) continue;
			}
			if (pl._id == c_id) continue;
			if (false == can_see(pl._id, c_id)) continue;
			pl.send_add_object_packet(c_id);
			objects[c_id].send_add_object_packet(pl._id);
		}

		break;
	}
	case CS_MOVE: {
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		objects[c_id]._last_move_time = p->move_time;
		float x = objects[c_id].x;
		float y = objects[c_id].y;
		float cx = objects[c_id].cx;
		float cy = objects[c_id].cy;
		cx = 0.0f;
		cy = 0.0f;
		switch (p->direction) {
		case 0: { if (y < 0) { y += 1.0f; cy = -1.0f; } break; }
		case 1: { if (y > -W_HEIGHT + 1) { y -= 1.0f; cy = 1.0f; } break; }
		case 2: { if (x > 0) { x -= 1.0f; cx = 1.0f; } break; }
		case 3: { if (x < W_WIDTH - 1) { x += 1.0f; cx = -1.0f; } break; }
		}
		objects[c_id].x = x;
		objects[c_id].y = y;
		objects[c_id].cx = cx;
		objects[c_id].cy = cy;

		objects[c_id]._vl_l.lock();
		// 움직인 client sector 갱신
		unordered_set<int> old_viewlist = objects[c_id].view_list;
		objects[c_id]._vl_l.unlock();
		unordered_set<int> new_viewlist;

		for (auto& pl : objects) {
			if (pl._state != ST_INGAME) continue;
			if (false == can_see(c_id, pl._id)) continue;
			if (pl._id == c_id) continue;
			new_viewlist.insert(pl._id);
			if ((true == pl._is_npc()) && (pl._active == false)) {
				bool expected = false;
				if (pl._active.compare_exchange_strong(expected, true)) {
					add_timer(pl._id, EV_RANDOM_MOVE, 1000);
				}
			}
		}
		objects[c_id].send_move_packet(c_id);

		for (int p_id : new_viewlist) {
			if (0 == old_viewlist.count(p_id)) {
				objects[c_id].send_add_object_packet(p_id);
				objects[p_id].send_add_object_packet(c_id);
			}
			else {
				objects[p_id].send_move_packet(c_id);
			}
		}

		for (int p_id : old_viewlist) {
			if (0 == new_viewlist.count(p_id)) {
				objects[c_id].send_remove_object_packet(p_id);
				objects[p_id].send_remove_object_packet(c_id);
			}
		}
	}
	}
}

void disconnect(int c_id)
{
	for (auto& pl : objects) {
		{
			lock_guard<mutex> ll(pl._s_lock);
			if (ST_INGAME != pl._state) continue;
		}
		if (pl._id == c_id) continue;
		if (true == can_see(pl._id, c_id))
			pl.send_remove_object_packet(c_id);
	}
	closesocket(objects[c_id]._socket);

	lock_guard<mutex> ll(objects[c_id]._s_lock);
	objects[c_id]._state = ST_FREE;
}

bool player_exist(int npc_id)
{
	for (int i = USER_START; i < USER_START + MAX_USER; ++i)
	{
		if (ST_INGAME != objects[i]._state)
			continue;
		if (true == can_see(npc_id, i))
			return true;
	}
	return false;
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
					lock_guard<mutex> ll(objects[client_id]._s_lock);
					objects[client_id]._state = ST_ALLOC;
				}
				objects[client_id].x = 0.0f;
				objects[client_id].y = 0.0f;
				objects[client_id].cx = 0.0f;
				objects[client_id].cy = 0.0f;
				objects[client_id]._id = client_id;
				objects[client_id]._name[0] = 0;
				objects[client_id]._prev_remain = 0;
				objects[client_id]._socket = g_c_socket;
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_c_socket),
					h_iocp, client_id, 0);
				objects[client_id].do_recv();
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
			int remain_data = num_bytes + objects[key]._prev_remain;
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
			objects[key]._prev_remain = remain_data;
			if (remain_data > 0) {
				memcpy(ex_over->_send_buf, p, remain_data);
			}
			objects[key].do_recv();
			break;
		}
		case OP_SEND:
			delete ex_over;
			break;
		case OP_RANDOM_MOVE:
			if (true == player_exist(key)) {
				objects[key].do_random_move();
				add_timer(key, EV_RANDOM_MOVE, 1000);
			}
			else {
				objects[key]._active = false;
			}
			delete ex_over;
			break;
		}
	}
}

int API_get_x(lua_State* L)
{
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int x = objects[user_id].x;
	lua_pushnumber(L, x);
	return 1;
}

int API_get_y(lua_State* L)
{
	int user_id =
		(int)lua_tointeger(L, -1);
	lua_pop(L, 2);
	int y = objects[user_id].y;
	lua_pushnumber(L, y);
	return 1;
}

int API_SendMessage(lua_State* L)
{
	int my_id = (int)lua_tointeger(L, -3);
	int user_id = (int)lua_tointeger(L, -2);
	char* mess = (char*)lua_tostring(L, -1);

	lua_pop(L, 4);

	objects[user_id].send_chat_packet(my_id, mess);
	return 0;
}

void initialize_npc()
{
	for (int i = 0; i < MAX_NPC; ++i) {
		int pos_x = rand() % W_WIDTH;
		int pos_y = rand() & W_HEIGHT;
		objects[i].x = pos_x;
		objects[i].y = pos_y;
		objects[i].y = objects[i].y * -1.0f;
		objects[i]._id = i;
		sprintf_s(objects[i]._name, "N%d", i);
		objects[i]._state = ST_INGAME;
		objects[i]._rm_time = chrono::system_clock::now();
		objects[i]._active = false;
	}
}

// timer로 제어
void do_timer(HANDLE h_iocp)
{
	using namespace chrono;
	while (true) {
		eql.lock();
		if (false == g_event_queue.empty())
		{
			EVENT ev = g_event_queue.top();
			if (ev.wakeup_time < system_clock::now()) {
				g_event_queue.pop();
				OVER_EXP* ov = new OVER_EXP;
				ov->_comp_type = OP_RANDOM_MOVE;
				PostQueuedCompletionStatus(h_iocp, 1, ev.obj_id, &ov->_over);
			}
		}
		eql.unlock();
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

	initialize_npc();
	thread ai_thread{ do_timer, h_iocp };

	vector <thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();
	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(worker_thread, h_iocp);
	for (auto& th : worker_threads)
		th.join();
	ai_thread.join();
	closesocket(g_s_socket);
	WSACleanup();
}