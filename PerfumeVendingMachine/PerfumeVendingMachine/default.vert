#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTex;

out vec2 texCoord;
out vec2 localPos;

uniform vec2 uScreen;
uniform vec2 uPos;
uniform vec2 uSize;
uniform vec4 uUVRect;

void main()
{
	vec2 pixel = uPos + aPos * uSize;
	vec2 ndc = pixel / uScreen * 2.0 - 1.0;
	gl_Position = vec4(ndc, 0.0, 1.0);

	texCoord = uUVRect.xy + aTex * uUVRect.zw;
	localPos = aPos;
}