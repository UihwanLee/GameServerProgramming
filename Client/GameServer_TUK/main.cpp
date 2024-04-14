#include "pch.h"
#include "ObjectManager.h"

#include <WS2tcpip.h>
#pragma comment (lib, "WS2_32.LIB")

constexpr short PORT = 4000;
constexpr int BUFSIZE = 256;

// 서버 
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

// 클라이언트 행동 함수
void check_packet();
void create_player();
void move_player();
void create_other_player();

void print_error(const char* msg, int err_no);

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

// 카메라
mat4 camera = mat4(1.0f);

// 삼각형 그리기 함수
void drawView();
void drawProjection();
void drawObjects(int idx);

// 오브젝트 리스트
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
	// 30 밀리초 마다 CALLBACK 호출 및 서버 recv 함수 호출
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
	std::wcout << L": 에러 : " << msg_buf;
	while (true);
	LocalFree(msg_buf);
}

void CALLBACK recv_callback(DWORD err, DWORD recv_size,
	LPWSAOVERLAPPED pwsaover, DWORD sendflag)
{
	if (0 != err) {
		print_error("WSARecv", WSAGetLastError());
	}

	// 서버에서 받은 데이터로 수행하는 함수 판단
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

	//std::cout << "[Client] recevicepacket tpye: " << receivedPacket.type << "번" << std::endl;
}

GLvoid initServer()
{
	// Overlapped I/O callback으로 서버 연결

	std::wcout.imbue(std::locale("korean"));

	// window 네트워크 프로그래밍 시 옛날에 만든 프로그램과의 호환성을 위해 필요
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 0), &WSAData);

	// SOCKET 생성
	server_s = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0, WSA_FLAG_OVERLAPPED);

	// SOCK ADDR 생성
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

	std::cout << "[Client] 클라이언트 서버 접속 성공" << std::endl;
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

	// 서버에 플레이어 생성 요청
	send_move_packet(0);

	// 카메라 세팅
	camera = glm::translate(camera, glm::vec3(0.0f, 0.0f, -10.0f));
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

void check_packet()
{
	// 서버에서 받아온 packet의 type을 확인하여 적절한 함수를 실행
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
	// 초기 플레이어를 생성한다.
	m_ObjectManager->setCurrentIDX(receivedPacket.playerPosIDX);
	playerID = m_ObjectManager->creatPlayer(&idx);

	// 서버로부터 받은 서버 아이디를 세팅한다
	m_ObjectManager->setServerID(playerID);

	std::cout << "[Client" << m_ObjectManager->getServerID() << "]: 서버로 부터 요청받아 자신의 플레이 말 생성!" << std::endl;
}

void create_other_player()
{
	std::cout << "[Client" << m_ObjectManager->getServerID() << "]: 서버로 부터 요청받아 다른 플레이 말 생성!" << std::endl;

	// 자신의 플레이어를 생성하기 전 서버에 저장되어 있는 다른 플레이어 생성
	m_ObjectManager->setCurrentIDX(receivedPacket.playerPosIDX);
	playerID = m_ObjectManager->creatPlayer(&idx);
	m_ObjectManager->setPosition(playerID, receivedPacket.pos);
}

void move_player()
{
	// 플레이어의 말을 움직이는 함수

	if (receivedPacket.pos == glm::vec3(0.0f, 0.0f, 0.0f))
	{
		std::cout << "ERROR: POS가 0,0,0 임!" << std::endl;
		return;
	}

	cout << receivedPacket.serverID << "번 IDX 말 이동" << endl;

	// 서버에서 받은 Position으로 말 이동
	m_ObjectManager->setPosition(receivedPacket.serverID, receivedPacket.pos);
	m_ObjectManager->setCurrentIDX(receivedPacket.playerPosIDX);
}

void specialKeyBoard(int key, int x, int y)
{
	// 스페설 키보드 입력 처리
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