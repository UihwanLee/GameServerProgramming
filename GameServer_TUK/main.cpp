#include "pch.h"
#include "ObjectManager.h"

// �ݺ� �Լ�
GLvoid Render(GLvoid);
GLvoid Reshape(int w, int h);
GLvoid SpecialKeyBoard(int key, int x, int y);

// Texture
GLuint texture;

// VAO, VBO
GLuint VAO, VBO[2], EBO;

GLchar* vertexSource, * fragmentSource; //--- �ҽ��ڵ� ���� ����
GLuint vertexShader, fragmentShader; //���̴� ��ü
GLuint shaderProgramID; // ���̴� ���α׷�

GLint CreateShader(const char* file, int type);
GLvoid CreateShaderProgram();

GLvoid InitBuffer();
GLvoid InitObjects();

// INDEX
int idx = -1;
int playerIDX = -1;

// ī�޶�
mat4 camera = mat4(1.0f);

// �ﰢ�� �׸��� �Լ�
void DrawView();
void DrawProjection();
void DrawObjects(int idx);

// ������Ʈ ����Ʈ
ObjectManager* m_ObjectManager = new ObjectManager();

void main(int argc, char** argv)
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

	InitBuffer();
	CreateShaderProgram();

	glutDisplayFunc(Render);

	// Ű����
	glutSpecialFunc(SpecialKeyBoard);

	glutMainLoop();
}

GLvoid Reshape(int w, int h)
{
	// ����Ʈ �⺻ WIDTH HEIGHT�� ����
	glViewport(0, 0, w, h);
}

GLvoid InitBuffer()
{
	glGenVertexArrays(1, &VAO);

	glGenBuffers(2, VBO);
	glGenBuffers(1, &EBO);

	InitObjects();
}

GLvoid InitObjects()
{
	m_ObjectManager->Reset();

	// ü���� ����
	m_ObjectManager->CreatBoard(&idx);

	// �÷��̾� ����
	playerIDX = m_ObjectManager->CreatPlayer(&idx);

	// ī�޶� ����
	camera = glm::translate(camera, glm::vec3(0.0f, 0.0f, -10.0f));
}

char* GetBuf(const char* file)
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

GLint CreateShader(const char* file, int type)
{
	// glsl ���� �б�
	GLchar* source = GetBuf(file);

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

void CreateShaderProgram()
{
	vertexShader = CreateShader("vertex.glsl", GL_VERTEX_SHADER);
	fragmentShader = CreateShader("fragment.glsl", GL_FRAGMENT_SHADER);

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

GLvoid Render()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	DrawView();
	DrawProjection();
	for (int i = 0; i < m_ObjectManager->m_ObjectList.size(); i++)
	{
		if (m_ObjectManager->m_ObjectList[i]->IsActive())
		{
			DrawObjects(i);
		}
	}

	glutSwapBuffers();
}

void DrawView()
{
	unsigned int viewLocation = glGetUniformLocation(shaderProgramID, "viewTransform");

	glUniformMatrix4fv(viewLocation, 1, GL_FALSE,glm::value_ptr(camera));
}

void DrawProjection()
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

void DrawObjects(int idx)	
{
	// Model
	unsigned int modelLocation = glGetUniformLocation(shaderProgramID, "modelTransform");
	m_ObjectManager->m_ObjectList[idx]->m_model = m_ObjectManager->TransformModel(idx);
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

void SpecialKeyBoard(int key, int x, int y)
{
	// ���伳 Ű���� �Է� ó��
	switch (key) {
	case GLUT_KEY_LEFT:
		m_ObjectManager->GoLeft(playerIDX);
		break;
	case GLUT_KEY_RIGHT:
		m_ObjectManager->GoRight(playerIDX);
		break;
	case GLUT_KEY_UP:
		m_ObjectManager->GoUp(playerIDX);
		break;
	case GLUT_KEY_DOWN:
		m_ObjectManager->GoDown(playerIDX);
		break;
	}
	glutPostRedisplay();
}