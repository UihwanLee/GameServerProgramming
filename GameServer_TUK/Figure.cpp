#include "Figure.h"

GLfloat Figure::RectVertexs[12] = {
	-0.5f, 0.5f, 0.0f,
	-0.5f, -0.5f, 0.0f,
	0.5f, -0.5f, 0.0f,
	0.5f, 0.5f, 0.0f
};

GLfloat Figure::RectColors[12] = {
	160.0f / 255.0f, 212.0f / 255.0f, 104.0f / 255.0f,
	160.0f / 255.0f, 212.0f / 255.0f, 104.0f / 255.0f,
	160.0f / 255.0f, 212.0f / 255.0f, 104.0f / 255.0f,
	160.0f / 255.0f, 212.0f / 255.0f, 104.0f / 255.0f
};

GLint Figure::RectIndecies[6] = {
	0, 1, 2,
	0, 2, 3
};

GLfloat Figure::board[8] = { -3.5f, -2.5f, -1.5f, -0.5f, 0.5f, 1.5f, 2.5f, 3.5f };

vector<vec3> Figure::Boards = {
	vec3(Figure::board[0], Figure::board[7], 0.0f), vec3(Figure::board[1], Figure::board[7], 0.0f), vec3(Figure::board[2], Figure::board[7], 0.0f), vec3(Figure::board[3], Figure::board[7], 0.0f), vec3(Figure::board[4], Figure::board[7], 0.0f), vec3(Figure::board[5], Figure::board[7], 0.0f), vec3(Figure::board[6], Figure::board[7], 0.0f), vec3(Figure::board[7], Figure::board[7], 0.0f),
	vec3(Figure::board[0], Figure::board[6], 0.0f), vec3(Figure::board[1], Figure::board[6], 0.0f), vec3(Figure::board[2], Figure::board[6], 0.0f), vec3(Figure::board[3], Figure::board[6], 0.0f), vec3(Figure::board[4], Figure::board[6], 0.0f), vec3(Figure::board[5], Figure::board[6], 0.0f), vec3(Figure::board[6], Figure::board[6], 0.0f), vec3(Figure::board[7], Figure::board[6], 0.0f),
	vec3(Figure::board[0], Figure::board[5], 0.0f), vec3(Figure::board[1], Figure::board[5], 0.0f), vec3(Figure::board[2], Figure::board[5], 0.0f), vec3(Figure::board[3], Figure::board[5], 0.0f), vec3(Figure::board[4], Figure::board[5], 0.0f), vec3(Figure::board[5], Figure::board[5], 0.0f), vec3(Figure::board[6], Figure::board[5], 0.0f), vec3(Figure::board[7], Figure::board[5], 0.0f),
	vec3(Figure::board[0], Figure::board[4], 0.0f), vec3(Figure::board[1], Figure::board[4], 0.0f), vec3(Figure::board[2], Figure::board[4], 0.0f), vec3(Figure::board[3], Figure::board[4], 0.0f), vec3(Figure::board[4], Figure::board[4], 0.0f), vec3(Figure::board[5], Figure::board[4], 0.0f), vec3(Figure::board[6], Figure::board[4], 0.0f), vec3(Figure::board[7], Figure::board[4], 0.0f),
	vec3(Figure::board[0], Figure::board[3], 0.0f), vec3(Figure::board[1], Figure::board[3], 0.0f), vec3(Figure::board[2], Figure::board[3], 0.0f), vec3(Figure::board[3], Figure::board[3], 0.0f), vec3(Figure::board[4], Figure::board[3], 0.0f), vec3(Figure::board[5], Figure::board[3], 0.0f), vec3(Figure::board[6], Figure::board[3], 0.0f), vec3(Figure::board[7], Figure::board[3], 0.0f),
	vec3(Figure::board[0], Figure::board[2], 0.0f), vec3(Figure::board[1], Figure::board[2], 0.0f), vec3(Figure::board[2], Figure::board[2], 0.0f), vec3(Figure::board[3], Figure::board[2], 0.0f), vec3(Figure::board[4], Figure::board[2], 0.0f), vec3(Figure::board[5], Figure::board[2], 0.0f), vec3(Figure::board[6], Figure::board[2], 0.0f), vec3(Figure::board[7], Figure::board[2], 0.0f),
	vec3(Figure::board[0], Figure::board[1], 0.0f), vec3(Figure::board[1], Figure::board[1], 0.0f), vec3(Figure::board[2], Figure::board[1], 0.0f), vec3(Figure::board[3], Figure::board[1], 0.0f), vec3(Figure::board[4], Figure::board[1], 0.0f), vec3(Figure::board[5], Figure::board[1], 0.0f), vec3(Figure::board[6], Figure::board[1], 0.0f), vec3(Figure::board[7], Figure::board[1], 0.0f),
	vec3(Figure::board[0], Figure::board[0], 0.0f), vec3(Figure::board[1], Figure::board[0], 0.0f), vec3(Figure::board[2], Figure::board[0], 0.0f), vec3(Figure::board[3], Figure::board[0], 0.0f), vec3(Figure::board[4], Figure::board[0], 0.0f), vec3(Figure::board[5], Figure::board[0], 0.0f), vec3(Figure::board[6], Figure::board[0], 0.0f), vec3(Figure::board[7], Figure::board[0], 0.0f),
};

highp_vec3 Figure::boardColorType1 = highp_vec3(254.0f / 255.0f, 206.0f / 255.0f, 158.0f / 255.0f);
highp_vec3 Figure::boardColorType2 = highp_vec3(211.0f / 255.0f, 138.0f / 255.0f, 69.0f / 255.0f);