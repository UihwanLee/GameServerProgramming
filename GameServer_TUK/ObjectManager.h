#pragma once

#include "pch.h"
#include "Object.h"
#include "Figure.h"

class ObjectManager
{
public:
	vector<Object*>	m_ObjectList;

public:
	ObjectManager();
	~ObjectManager();

	void CreateRect(int* idx, highp_vec3 color);
	void CreatBoard(int* idx);

	void SetPosition(int idx, float x, float y, float z);
	void SetPosition(int idx, vec3 position);

	void Move(int idx, float x, float y, float z);
	void Move(int idx, vec3 position);

	void Reset();

	bool IsActive(int idx);

public:
	mat4 TransformModel(int idx);
};