#include "pch.h"
#include "ObjectManager.h"

#include <WS2tcpip.h>
#pragma comment (lib, "WS2_32.LIB")

constexpr short PORT = 4000;
constexpr char SERVER_ADDR[] = "127.0.0.1";
constexpr int BUFSIZE = 256;

// ���� 
GLvoid server();
GLvoid client(int argc, char** argv);

// �ݺ� �Լ�
GLvoid render(GLvoid);
GLvoid reshape(int w, int h);
GLvoid specialKeyBoard(int key, int x, int y);

// Texture
unsigned int texture;

// VAO, VBO
GLuint VAO, VBO[2], EBO;

BITMAPINFO* bmp;

GLchar* vertexSource, * fragmentSource; //--- �ҽ��ڵ� ���� ����
GLuint vertexShader, fragmentShader; //���̴� ��ü
GLuint shaderProgramID; // ���̴� ���α׷�

GLint createShader(const char* file, int type);
GLvoid createShaderProgram();

GLvoid initBuffer();
GLvoid initObjects();

// INDEX
int idx = -1;
int playerIDX = -1;

// ī�޶�
mat4 camera = mat4(1.0f);

// �ﰢ�� �׸��� �Լ�
void drawView();
void drawProjection();
void drawObjects(int idx);

// ������Ʈ ����Ʈ
ObjectManager* m_ObjectManager = new ObjectManager();

void main(int argc, char** argv)
{
	server();
	//client(argc, argv);
}

GLvoid server()
{
	std::wcout.imbue(locale("korean"));

	// window ��Ʈ��ũ ���α׷��� �� ������ ���� ���α׷����� ȣȯ���� ���� �ʿ�
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 0), &WSAData);

	// SOCKET ����
	SOCKET server_s = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, 0);

	// SOCK ADDR ����
	SOCKADDR_IN server_a;

	server_a.sin_family = AF_INET;
	server_a.sin_port = htons(PORT);
	inet_pton(AF_INET, SERVER_ADDR, &server_a.sin_addr);

	connect(server_s, reinterpret_cast<sockaddr *>(& server_a), sizeof(server_a));

	while (true) {
		char buf[BUFSIZE];

		std::cout << "Enter Message: ";
		std::cin.getline(buf, BUFSIZE);

		WSABUF wsabuf[1];
		wsabuf[0].buf = buf;
		wsabuf[0].len = static_cast<int>(strlen(buf)) + 1;

		if (wsabuf[0].len == 1)
			break;

		DWORD sent_size;
		WSASend(server_s, wsabuf, 1, &sent_size, 0, 0, 0);

		wsabuf[0].len = BUFSIZE;
		DWORD recv_size;
		DWORD recv_flag = 0;
		WSARecv(server_s, wsabuf, 1, &recv_size, &recv_flag, nullptr, nullptr);

		for (DWORD i = 0; i < recv_size; ++i)
			std::cout << buf[i];
		std::cout << std::endl;
	}
	closesocket(server_s);
	WSACleanup();
}

GLvoid client(int argc, char** argv)
{
	// ������ ����
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(WIDTH, HEIGHT);
	glutCreateWindow("GameServerTUK");

	//--- GLEW �ʱ�ȭ�ϱ�
	glewExperimental = GL_TRUE;
	glewInit();

	initBuffer();
	createShaderProgram();

	glutDisplayFunc(render);

	// Ű����
	glutSpecialFunc(specialKeyBoard);

	glutMainLoop();
}

GLvoid reshape(int w, int h)
{
	// ����Ʈ �⺻ WIDTH HEIGHT�� ����
	glViewport(0, 0, w, h);
}

GLvoid initBuffer()
{
	glGenVertexArrays(1, &VAO);

	glGenBuffers(2, VBO);
	glGenBuffers(1, &EBO);
	glGenTextures(1, &texture);

	initObjects();
}

GLvoid initObjects()
{
	m_ObjectManager->reset();

	// ü���� ����
	m_ObjectManager->creatBoard(&idx);

	// �÷��̾� ����
	playerIDX = m_ObjectManager->creatPlayer(&idx);

	// ī�޶� ����
	camera = glm::translate(camera, glm::vec3(0.0f, 0.0f, -10.0f));
}

char* getBuf(const char* file)
{
	FILE* fptr;
	long length;
	char* buf;

	// ���� �б� �������� ����
	fopen_s(&fptr, file, "rb");

	// ����ó��
	if (!fptr) return NULL;

	// ���� buf�� ��ȯ
	fseek(fptr, 0, SEEK_END);
	length = ftell(fptr);
	buf = (char*)malloc(length + 1);
	fseek(fptr, 0, SEEK_SET);
	fread(buf, length, 1, fptr);

	// ���� ����
	fclose(fptr);

	buf[length] = 0;
	return buf;
}

GLint createShader(const char* file, int type)
{
	// glsl ���� �б�
	GLchar* source = getBuf(file);

	// ��ü ����
	GLint shader = glCreateShader(type);
	glShaderSource(shader, 1, (const GLchar**)&source, 0);
	glCompileShader(shader);

	// ������ ���� üũ
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);

	if (!result)
	{
		glGetShaderInfoLog(shader, 512, NULL, errorLog);
		// std::cerr << "ERROR: ������ ����\n" << errorLog << std::endl;
		return 0;
	}

	return shader;
}

void createShaderProgram()
{
	vertexShader = createShader("vertex.glsl", GL_VERTEX_SHADER);
	fragmentShader = createShader("fragment.glsl", GL_FRAGMENT_SHADER);

	// ���̴� ���α׷� ����
	shaderProgramID = glCreateProgram();
	glAttachShader(shaderProgramID, vertexShader);
	glAttachShader(shaderProgramID, fragmentShader);
	glLinkProgram(shaderProgramID);

	// ���̴� ����
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// ���̴� ���α׷� ���
	glUseProgram(shaderProgramID);
}

GLvoid render()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	drawView();
	drawProjection();
	for (int i = 0; i < m_ObjectManager->m_ObjectList.size(); i++)
	{
		if (m_ObjectManager->m_ObjectList[i]->isActive())
		{
			drawObjects(i);
		}
	}

	glutSwapBuffers();
}

void drawView()
{
	unsigned int viewLocation = glGetUniformLocation(shaderProgramID, "viewTransform");

	glUniformMatrix4fv(viewLocation, 1, GL_FALSE,glm::value_ptr(camera));
}

void drawProjection()
{
	unsigned int projectionLocation = glGetUniformLocation(shaderProgramID, "projectionTransform");

	glm::mat4 projection = glm::mat4(1.0f);

	// ���� ����
	float fov = 45.0f; // �þ߰� (Field of View)
	float aspectRatio = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT); // ȭ���� ���� ���� ����
	float zNear = 0.1f; // ����� Ŭ���� ���
	float zFar = 50.0f; // �� Ŭ���� ���
	projection = glm::perspective(glm::radians(fov), aspectRatio, zNear, zFar); //--- ���� ���� ����: (���װ���, ��Ⱦ��, �����Ÿ�, �հŸ�)

	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, &projection[0][0]);		// ������ȯ
}

void drawObjects(int idx)	
{
	// Model
	unsigned int modelLocation = glGetUniformLocation(shaderProgramID, "modelTransform");
	m_ObjectManager->m_ObjectList[idx]->m_model = m_ObjectManager->transformModel(idx);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(m_ObjectManager->m_ObjectList[idx]->m_model));

	// Position / Color
	glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, m_ObjectManager->m_ObjectList[idx]->m_pos.size() * sizeof(GLfloat), &m_ObjectManager->m_ObjectList[idx]->m_pos[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_ObjectManager->m_ObjectList[idx]->m_index.size() * sizeof(GLfloat), &m_ObjectManager->m_ObjectList[idx]->m_index[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);

	glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
	glBufferData(GL_ARRAY_BUFFER, m_ObjectManager->m_ObjectList[idx]->m_col.size() * sizeof(GLfloat), &m_ObjectManager->m_ObjectList[idx]->m_col[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);

	glDrawElements(GL_TRIANGLES, m_ObjectManager->m_ObjectList[idx]->m_index.size(), GL_UNSIGNED_INT, 0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

void specialKeyBoard(int key, int x, int y)
{
	// ���伳 Ű���� �Է� ó��
	switch (key) {
	case GLUT_KEY_LEFT:
		m_ObjectManager->goLeft(playerIDX);
		break;
	case GLUT_KEY_RIGHT:
		m_ObjectManager->goRight(playerIDX);
		break;
	case GLUT_KEY_UP:
		m_ObjectManager->goUp(playerIDX);
		break;
	case GLUT_KEY_DOWN:
		m_ObjectManager->goDown(playerIDX);
		break;
	}
	glutPostRedisplay();
}