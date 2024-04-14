#include "pch.h"
#include "ObjectManager.h"

#include <WS2tcpip.h>
#pragma comment (lib, "WS2_32.LIB")

constexpr short PORT = 4000;
constexpr int BUFSIZE = 256;

// ���� 
GLvoid initServer();
GLvoid client(int argc, char** argv);
constexpr char SERVER_ADDR[] = "127.0.0.1";

WSABUF wsabuf;
WSAOVERLAPPED wsaover;
bool bshutdown = false;

void CALLBACK send_callback(DWORD error, DWORD sent_size,
	LPWSAOVERLAPPED pwsaover, DWORD sendflag);

void CALLBACK recv_callback(DWORD error, DWORD recv_size,
	LPWSAOVERLAPPED pwsaover, DWORD sendflag);

// Ŭ���̾�Ʈ �ൿ �Լ�
void check_packet();
void create_player();
void move_player();
void create_other_player();

void print_error(const char* msg, int err_no);

SOCKET server_s;
char buf[BUFSIZE];

// �ݺ� �Լ�
GLvoid render(GLvoid);
GLvoid reshape(int w, int h);
GLvoid keyBoard(unsigned char key, int x, int y);
GLvoid specialKeyBoard(int key, int x, int y);
GLvoid update(int value);

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
int playerID = -1;

// ī�޶�
mat4 camera = mat4(1.0f);

// �ﰢ�� �׸��� �Լ�
void drawView();
void drawProjection();
void drawObjects(int idx);

// ������Ʈ ����Ʈ
ObjectManager* m_ObjectManager = new ObjectManager();

#pragma pack (push, 1)
struct packet {
	short		size;
	int			type;
	int			serverID;
	int			playerPosIDX;
	int			currentPlayerCount;
	glm::vec3	pos;
};
#pragma pack (pop)

void main(int argc, char** argv)
{
	initServer();
	client(argc, argv);
}

void send_move_packet(int type);

packet receivedPacket;

void do_recv()
{
	wsabuf.len = sizeof(packet);
	wsabuf.buf = reinterpret_cast<CHAR*>(&receivedPacket);
	DWORD recv_size;
	DWORD recv_flag = 0;
	ZeroMemory(&wsaover, sizeof(wsaover));
	int res = WSARecv(server_s, &wsabuf, 1, &recv_size, &recv_flag, &wsaover, recv_callback);

	if (0 != res) {
		int err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no)
			print_error("WSARecv", WSAGetLastError());
	}
}

GLvoid update(int value)
{
	// 30 �и��� ���� CALLBACK ȣ�� �� ���� recv �Լ� ȣ��
	SleepEx(0, TRUE);
	do_recv();
	glutPostRedisplay();
	glutTimerFunc(30, update, 1);
}

void send_move_packet(int type)
{
	receivedPacket.size = sizeof(packet);
	receivedPacket.type = type;
	receivedPacket.serverID = m_ObjectManager->getServerID();
	receivedPacket.playerPosIDX = m_ObjectManager->getCurrentIDX();
	receivedPacket.currentPlayerCount = 0;
	receivedPacket.pos = glm::vec3(0.0f, 0.0f, 0.0f);

	wsabuf.len = sizeof(packet);
	wsabuf.buf = reinterpret_cast<CHAR*>(&receivedPacket);
	if (wsabuf.len == 1) {
		bshutdown = true;
		return;
	}
	ZeroMemory(&wsaover, sizeof(wsaover));
	WSASend(server_s, &wsabuf, 1, nullptr, 0, &wsaover, send_callback);
}

void print_error(const char* msg, int err_no)
{
	WCHAR* msg_buf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPWSTR>(&msg_buf), 0, NULL);

	std::cout << msg;
	std::wcout << L": ���� : " << msg_buf;
	while (true);
	LocalFree(msg_buf);
}

void CALLBACK recv_callback(DWORD err, DWORD recv_size,
	LPWSAOVERLAPPED pwsaover, DWORD sendflag)
{
	if (0 != err) {
		print_error("WSARecv", WSAGetLastError());
	}

	// �������� ���� �����ͷ� �����ϴ� �Լ� �Ǵ�
	check_packet();

	glutPostRedisplay();
}

void CALLBACK send_callback(DWORD err, DWORD sent_size,
	LPWSAOVERLAPPED pwsaover, DWORD sendflag)
{
	wsabuf.len = sizeof(packet);
	DWORD recv_flag = 0;
	ZeroMemory(pwsaover, sizeof(*pwsaover));
	WSARecv(server_s, &wsabuf, 1, nullptr, &recv_flag, pwsaover, recv_callback);

	//std::cout << "[Client] recevicepacket tpye: " << receivedPacket.type << "��" << std::endl;
}

GLvoid initServer()
{
	// Overlapped I/O callback���� ���� ����

	std::wcout.imbue(std::locale("korean"));

	// window ��Ʈ��ũ ���α׷��� �� ������ ���� ���α׷����� ȣȯ���� ���� �ʿ�
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 0), &WSAData);

	// SOCKET ����
	server_s = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);

	// SOCK ADDR ����
	SOCKADDR_IN server_a;
	server_a.sin_family = AF_INET;
	server_a.sin_port = htons(PORT);
	inet_pton(AF_INET, SERVER_ADDR, &server_a.sin_addr);

	if (connect(server_s, reinterpret_cast<sockaddr*>(&server_a), sizeof(server_a)) == SOCKET_ERROR)
	{
		std::cerr << "Failed to connect to server.\n";
		closesocket(server_s);
		WSACleanup();
		return;
	}

	std::cout << "[Client] Ŭ���̾�Ʈ ���� ���� ����" << std::endl;
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

	// Update
	glutTimerFunc(10, update, 1);

	// Ű����
	glutKeyboardFunc(keyBoard);
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

	// ������ �÷��̾� ���� ��û
	send_move_packet(0);

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

	glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(camera));
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

void keyBoard(unsigned char key, int x, int y)
{
	// Ű���� �Է�
	switch (key)
	{
	case 'Q':
	case 'q':
		// Ŭ���̾�Ʈ ���� ���� ����
		std::cout << "[Client] Ŭ���̾�Ʈ ���� ���� ����!" << std::endl;
		glutLeaveMainLoop();
		closesocket(server_s);
		WSACleanup();
		break;
	default:
		break;
	}
}

void check_packet()
{
	// �������� �޾ƿ� packet�� type�� Ȯ���Ͽ� ������ �Լ��� ����
	if (receivedPacket.type == 0)
	{
		create_player();
	}
	else if (receivedPacket.type >= 1 && receivedPacket.type <= 4)
	{
		move_player();
	}
	else if (receivedPacket.type == 5)
	{
		create_other_player();
	}
}

void create_player()
{
	// �ʱ� �÷��̾ �����Ѵ�.
	m_ObjectManager->setCurrentIDX(receivedPacket.playerPosIDX);
	playerID = m_ObjectManager->creatPlayer(&idx);

	// �����κ��� ���� ���� ���̵� �����Ѵ�
	m_ObjectManager->setServerID(playerID);

	std::cout << "[Client" << m_ObjectManager->getServerID() << "]: ������ ���� ��û�޾� �ڽ��� �÷��� �� ����!" << std::endl;
}

void create_other_player()
{
	std::cout << "[Client" << m_ObjectManager->getServerID() << "]: ������ ���� ��û�޾� �ٸ� �÷��� �� ����!" << std::endl;

	// �ڽ��� �÷��̾ �����ϱ� �� ������ ����Ǿ� �ִ� �ٸ� �÷��̾� ����
	m_ObjectManager->setCurrentIDX(receivedPacket.playerPosIDX);
	playerID = m_ObjectManager->creatPlayer(&idx);
	m_ObjectManager->setPosition(playerID, receivedPacket.pos);
}

void move_player()
{
	// �÷��̾��� ���� �����̴� �Լ�

	if (receivedPacket.pos == glm::vec3(0.0f, 0.0f, 0.0f))
	{
		std::cout << "ERROR: POS�� 0,0,0 ��!" << std::endl;
		return;
	}

	cout << receivedPacket.serverID << "�� IDX �� �̵�" << endl;

	// �������� ���� Position���� �� �̵�
	m_ObjectManager->setPosition(receivedPacket.serverID, receivedPacket.pos);
	m_ObjectManager->setCurrentIDX(receivedPacket.playerPosIDX);
}

void specialKeyBoard(int key, int x, int y)
{
	// ���伳 Ű���� �Է� ó��
	switch (key) {
	case GLUT_KEY_LEFT:
		send_move_packet(1);
		break;
	case GLUT_KEY_RIGHT:
		send_move_packet(2);
		break;
	case GLUT_KEY_UP:
		send_move_packet(3);
		break;
	case GLUT_KEY_DOWN:
		send_move_packet(4);
		break;
	}
	glutPostRedisplay();
}