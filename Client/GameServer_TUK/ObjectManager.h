#pragma once

#include "pch.h"
#include "Object.h"
#include "Figure.h"

class ObjectManager
{
public:
	vector<Object*>	m_ObjectList;
	vector<int> m_playerIDList;
	unordered_map<int, Object*> m_players;
	int serverID;
	int currentIDX;

public:
	ObjectManager();
	~ObjectManager();

	void creatFigure(int* idx, highp_vec3 color, GLfloat* vertexs);
	void createRect(int* idx, highp_vec3 color);
	void creatBoard(int* idx);

	Object* creatPlayer(int id);
	void setPlayerPosition(int idx, float x, float y);

	void setPosition(int idx, float x, float y, float z);
	void setPosition(int idx, vec3 position);

	void move(int idx, float x, float y);
	void move(int idx, vec3 position);

	void reset();

	bool isActive(int idx);

	void setChild(int idx, int child);

public:
	mat4 transformModel(int idx);
	mat4 transformModelPlayer(int idx);
	void setCurrentIDX(int idx);
	int getCurrentIDX();

public:
	void setServerID(int id);
	int getServerID();
	void addPlayer(int idx);
	int getMyPlayer();
};