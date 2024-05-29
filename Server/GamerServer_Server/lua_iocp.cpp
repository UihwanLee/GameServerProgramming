#include <iostream>
#include <array>
#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <vector>
#include <mutex>
#include <unordered_set>
#include <concurrent_priority_queue.h>
#include "protocol.h"
#include "DB.h"

#include "include/lua.hpp"


#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")
#pragma comment(lib, "lua54.lib")
using namespace std;

constexpr int VIEW_RANGE = 5;

enum EVENT_TYPE { EV_RANDOM_MOVE };

struct TIMER_EVENT {
	int obj_id;
	chrono::system_clock::time_point wakeup_time;
	EVENT_TYPE event_id;
	int target_id;
	constexpr bool operator < (const TIMER_EVENT& L) const
	{
		return (wakeup_time > L.wakeup_time);
	}
};
concurrency::concurrent_priority_queue<TIMER_EVENT> timer_queue;

enum COMP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND, OP_NPC_MOVE, OP_AI_MOVE };
class OVER_EXP {
public:
	WSAOVERLAPPED _over;
	WSABUF _wsabuf;
	char _send_buf[BUF_SIZE];
	COMP_TYPE _comp_type;
	int _ai_target_obj;
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
	atomic_bool	_is_active;		// 주위에 플레이어가 있는가?
	int _id;
	int	_dbId;
	SOCKET _socket;
	short	x, y;
	char	_name[NAME_SIZE];
	int		_prev_remain;
	unordered_set <int> _view_list;
	mutex	_vl;
	int		last_move_time;
	lua_State* _L;
	mutex	_ll;
public:
	SESSION()
	{
		_id = -1;
		_socket = 0;
		x = y = 0;
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
	void send_login_fail_packet()
	{
		SC_LOGIN_FAIL_PACKET p;
		p.size = sizeof(SC_LOGIN_INFO_PACKET);
		p.type = SC_LOGIN_FAIL;
		do_send(&p);
	}
	void send_login_info_packet()
	{
		SC_LOGIN_INFO_PACKET p;
		p.id = _id;
		p.size = sizeof(SC_LOGIN_INFO_PACKET);
		strcpy_s(p.name, sizeof(p.name), _name);
		p.type = SC_LOGIN_INFO;
		p.x = x;
		p.y = y;
		do_send(&p);
	}
	void send_move_packet(int c_id);
	void send_add_player_packet(int c_id);
	void send_chat_packet(int c_id, const char* mess);
	void send_remove_player_packet(int c_id)
	{
		_vl.lock();
		if (_view_list.count(c_id))
			_view_list.erase(c_id);
		else {
			_vl.unlock();
			return;
		}
		_vl.unlock();
		SC_REMOVE_OBJECT_PACKET p;
		p.id = c_id;
		p.size = sizeof(p);
		p.type = SC_REMOVE_OBJECT;
		do_send(&p);
	}
};

HANDLE h_iocp;
array<SESSION, MAX_USER + MAX_NPC> objects;
DB* db;

// NPC 구현 첫번째 방법
//  NPC클래스를 별도 제작, NPC컨테이너를 따로 생성한다.
//  장점 : 깔끔하다, 군더더기가 없다.
//  단점 : 플레이어와 NPC가 따로논다. 똑같은 역할을 수행하는 함수를 여러개씩 중복 작성해야 한다.
//         예) bool can_see(int from, int to)
//                 => bool can_see_p2p()
//				    bool can_see_p2n()
//					bool can_see_n2n()

// NPC 구현 두번째 방법  <===== 실습에서 사용할 방법.
//   objects 컨테이너에 NPC도 추가한다.
//   장점 : 플레이어와 NPC를 동일하게 취급할 수 있어서, 프로그래밍 작성 부하가 줄어든다.
//   단점 : 사용하지 않는 멤버들로 인한 메모리 낭비.

// NPC 구현 세번째 방법  (실제로 많이 사용되는 방법)
//   클래스 상속기능을 사용한다.
//     SESSION은 NPC클래스를 상속받아서 네트워크 관련 기능을 추가한 형태로 정의한다.
//       objects컨테이너를 objects컨테이너로 변경하고, 컨테이너는 NPC의 pointer를 저장한다.
//      장점 : 메모리 낭비가 없다, 함수의 중복작성이 필요없다.
//          (포인터로 관리되므로 player id의 중복사용 방지를 구현하기 쉬워진다 => Data Race 방지를 위한 추가 구현이 필요)
//      단점 : 포인터가 사용되고, reinterpret_cast가 필요하다. (별로 단점이 안니다).

SOCKET g_s_socket, g_c_socket;
OVER_EXP g_a_over;

bool is_pc(int object_id)
{
	return object_id < MAX_USER;
}

bool is_npc(int object_id)
{
	return !is_pc(object_id);
}

bool can_see(int from, int to)
{
	if (abs(objects[from].x - objects[to].x) > VIEW_RANGE) return false;
	return abs(objects[from].y - objects[to].y) <= VIEW_RANGE;
}

void SESSION::send_move_packet(int c_id)
{
	SC_MOVE_OBJECT_PACKET p;
	p.id = c_id;
	p.size = sizeof(SC_MOVE_OBJECT_PACKET);
	p.type = SC_MOVE_OBJECT;
	p.x = objects[c_id].x;
	p.y = objects[c_id].y;
	p.move_time = objects[c_id].last_move_time;
	do_send(&p);
}

void SESSION::send_add_player_packet(int c_id)
{
	SC_ADD_OBJECT_PACKET add_packet;
	add_packet.id = c_id;
	strcpy_s(add_packet.name, sizeof(add_packet.name), objects[c_id]._name);
	add_packet.size = sizeof(add_packet);
	add_packet.type = SC_ADD_OBJECT;
	add_packet.x = objects[c_id].x;
	add_packet.y = objects[c_id].y;
	_vl.lock();
	_view_list.insert(c_id);
	_vl.unlock();
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

int get_new_client_id()
{
	for (int i = 0; i < MAX_USER; ++i) {
		lock_guard <mutex> ll{ objects[i]._s_lock };
		if (objects[i]._state == ST_FREE)
			return i;
	}
	return -1;
}

void WakeUpNPC(int npc_id, int waker)
{
	OVER_EXP* exover = new OVER_EXP;
	exover->_comp_type = OP_AI_MOVE;
	exover->_ai_target_obj = waker;
	PostQueuedCompletionStatus(h_iocp, 1, npc_id, &exover->_over);

	if (objects[npc_id]._is_active) return;
	bool old_state = false;
	if (false == atomic_compare_exchange_strong(&objects[npc_id]._is_active, &old_state, true))
		return;
	TIMER_EVENT ev{ npc_id, chrono::system_clock::now(), EV_RANDOM_MOVE, 0 };
	timer_queue.push(ev);
}

void process_packet(int c_id, char* packet)
{
	switch (packet[1]) {
	case CS_LOGIN: {
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);

		// DB에 로그인 정보가 있는지 확인
		if (false == (db->check_id(p->id)))
		{
			objects[c_id].send_login_fail_packet();
			return;
		}

		objects[c_id]._dbId = p->id;

		errno_t err = strcpy_s(objects[c_id]._name, sizeof(objects[c_id]._name), db->getName());
		{
			lock_guard<mutex> ll{ objects[c_id]._s_lock };
			objects[c_id].x = db->getPosX();
			objects[c_id].y = db->getPosY();
			objects[c_id]._state = ST_INGAME;
		}
		objects[c_id].send_login_info_packet();
		for (auto& pl : objects) {
			{
				lock_guard<mutex> ll(pl._s_lock);
				if (ST_INGAME != pl._state) continue;
			}
			if (pl._id == c_id) continue;
			if (false == can_see(c_id, pl._id))
				continue;
			if (is_pc(pl._id)) pl.send_add_player_packet(c_id);
			else WakeUpNPC(pl._id, c_id);
			objects[c_id].send_add_player_packet(pl._id);
		}
		break;
	}
	case CS_MOVE: {
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		objects[c_id].last_move_time = p->move_time;
		short x = objects[c_id].x;
		short y = objects[c_id].y;
		switch (p->direction) {
		case 0: if (y > 0) y--; break;
		case 1: if (y < W_HEIGHT - 1) y++; break;
		case 2: if (x > 0) x--; break;
		case 3: if (x < W_WIDTH - 1) x++; break;
		}
		objects[c_id].x = x;
		objects[c_id].y = y;

		unordered_set<int> near_list;
		objects[c_id]._vl.lock();
		unordered_set<int> old_vlist = objects[c_id]._view_list;
		objects[c_id]._vl.unlock();
		for (auto& cl : objects) {
			if (cl._state != ST_INGAME) continue;
			if (cl._id == c_id) continue;
			if (can_see(c_id, cl._id))
				near_list.insert(cl._id);
		}

		db->update_pos(objects[c_id]._dbId, x, y);
		objects[c_id].send_move_packet(c_id);

		for (auto& pl : near_list) {
			auto& cpl = objects[pl];
			if (is_pc(pl)) {
				cpl._vl.lock();
				if (objects[pl]._view_list.count(c_id)) {
					cpl._vl.unlock();
					objects[pl].send_move_packet(c_id);
				}
				else {
					cpl._vl.unlock();
					objects[pl].send_add_player_packet(c_id);
				}
			}
			else WakeUpNPC(pl, c_id);

			if (old_vlist.count(pl) == 0)
				objects[c_id].send_add_player_packet(pl);
		}

		for (auto& pl : old_vlist)
			if (0 == near_list.count(pl)) {
				objects[c_id].send_remove_player_packet(pl);
				if (is_pc(pl))
					objects[pl].send_remove_player_packet(c_id);
			}
	}
				break;
	}
}

void disconnect(int c_id)
{
	objects[c_id]._vl.lock();
	unordered_set <int> vl = objects[c_id]._view_list;
	objects[c_id]._vl.unlock();
	for (auto& p_id : vl) {
		if (is_npc(p_id)) continue;
		auto& pl = objects[p_id];
		{
			lock_guard<mutex> ll(pl._s_lock);
			if (ST_INGAME != pl._state) continue;
		}
		if (pl._id == c_id) continue;
		pl.send_remove_player_packet(c_id);
	}
	closesocket(objects[c_id]._socket);

	lock_guard<mutex> ll(objects[c_id]._s_lock);
	objects[c_id]._state = ST_FREE;
}

void do_npc_random_move(int npc_id)
{
	SESSION& npc = objects[npc_id];
	unordered_set<int> old_vl;
	for (auto& obj : objects) {
		if (ST_INGAME != obj._state) continue;
		if (true == is_npc(obj._id)) continue;
		if (true == can_see(npc._id, obj._id))
			old_vl.insert(obj._id);
	}

	int x = npc.x;
	int y = npc.y;
	switch (rand() % 4) {
	case 0: if (x < (W_WIDTH - 1)) x++; break;
	case 1: if (x > 0) x--; break;
	case 2: if (y < (W_HEIGHT - 1)) y++; break;
	case 3:if (y > 0) y--; break;
	}
	npc.x = x;
	npc.y = y;

	unordered_set<int> new_vl;
	for (auto& obj : objects) {
		if (ST_INGAME != obj._state) continue;
		if (true == is_npc(obj._id)) continue;
		if (true == can_see(npc._id, obj._id))
			new_vl.insert(obj._id);
	}

	for (auto pl : new_vl) {
		if (0 == old_vl.count(pl)) {
			// 플레이어의 시야에 등장
			objects[pl].send_add_player_packet(npc._id);
		}
		else {
			// 플레이어가 계속 보고 있음.
			objects[pl].send_move_packet(npc._id);
		}
	}
	///vvcxxccxvvdsvdvds
	for (auto pl : old_vl) {
		if (0 == new_vl.count(pl)) {
			objects[pl]._vl.lock();
			if (0 != objects[pl]._view_list.count(npc._id)) {
				objects[pl]._vl.unlock();
				objects[pl].send_remove_player_packet(npc._id);
			}
			else {
				objects[pl]._vl.unlock();
			}
		}
	}
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
				objects[client_id].x = 0;
				objects[client_id].y = 0;
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
		case OP_NPC_MOVE: {
			bool keep_alive = false;
			for (int j = 0; j < MAX_USER; ++j) {
				if (objects[j]._state != ST_INGAME) continue;
				if (can_see(static_cast<int>(key), j)) {
					keep_alive = true;
					break;
				}
			}
			if (true == keep_alive) {
				do_npc_random_move(static_cast<int>(key));
				TIMER_EVENT ev{ key, chrono::system_clock::now() + 1s, EV_RANDOM_MOVE, 0 };
				timer_queue.push(ev);
			}
			else {
				objects[key]._is_active = false;
			}
			delete ex_over;
		}
						break;
		case OP_AI_MOVE: {
			objects[key]._ll.lock();
			auto L = objects[key]._L;
			lua_getglobal(L, "event_player_move");
			lua_pushnumber(L, ex_over->_ai_target_obj);
			lua_pcall(L, 1, 0, 0);
			//lua_pop(L, 1);
			objects[key]._ll.unlock();
			delete ex_over;
		}
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

void InitializeNPC()
{
	cout << "NPC intialize begin.\n";
	for (int i = MAX_USER; i < MAX_USER + MAX_NPC; ++i) {
		objects[i].x = rand() % W_WIDTH;
		objects[i].y = rand() % W_HEIGHT;
		objects[i]._id = i;
		sprintf_s(objects[i]._name, "NPC%d", i);
		objects[i]._state = ST_INGAME;

		auto L = objects[i]._L = luaL_newstate();
		luaL_openlibs(L);
		luaL_loadfile(L, "npc.lua");
		lua_pcall(L, 0, 0, 0);

		lua_getglobal(L, "set_uid");
		lua_pushnumber(L, i);
		lua_pcall(L, 1, 0, 0);
		// lua_pop(L, 1);// eliminate set_uid from stack after call

		lua_register(L, "API_SendMessage", API_SendMessage);
		lua_register(L, "API_get_x", API_get_x);
		lua_register(L, "API_get_y", API_get_y);
	}
	cout << "NPC initialize end.\n";
}

void do_timer()
{
	while (true) {
		TIMER_EVENT ev;
		auto current_time = chrono::system_clock::now();
		if (true == timer_queue.try_pop(ev)) {
			if (ev.wakeup_time > current_time) {
				timer_queue.push(ev);		// 최적화 필요
				// timer_queue에 다시 넣지 않고 처리해야 한다.
				this_thread::sleep_for(1ms);  // 실행시간이 아직 안되었으므로 잠시 대기
				continue;
			}
			switch (ev.event_id) {
			case EV_RANDOM_MOVE:
				OVER_EXP* ov = new OVER_EXP;
				ov->_comp_type = OP_NPC_MOVE;
				PostQueuedCompletionStatus(h_iocp, 1, ev.obj_id, &ov->_over);
				break;
			}
			continue;		// 즉시 다음 작업 꺼내기
		}
		this_thread::sleep_for(1ms);   // timer_queue가 비어 있으니 잠시 기다렸다가 다시 시작
	}
}

int main()
{
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

	db = new DB();

	InitializeNPC();

	h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	CreateIoCompletionPort(reinterpret_cast<HANDLE>(g_s_socket), h_iocp, 9999, 0);
	g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	g_a_over._comp_type = OP_ACCEPT;
	AcceptEx(g_s_socket, g_c_socket, g_a_over._send_buf, 0, addr_size + 16, addr_size + 16, 0, &g_a_over._over);

	vector <thread> worker_threads;
	int num_threads = std::thread::hardware_concurrency();
	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(worker_thread, h_iocp);
	thread timer_thread{ do_timer };
	timer_thread.join();
	for (auto& th : worker_threads)
		th.join();
	closesocket(g_s_socket);
	WSACleanup();
}