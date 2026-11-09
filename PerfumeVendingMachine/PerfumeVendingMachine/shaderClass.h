#ifndef SHADER_CLASS_H
#define SHADER_CLASS_H

#include <GL/glew.h>

#include <string>

std::string get_file_contents(const char* filename);

class Shader
{
public:
	GLuint ID;

	Shader(const char* vertexFile, const char* fragmentFile);

	void Activate();
	void Delete();

	void setInt(const char* name, int value);
	void setFloat(const char* name, float value);
	void setVec2(const char* name, float x, float y);
	void setVec3(const char* name, float x, float y, float z);
	void setVec4(const char* name, float x, float y, float z, float w);

private:
	void compileErrors(GLuint shader, const char* type);
};

#endif