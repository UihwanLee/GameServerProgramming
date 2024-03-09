#include "Object.h"

Object::Object()
{
	m_model = mat4(1.0f);

	m_position = vec3(0.f);
	m_rotate = vec3(0.f);
	m_scale = vec3(1.f);

	m_bActive = true;

	childs.clear();
}

Object::~Object()
{
}

void Object::setPosition(float x, float y, float z)
{
	m_position.x = x;
	m_position.y = y;
	m_position.z = z;
}

void Object::setRotate(float x, float y, float z)
{
}

void Object::setScale(float x, float y, float z)
{
}

void Object::setActive(bool bActive)
{
	m_bActive = bActive;
}

void Object::setPosition(vec3 position)
{
	m_position = position;
}

void Object::move(float x, float y, float z)
{
	m_position.x += x;
	m_position.y += y;
	m_position.z += z;
}

void Object::rotate(float x, float y, float z)
{
}

void Object::scale(float x, float y, float z)
{
}

void Object::move(vec3 position)
{
	m_position = position;
}

bool Object::isActive()
{
	return m_bActive;
}

void Object::setChild(int idx)
{
	childs.push_back(idx);
}
