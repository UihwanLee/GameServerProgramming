#include "Object.h"

Object::Object()
{
	m_model = mat4(1.0f);

	m_position = vec3(0.f);
	m_rotate = vec3(0.f);
	m_scale = vec3(1.f);

	m_bActive = true;
}

Object::~Object()
{
}

void Object::SetPosition(float x, float y, float z)
{
	m_position.x = x;
	m_position.y = y;
	m_position.z = z;
}

void Object::SetRotate(float x, float y, float z)
{
}

void Object::SetScale(float x, float y, float z)
{
}

void Object::SetActive(bool bActive)
{
	m_bActive = bActive;
}

void Object::SetPosition(vec3 position)
{
	m_position = position;
}

void Object::Move(float x, float y, float z)
{
	m_position.x += x;
	m_position.y += y;
	m_position.z += z;
}

void Object::Rotate(float x, float y, float z)
{
}

void Object::Scale(float x, float y, float z)
{
}

void Object::Move(vec3 position)
{
	m_position = position;
}

bool Object::IsActive()
{
	return m_bActive;
}