#pragma once

#include "pch.h"

class Object
{
public :
	mat4		m_model;

	vec3		m_position;
	vec3		m_rotate;
	vec3		m_scale;

	highp_vec3	m_colors;
	bool		m_bActive;

	vector<int> childs;

public:
	vector<GLfloat> m_pos;
	vector<GLfloat> m_col;
	vector<GLint> m_index;

public:
	Object();
	~Object();

	void setPosition(float x, float y, float z);
	void setRotate(float x, float y, float z);
	void setScale(float x, float y, float z);
	void setActive(bool bActive);	

	void setPosition(vec3 position);

	void move(float x, float y, float z);
	void rotate(float x, float y, float z);
	void scale(float x, float y, float z);

	void move(vec3 position);

	bool isActive();

	void setChild(int idx);
};