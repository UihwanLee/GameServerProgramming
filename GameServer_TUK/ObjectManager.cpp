#include "ObjectManager.h"

ObjectManager::ObjectManager()
{
}

ObjectManager::~ObjectManager()
{
	for (int i = 0; i < m_ObjectList.size(); i++)
	{
		delete m_ObjectList[i];
	}
}

void ObjectManager::CreateRect(int* idx)
{
	*idx += 1;

	Object* object = new Object();

	for (int i = 0; i < 12; i++) object->m_pos.emplace_back(Figure::RectVertexs[i]);
	for (int i = 0; i < 12; i++) object->m_col.emplace_back(Figure::RectColors[i]);
	for (int i = 0; i < 6; i++) object->m_index.emplace_back(Figure::RectIndecies[i]);

	m_ObjectList.emplace_back(object);
}

void ObjectManager::SetPosition(int idx, float x, float y, float z)
{
	if (m_ObjectList.empty()) return;

	m_ObjectList[idx]->SetPosition(x, y, z);
}

void ObjectManager::Reset()
{
	m_ObjectList.clear();
}

bool ObjectManager::IsActive(int idx)
{
	return m_ObjectList[idx]->IsActive();
}