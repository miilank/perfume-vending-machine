#version 330 core

layout (location = 0) in vec2 aPos; // corner of the 1x1 square, from -0.5 to 0.5
layout (location = 1) in vec2 aTex; // which corner of the image belongs to this corner, 0 to 1

// Passed on to the fragment shader, interpolated across the square
out vec2 texCoord;
out vec2 localPos;

uniform vec2 uScreen; // screen size in pixels
uniform vec2 uPos;    // center of the element in pixels
uniform vec2 uSize;   // width and height of the element in pixels
uniform vec4 uUVRect; // part of the image to use: start (xy) and size (zw); whole image is 0,0,1,1

void main()
{
	// Main thinks in pixels, so the square is scaled to the element size and moved to its center
	vec2 pixel = uPos + aPos * uSize;
	// OpenGL needs the screen to go from -1 to 1, so pixels are converted:
	// 0 becomes -1, the middle becomes 0, the full width becomes 1
	vec2 ndc = pixel / uScreen * 2.0 - 1.0;
	gl_Position = vec4(ndc, 0.0, 1.0);

	// Normally the whole image; for a digit only its cell in the digit atlas
	texCoord = uUVRect.xy + aTex * uUVRect.zw;
	localPos = aPos;
}