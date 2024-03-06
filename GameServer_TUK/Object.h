#pragma once

#include "pch.h"

class Object
{
private:
	vec3		m_position;
	vec3		m_rotate;
	vec3		m_scale;

	highp_vec3	m_colors;
	bool		m_bActive;

public:
	vector<GLfloat> m_pos;
	vector<GLfloat> m_col;
	vector<GLint> m_index;

public:
	Object();
	~Object();

	void SetPosition(float x, float y, float z);
	void SetRotate(float x, float y, float z);
	void SetScale(float x, float y, float z);
	void SetActive(bool bActive);	

	void Move(float x, float y, float z);
	void Rotate(float x, float y, float z);
	void Scale(float x, float y, float z);

	bool IsActive();
};