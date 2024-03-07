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

void ObjectManager::CreateRect(int* idx, highp_vec3 color)
{
	*idx += 1;

	Object* object = new Object();

	for (int i = 0; i < 12; i++) object->m_pos.emplace_back(Figure::RectVertexs[i]);
	for (int i = 0; i < 6; i++) object->m_index.emplace_back(Figure::RectIndecies[i]);

	for (int i = 0; i < 4; i++)
	{
		object->m_col.emplace_back(color.r);
		object->m_col.emplace_back(color.g);
		object->m_col.emplace_back(color.b);
	}

	m_ObjectList.emplace_back(object);
}

void ObjectManager::CreatBoard(int *idx)
{
	// 체스판 만들기
	int index;
	for (const auto& board : Figure::Boards)
	{
		index = *idx + 1;
		if ((index / 8) % 2 == 0)
		{
			CreateRect(idx, (*idx % 2 != 0) ? Figure::boardColorType1 : Figure::boardColorType2);
		}
		else
		{
			CreateRect(idx, (*idx % 2 == 0) ? Figure::boardColorType1 : Figure::boardColorType2);
		}
		SetPosition(*idx, board);
	}
}

void ObjectManager::SetPosition(int idx, float x, float y, float z)
{
	if (m_ObjectList.empty()) return;

	m_ObjectList[idx]->SetPosition(x, y, z);
}

void ObjectManager::SetPosition(int idx, vec3 position)
{
	if (m_ObjectList.empty()) return;

	m_ObjectList[idx]->SetPosition(position);
}

void ObjectManager::Move(int idx, float x, float y, float z)
{
	if (m_ObjectList.empty()) return;

	m_ObjectList[idx]->Move(x, y, z);
}

void ObjectManager::Move(int idx, vec3 position)
{
	if (m_ObjectList.empty()) return;


}

void ObjectManager::Reset()
{
	m_ObjectList.clear();
}

bool ObjectManager::IsActive(int idx)
{
	return m_ObjectList[idx]->IsActive();
}

mat4 ObjectManager::TransformModel(int idx)
{
	mat4 model = glm::mat4(1.0f);
	mat4 scale = glm::mat4(1.0f);
	mat4 rot = glm::mat4(1.0f);
	mat4 move = glm::mat4(1.0f);

	if (!m_ObjectList.empty())
	{
		float move_x = m_ObjectList[idx]->m_position.x;
		float move_y = m_ObjectList[idx]->m_position.y;
		float move_z = m_ObjectList[idx]->m_position.z;

		float rotate_x = m_ObjectList[idx]->m_rotate.x;
		float rotate_y = m_ObjectList[idx]->m_rotate.y;
		float rotate_z = m_ObjectList[idx]->m_rotate.z;

		float scale_x = m_ObjectList[idx]->m_scale.x;
		float scale_y = m_ObjectList[idx]->m_scale.y;
		float scale_z = m_ObjectList[idx]->m_scale.z;

		model = glm::mat4(1.0f);

		model = glm::scale(model, glm::vec3(scale_x, scale_y, scale_z));
		model = glm::rotate(model, glm::radians(rotate_x), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(rotate_y), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(rotate_z), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::translate(model, glm::vec3(move_x, move_y, move_z));
	}

	return model;
}
