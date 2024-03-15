#pragma once

#include "pch.h"
#include "Object.h"
#include "Figure.h"

class ObjectManager
{
public:
	vector<Object*>	m_ObjectList;
	int currentIDX;

public:
	ObjectManager();
	~ObjectManager();

	void creatFigure(int* idx, highp_vec3 color, GLfloat* vertexs);
	void createRect(int* idx, highp_vec3 color);
	void creatBoard(int* idx);

	int creatPlayer(int* idx);

	void setPosition(int idx, float x, float y, float z);
	void setPosition(int idx, vec3 position);

	void move(int idx, float x, float y, float z);
	void move(int idx, vec3 position);

	void reset();

	bool isActive(int idx);

	void setChild(int idx, int child);

public:
	mat4 transformModel(int idx);
	void setCurrentIDX(int idx);
	int getCurrentIDX();
};