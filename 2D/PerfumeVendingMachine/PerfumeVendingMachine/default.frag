#version 330 core

in vec2 texCoord; // where in the image this pixel is

out vec4 FragColor;

uniform sampler2D tex0;   // the texture bound to unit 0
uniform int uUseTexture;  // 1 = draw an image (drawTexture), 0 = draw one color (drawColor)
uniform vec4 uColor;      // the color for drawColor
uniform vec3 uTint;       // multiplies the image color: 1,1,1 keeps it, 0.6 darkens, a color colors it
uniform float uAlpha;     // transparency of the whole element

void main()
{
	vec4 color;
	if (uUseTexture == 1)
		color = texture(tex0, texCoord) * vec4(uTint, 1.0);
	else
		color = uColor;

	color.a *= uAlpha;
	// Fully transparent pixels are thrown away instead of being blended for nothing
	if (color.a < 0.003)
		discard;

	FragColor = color;
}
