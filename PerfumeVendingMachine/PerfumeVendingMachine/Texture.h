#ifndef TEXTURE_CLASS_H
#define TEXTURE_CLASS_H

#include <GL/glew.h>
#include "shaderClass.h"

class Texture
{
public:
	GLuint ID;
	GLenum type;
	GLenum unit;
	int width;
	int height;

	Texture(const char* image, GLenum texType, GLenum slot);

	void texUnit(Shader& shader, const char* uniform, GLuint unitIndex);
	void Bind();
	void Unbind();
	void Delete();
};

#endif