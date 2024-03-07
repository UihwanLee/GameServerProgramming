#pragma once

#include "pch.h"

class Figure
{
public:
	// 체스판
	static GLfloat	RectVertexs[12];
	static GLfloat	RectColors[12];
	static GLint	RectIndecies[6];

	// 체스판 Position
	static vector<vec3> Boards;

	static highp_vec3 boardColorType1;
	static highp_vec3 boardColorType2;
};