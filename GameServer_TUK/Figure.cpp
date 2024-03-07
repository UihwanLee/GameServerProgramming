#include "Figure.h"

GLfloat Figure::RectVertexs[12] = {
	-0.1f, 0.1f, 0.0f,
	-0.1f, -0.1f, 0.0f,
	0.1f, -0.1f, 0.0f,
	0.1f, 0.1f, 0.0f
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

vector<vec3> Figure::Boards = {
	vec3(-0.7f, 0.7f, 0.0f), vec3(-0.5f, 0.7f, 0.0f), vec3(-0.3f, 0.7f, 0.0f), vec3(-0.1f, 0.7f, 0.0f), vec3(0.1f, 0.7f, 0.0f), vec3(0.3f, 0.7f, 0.0f), vec3(0.5f, 0.7f, 0.0f), vec3(0.7f, 0.7f, 0.0f),
	vec3(-0.7f, 0.5f, 0.0f), vec3(-0.5f, 0.5f, 0.0f), vec3(-0.3f, 0.5f, 0.0f), vec3(-0.1f, 0.5f, 0.0f), vec3(0.1f, 0.5f, 0.0f), vec3(0.3f, 0.5f, 0.0f), vec3(0.5f, 0.5f, 0.0f), vec3(0.7f, 0.5f, 0.0f),
	vec3(-0.7f, 0.3f, 0.0f), vec3(-0.5f, 0.3f, 0.0f), vec3(-0.3f, 0.3f, 0.0f), vec3(-0.1f, 0.3f, 0.0f), vec3(0.1f, 0.3f, 0.0f), vec3(0.3f, 0.3f, 0.0f), vec3(0.5f, 0.3f, 0.0f), vec3(0.7f, 0.3f, 0.0f),
	vec3(-0.7f, 0.1f, 0.0f), vec3(-0.5f, 0.1f, 0.0f), vec3(-0.3f, 0.1f, 0.0f), vec3(-0.1f, 0.1f, 0.0f), vec3(0.1f, 0.1f, 0.0f), vec3(0.3f, 0.1f, 0.0f), vec3(0.5f, 0.1f, 0.0f), vec3(0.7f, 0.1f, 0.0f),
	vec3(-0.7f, -0.1f, 0.0f), vec3(-0.5f, -0.1f, 0.0f), vec3(-0.3f, -0.1f, 0.0f), vec3(-0.1f, -0.1f, 0.0f), vec3(0.1f, -0.1f, 0.0f), vec3(0.3f, -0.1f, 0.0f), vec3(0.5f, -0.1f, 0.0f), vec3(0.7f, -0.1f, 0.0f),
	vec3(-0.7f, -0.3f, 0.0f), vec3(-0.5f, -0.3f, 0.0f), vec3(-0.3f, -0.3f, 0.0f), vec3(-0.1f, -0.3f, 0.0f), vec3(0.1f, -0.3f, 0.0f), vec3(0.3f, -0.3f, 0.0f), vec3(0.5f, -0.3f, 0.0f), vec3(0.7f, -0.3f, 0.0f),
	vec3(-0.7f, -0.5f, 0.0f), vec3(-0.5f, -0.5f, 0.0f), vec3(-0.3f, -0.5f, 0.0f), vec3(-0.1f, -0.5f, 0.0f), vec3(0.1f, -0.5f, 0.0f), vec3(0.3f, -0.5f, 0.0f), vec3(0.5f, -0.5f, 0.0f), vec3(0.7f, -0.5f, 0.0f),
	vec3(-0.7f, -0.7f, 0.0f), vec3(-0.5f, -0.7f, 0.0f), vec3(-0.3f, -0.7f, 0.0f), vec3(-0.1f, -0.7f, 0.0f), vec3(0.1f, -0.7f, 0.0f), vec3(0.3f, -0.7f, 0.0f), vec3(0.5f, -0.7f, 0.0f), vec3(0.7f, -0.7f, 0.0f),
};

highp_vec3 Figure::boardColorType1 = highp_vec3(254.0f / 255.0f, 206.0f / 255.0f, 158.0f / 255.0f);
highp_vec3 Figure::boardColorType2 = highp_vec3(211.0f / 255.0f, 138.0f / 255.0f, 69.0f / 255.0f);