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