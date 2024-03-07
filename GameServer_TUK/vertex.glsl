#version 330 core

layout(location=0) in vec3 vPos;
layout(location=1) in vec3 vColor;

out vec3 outColor;

uniform mat4 modelTransform;
uniform mat4 viewTransform;
uniform mat4 projectionTransform;

void main()
{
	gl_Position = projectionTransform * viewTransform * modelTransform * vec4(vPos,1.0);
	outColor=vColor;
}