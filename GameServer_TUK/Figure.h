#pragma once

#include "pch.h"

class Figure
{
public:
	static GLfloat board[8];

public:
	// ü����
	static GLfloat	RectVertexs[12];
	static GLfloat	RectColors[12];
	static GLint	RectIndecies[6];

	static GLfloat	PlayerVertex[12];
	static GLfloat	PlayerVertex2[12];
	static GLfloat	PlayerVertex3[12];

	// ü���� Position
	static vector<vec3> Boards;

	static highp_vec3 boardColorType1;
	static highp_vec3 boardColorType2;
};