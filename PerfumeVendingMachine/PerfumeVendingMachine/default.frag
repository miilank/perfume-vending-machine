#version 330 core

in vec2 texCoord; // where in the image this pixel is
in vec2 localPos; // where in the square this pixel is, -0.5 to 0.5 (center is 0,0)

out vec4 FragColor;

uniform sampler2D tex0;   // the texture bound to unit 0
uniform int uUseTexture;  // 1 = draw an image (drawTexture), 0 = draw one color (drawColor)
uniform int uShape;       // 0 = rectangle, 1 = circle, 2 = soft round glow
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

	// Distance from the center of the square, 0 in the middle and 1 at the edge,
	// used to cut a circle out of the square or make a glow that fades towards the edge
	float d = length(localPos) * 2.0;
	if (uShape == 1)
		color.a *= 1.0 - smoothstep(0.92, 1.0, d);
	else if (uShape == 2)
		color.a *= 1.0 - smoothstep(0.0, 1.0, d);

	color.a *= uAlpha;
	// Fully transparent pixels are thrown away instead of being blended for nothing
	if (color.a < 0.003)
		discard;

	FragColor = color;
}
