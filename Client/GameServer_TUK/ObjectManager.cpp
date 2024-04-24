#include "ObjectManager.h"

ObjectManager::ObjectManager()
{
	serverID = 0;
	currentIDX = 0;
}

ObjectManager::~ObjectManager()
{
	for (int i = 0; i < m_ObjectList.size(); i++)
	{
		delete m_ObjectList[i];
	}
}

void ObjectManager::creatFigure(int* idx, highp_vec3 color, GLfloat* vertexs)
{
	*idx += 1;

	Object* object = new Object();

	for (int i = 0; i < 12; i++) object->m_pos.emplace_back(vertexs[i]);
	for (int i = 0; i < 6; i++) object->m_index.emplace_back(Figure::RectIndecies[i]);

	for (int i = 0; i < 4; i++)
	{
		object->m_col.emplace_back(color.r);
		object->m_col.emplace_back(color.g);
		object->m_col.emplace_back(color.b);
	}

	m_ObjectList.emplace_back(object);
}

void ObjectManager::createRect(int* idx, highp_vec3 color)
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

void ObjectManager::creatBoard(int *idx)
{
	// 체스판 만들기
	int index;
	for (const auto& board : Figure::Boards)
	{
		index = *idx + 1;
		if ((index / 8) % 2 == 0)
		{
			createRect(idx, (*idx % 2 != 0) ? Figure::boardColorType1 : Figure::boardColorType2);
		}
		else
		{
			createRect(idx, (*idx % 2 == 0) ? Figure::boardColorType1 : Figure::boardColorType2);
		}
		setPosition(*idx, board);
	}
}

Object* ObjectManager::creatPlayer()
{
	Object* player = new Object();

	for (int i = 0; i < 12; i++) player->m_pos.emplace_back(Figure::PlayerVertex[i]);
	for (int i = 0; i < 6; i++) player->m_index.emplace_back(Figure::RectIndecies[i]);

	for (int i = 0; i < 4; i++)
	{
		player->m_col.emplace_back(0.0f);
		player->m_col.emplace_back(0.0f);
		player->m_col.emplace_back(0.0f);
	}

	return player;
}

void ObjectManager::setPlayerPosition(int idx, float x, float y)
{
	if (m_players.empty()) return;

	m_players[idx]->setPosition(x, y, 0);
}

void ObjectManager::setPosition(int idx, float x, float y, float z)
{
	if (m_ObjectList.empty()) return;

	m_ObjectList[idx]->setPosition(x, y, z);

	// 자식이 존재한다면 자식에게도 적용
	if (!m_ObjectList[idx]->childs.empty()) {
		for (int i = 0; i < m_ObjectList[idx]->childs.size(); i++){
			m_ObjectList[m_ObjectList[idx]->childs[i]]->setPosition(x, y, z);
		}
	}
}

void ObjectManager::setPosition(int idx, vec3 position)
{
	if (m_ObjectList.empty()) return;

	m_ObjectList[idx]->setPosition(position);

	// 자식이 존재한다면 자식에게도 적용
	if (!m_ObjectList[idx]->childs.empty()) {
		for (int i = 0; i < m_ObjectList[idx]->childs.size(); i++) {
			m_ObjectList[m_ObjectList[idx]->childs[i]]->setPosition(position);
		}
	}
}

void ObjectManager::move(int idx, float x, float y)
{
	if (m_players.empty()) return;

	m_players[idx]->move(x, y, 0);
}

void ObjectManager::move(int idx, vec3 position)
{
	if (m_ObjectList.empty()) return;

	m_ObjectList[idx]->move(position);

	// 자식이 존재한다면 자식에게도 적용
	if (!m_ObjectList[idx]->childs.empty()) {
		for (int i = 0; i < m_ObjectList[idx]->childs.size(); i++) {
			m_ObjectList[m_ObjectList[idx]->childs[i]]->move(position);
		}
	}
}

void ObjectManager::reset()
{
	m_ObjectList.clear();
}

bool ObjectManager::isActive(int idx)
{
	return m_ObjectList[idx]->isActive();
}

void ObjectManager::setChild(int idx, int child)
{
	m_ObjectList[idx]->setChild(child);
}

mat4 ObjectManager::transformModel(int idx)
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

mat4 ObjectManager::transformModelPlayer(int idx)
{
	mat4 model = glm::mat4(1.0f);
	mat4 scale = glm::mat4(1.0f);
	mat4 rot = glm::mat4(1.0f);
	mat4 move = glm::mat4(1.0f);

	if (!m_players.empty())
	{
		float move_x = m_players[idx]->m_position.x;
		float move_y = m_players[idx]->m_position.y;
		float move_z = m_players[idx]->m_position.z;

		float rotate_x = m_players[idx]->m_rotate.x;
		float rotate_y = m_players[idx]->m_rotate.y;
		float rotate_z = m_players[idx]->m_rotate.z;

		float scale_x = m_players[idx]->m_scale.x;
		float scale_y = m_players[idx]->m_scale.y;
		float scale_z = m_players[idx]->m_scale.z;

		model = glm::mat4(1.0f);

		model = glm::scale(model, glm::vec3(scale_x, scale_y, scale_z));
		model = glm::rotate(model, glm::radians(rotate_x), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(rotate_y), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(rotate_z), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::translate(model, glm::vec3(move_x, move_y, move_z));
	}

	return model;
}

void ObjectManager::setCurrentIDX(int idx)
{
	currentIDX = idx;
}

int ObjectManager::getCurrentIDX()
{
	return currentIDX;
}

void ObjectManager::setServerID(int id)
{
	serverID = id;
}

int ObjectManager::getServerID()
{
	return serverID;
}

void ObjectManager::addPlayer(int idx)
{
	m_playerIDList.emplace_back(idx);
}

int ObjectManager::getMyPlayer()
{
	if (m_playerIDList.empty()) return 0;

	// 항상 초기 생성되는 말이 자기 자신이므로 0번째 인덱스 리턴
	return m_playerIDList[serverID];
}
