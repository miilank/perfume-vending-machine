#version 330 core

in vec2 texCoord;
in vec2 localPos;

out vec4 FragColor;

uniform sampler2D tex0;
uniform int uUseTexture;
uniform int uShape;
uniform vec4 uColor;
uniform vec3 uTint;
uniform float uAlpha;

void main()
{
	vec4 color;
	if (uUseTexture == 1)
		color = texture(tex0, texCoord) * vec4(uTint, 1.0);
	else
		color = uColor;

	float d = length(localPos) * 2.0;
	if (uShape == 1)
		color.a *= 1.0 - smoothstep(0.92, 1.0, d);
	else if (uShape == 2)
		color.a *= 1.0 - smoothstep(0.0, 1.0, d);

	color.a *= uAlpha;
	if (color.a < 0.003)
		discard;

	FragColor = color;
}