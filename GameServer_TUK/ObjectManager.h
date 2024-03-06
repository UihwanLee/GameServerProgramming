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

	void CreateRect(int* idx);

	void SetPosition(int idx, float x, float y, float z);

	void Reset();

	bool IsActive(int idx);
};