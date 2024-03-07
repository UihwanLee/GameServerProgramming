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

	void CreateRect(int* idx, highp_vec3 color);
	void CreatBoard(int* idx);

	int CreatPlayer(int* idx);

	void SetPosition(int idx, float x, float y, float z);
	void SetPosition(int idx, vec3 position);

	void Move(int idx, float x, float y, float z);
	void Move(int idx, vec3 position);

	void GoUp(int idx);
	void GoDown(int idx);
	void GoLeft(int idx);
	void GoRight(int idx);

	void Reset();

	bool IsActive(int idx);

public:
	mat4 TransformModel(int idx);
};