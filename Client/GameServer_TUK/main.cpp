#include "pch.h"
#include "ObjectManager.h"

#include <WS2tcpip.h>
#pragma comment (lib, "WS2_32.LIB")

#include <SFML/Network.hpp>

#include "..\..\Server\GamerServer_Server\protocol.h"

sf::TcpSocket s_socket;

constexpr short PORT = 4000;
constexpr int BUFSIZE = 256;

// 서버 
GLvoid initServer();
GLvoid client(int argc, char** argv);
constexpr char SERVER_ADDR[] = "127.0.0.1";

WSABUF wsabuf;
WSAOVERLAPPED wsaover;
bool bshutdown = false;

int g_pos_x;
int g_pos_y;
int g_myid;

// 오브젝트 리스트
ObjectManager* m_ObjectManager = new ObjectManager();

// 카메라
mat4 camera = mat4(1.0f);

void print_error(const char* msg, int err_no);

void ProcessPacket(char* ptr)
{
	static bool first_time = true;
	switch (ptr[1])
	{
	case SC_LOGIN_INFO:
	{
		SC_LOGIN_INFO_PACKET* packet = reinterpret_cast<SC_LOGIN_INFO_PACKET*>(ptr);
		m_ObjectManager->m_players[packet->id] = m_ObjectManager->creatPlayer();
		m_ObjectManager->setPlayerPosition(packet->id, packet->x, packet->y);
		camera = glm::translate(camera, glm::vec3(packet->cx, packet->cy, 0.0f));

		g_myid = packet->id;
		g_pos_x = packet->x;
		g_pos_y = packet->cy;
		//avatar.show();
	}
	break;

	case SC_ADD_PLAYER:
	{
		SC_ADD_PLAYER_PACKET* my_packet = reinterpret_cast<SC_ADD_PLAYER_PACKET*>(ptr);
		int id = my_packet->id;

		if (id == g_myid) {
			m_ObjectManager->setPlayerPosition(g_myid, my_packet->x, my_packet->y);
		}
		else if (id < MAX_USER) {
			m_ObjectManager->m_players[id] = m_ObjectManager->creatPlayer();
			m_ObjectManager->setPlayerPosition(id, my_packet->x, my_packet->y);
		}
		else {
		}
		break;
	}
	case SC_MOVE_PLAYER:
	{
		SC_MOVE_PLAYER_PACKET* my_packet = reinterpret_cast<SC_MOVE_PLAYER_PACKET*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			camera = glm::translate(camera, glm::vec3(my_packet->cx, my_packet->cy, 0.0f));
			m_ObjectManager->setPlayerPosition(g_myid, my_packet->x, my_packet->y);
			//avatar.move(my_packet->x, my_packet->y);
			g_pos_x = my_packet->x;
			g_pos_y = my_packet->cy;
		}
		else if (other_id < MAX_USER) {
			m_ObjectManager->setPlayerPosition(other_id, my_packet->x, my_packet->y);
			//players[other_id].move(my_packet->x, my_packet->y);
		}
		else {
			//npc[other_id - NPC_START].x = my_packet->x;
			//npc[other_id - NPC_START].y = my_packet->y;
		}
		break;
	}

	case SC_REMOVE_PLAYER:
	{
		SC_REMOVE_PLAYER_PACKET* my_packet = reinterpret_cast<SC_REMOVE_PLAYER_PACKET*>(ptr);
		int other_id = my_packet->id;
		if (other_id == g_myid) {
			//avatar.hide();
		}
		else if (other_id < MAX_USER) {
			m_ObjectManager->m_players.erase(other_id);
			//players.erase(other_id);
		}
		else {
			//		npc[other_id - NPC_START].attr &= ~BOB_ATTR_VISIBLE;
		}
		break;
	}
	default:
		printf("Unknown PACKET type [%d]\n", ptr[1]);
	}
}

SOCKET server_s;
char buf[BUFSIZE];

// 콜벡 함수
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

GLchar* vertexSource, * fragmentSource; //--- 소스코드 저장 변수
GLuint vertexShader, fragmentShader; //세이더 객체
GLuint shaderProgramID; // 세이더 프로그램

GLint createShader(const char* file, int type);
GLvoid createShaderProgram();

GLvoid initBuffer();
GLvoid initObjects();

// INDEX
int idx = -1;
int playerID = -1;

// 삼각형 그리기 함수
void drawView();
void drawProjection();
void drawObjects(int idx);
void drawPlayers(int idx);

void client_update();

void main(int argc, char** argv)
{
	initServer();
	client(argc, argv);
}

GLvoid update(int value)
{
	// 통신 업데이트
	client_update();
	
	glutPostRedisplay();
	glutTimerFunc(30, update, 1);
}

void send_packet(void* packet)
{
	unsigned char* p = reinterpret_cast<unsigned char*>(packet);
	size_t sent = 0;
	s_socket.send(packet, p[0], sent);
}

void print_error(const char* msg, int err_no)
{
	WCHAR* msg_buf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_no,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPWSTR>(&msg_buf), 0, NULL);

	std::cout << msg;
	std::wcout << L": 에러 : " << msg_buf;
	while (true);
	LocalFree(msg_buf);
}

void process_data(char* net_buf, size_t io_byte)
{
	char* ptr = net_buf;
	static size_t in_packet_size = 0;
	static size_t saved_packet_size = 0;
	static char packet_buffer[BUF_SIZE];

	while (0 != io_byte) {
		if (0 == in_packet_size) in_packet_size = ptr[0];
		if (io_byte + saved_packet_size >= in_packet_size) {
			memcpy(packet_buffer + saved_packet_size, ptr, in_packet_size - saved_packet_size);
			ProcessPacket(packet_buffer);
			ptr += in_packet_size - saved_packet_size;
			io_byte -= in_packet_size - saved_packet_size;
			in_packet_size = 0;
			saved_packet_size = 0;
		}
		else {
			memcpy(packet_buffer + saved_packet_size, ptr, io_byte);
			saved_packet_size += io_byte;
			io_byte = 0;
		}
	}
}

GLvoid client_update()
{
	char net_buf[BUF_SIZE];
	size_t	received;

	auto recv_result = s_socket.receive(net_buf, BUF_SIZE, received);
	if (recv_result == sf::Socket::Error)
	{
		wcout << L"Recv 에러!";
		exit(-1);
	}
	if (recv_result == sf::Socket::Disconnected) {
		wcout << L"Disconnected\n";
		exit(-1);
	}
	if (recv_result != sf::Socket::NotReady)
		if (received > 0) process_data(net_buf, received);
}

GLvoid initServer()
{
	wcout.imbue(locale("korean"));
	sf::Socket::Status status = s_socket.connect("127.0.0.1", PORT_NUM);
	s_socket.setBlocking(false);

	if (status != sf::Socket::Done) {
		wcout << L"서버와 연결할 수 없습니다.\n";
		exit(-1);
	}

	// 서버에 플레이어 로그인 요청
	CS_LOGIN_PACKET p;
	p.size = sizeof(p);
	p.type = CS_LOGIN;

	string player_name{ "P" };
	player_name += to_string(GetCurrentProcessId());

	strcpy_s(p.name, player_name.c_str());
	send_packet(&p);
}

GLvoid client(int argc, char** argv)
{
	// 윈도우 생성
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(WIDTH, HEIGHT);
	glutCreateWindow("GameServerTUK");

	//--- GLEW 초기화하기
	glewExperimental = GL_TRUE;
	glewInit();

	initBuffer();
	createShaderProgram();

	glutDisplayFunc(render);

	// Update
	glutTimerFunc(10, update, 1);

	// 키보드
	glutKeyboardFunc(keyBoard);
	glutSpecialFunc(specialKeyBoard);

	glutMainLoop();
}

GLvoid reshape(int w, int h)
{
	// 뷰포트 기본 WIDTH HEIGHT로 설정
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

	// 체스판 생성
	m_ObjectManager->creatBoard(&idx);

	// 카메라 세팅
	camera = glm::translate(camera, glm::vec3(0.0f, 0.0f, -16.0f));
}

char* getBuf(const char* file)
{
	FILE* fptr;
	long length;
	char* buf;

	// 파일 읽기 형식으로 열기
	fopen_s(&fptr, file, "rb");

	// 예외처리
	if (!fptr) return NULL;

	// 파일 buf로 변환
	fseek(fptr, 0, SEEK_END);
	length = ftell(fptr);
	buf = (char*)malloc(length + 1);
	fseek(fptr, 0, SEEK_SET);
	fread(buf, length, 1, fptr);

	// 파일 종료
	fclose(fptr);

	buf[length] = 0;
	return buf;
}

GLint createShader(const char* file, int type)
{
	// glsl 파일 읽기
	GLchar* source = getBuf(file);

	// 객체 생성
	GLint shader = glCreateShader(type);
	glShaderSource(shader, 1, (const GLchar**)&source, 0);
	glCompileShader(shader);

	// 컴파일 에러 체크
	GLint result;
	GLchar errorLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);

	if (!result)
	{
		glGetShaderInfoLog(shader, 512, NULL, errorLog);
		// std::cerr << "ERROR: 컴파일 실패\n" << errorLog << std::endl;
		return 0;
	}

	return shader;
}

void createShaderProgram()
{
	vertexShader = createShader("vertex.glsl", GL_VERTEX_SHADER);
	fragmentShader = createShader("fragment.glsl", GL_FRAGMENT_SHADER);

	// 세이더 프로그램 생성
	shaderProgramID = glCreateProgram();
	glAttachShader(shaderProgramID, vertexShader);
	glAttachShader(shaderProgramID, fragmentShader);
	glLinkProgram(shaderProgramID);

	// 세이더 삭제
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// 세이더 프로그램 사용
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
			int min_x = (g_pos_x - 8 < 0) ? 0 : g_pos_x - 8;
			int max_x = (g_pos_x + 8 >= W_WIDTH) ? W_WIDTH : g_pos_x + 8;

			int min_y = (g_pos_y - 8 < 0) ? 0 : g_pos_y - 8;
			int max_y = (g_pos_y + 8 >= W_HEIGHT) ? W_HEIGHT : g_pos_y + 8;

			if (i%W_WIDTH >= min_x && i % W_WIDTH <= max_x)
			{
				drawObjects(i);
			}
		}
	}
	for (auto& player : m_ObjectManager->m_players)
	{
		if (m_ObjectManager->m_players[player.first]->isActive())
		{
			drawPlayers(player.first);
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

	// 원근 투영
	float fov = 45.0f; // 시야각 (Field of View)
	float aspectRatio = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT); // 화면의 가로 세로 비율
	float zNear = 0.1f; // 가까운 클리핑 평면
	float zFar = 50.0f; // 먼 클리핑 평면
	projection = glm::perspective(glm::radians(fov), aspectRatio, zNear, zFar); //--- 투영 공간 설정: (뷰잉각도, 종횡비, 가까운거리, 먼거리)

	glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, &projection[0][0]);		// 투영변환
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

void drawPlayers(int idx)
{
	// Model
	unsigned int modelLocation = glGetUniformLocation(shaderProgramID, "modelTransform");
	m_ObjectManager->m_players[idx]->m_model = m_ObjectManager->transformModelPlayer(idx);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(m_ObjectManager->m_players[idx]->m_model));

	// Position / Color
	glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, m_ObjectManager->m_players[idx]->m_pos.size() * sizeof(GLfloat), &m_ObjectManager->m_players[idx]->m_pos[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_ObjectManager->m_players[idx]->m_index.size() * sizeof(GLfloat), &m_ObjectManager->m_players[idx]->m_index[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);

	glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
	glBufferData(GL_ARRAY_BUFFER, m_ObjectManager->m_players[idx]->m_col.size() * sizeof(GLfloat), &m_ObjectManager->m_players[idx]->m_col[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), 0);

	glDrawElements(GL_TRIANGLES, m_ObjectManager->m_players[idx]->m_index.size(), GL_UNSIGNED_INT, 0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

void keyBoard(unsigned char key, int x, int y)
{
	// 키보드 입력
	switch (key)
	{
	case 'Q':
	case 'q':
		// 클라이언트 서버 접속 종료
		std::cout << "[Client] 클라이언트 서버 접속 종료!" << std::endl;
		glutLeaveMainLoop();
		closesocket(server_s);
		WSACleanup();
		break;
	default:
		break;
	}
}

void specialKeyBoard(int key, int x, int y)
{
	int direction = -1;
	// 스페설 키보드 입력 처리
	switch (key) {
	case GLUT_KEY_LEFT:
		direction = 2;
		break;
	case GLUT_KEY_RIGHT:
		direction = 3;
		break;
	case GLUT_KEY_UP:
		direction = 0;
		break;
	case GLUT_KEY_DOWN:
		direction = 1;
		break;
	}
	if (-1 != direction) {
		CS_MOVE_PACKET p;
		p.size = sizeof(p);
		p.type = CS_MOVE;
		p.direction = direction;
		send_packet(&p);
	}
	glutPostRedisplay();
}