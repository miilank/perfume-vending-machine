#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

#include "shaderClass.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "Texture.h"

const double TARGET_FPS = 75.0;
const double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;

struct Rect {
	float cx, cy, w, h;
	bool contains(float x, float y) const {
		return x >= cx - w / 2 && x <= cx + w / 2 && y >= cy - h / 2 && y <= cy + h / 2;
	}
};

int screenW = 1920, screenH = 1080;

int endProgram(std::string message) {
	std::cout << message << std::endl;
	glfwTerminate();
	return -1;
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

Shader* gShader = nullptr;

void setCommonUniforms(const Rect& r, float alpha, int shape) {
	gShader->setVec2("uPos", r.cx, r.cy);
	gShader->setVec2("uSize", r.w, r.h);
	gShader->setFloat("uAlpha", alpha);
	gShader->setInt("uShape", shape);
}

void drawTexture(Texture& tex, const Rect& r, float alpha = 1.0f,
	float tr = 1.0f, float tg = 1.0f, float tb = 1.0f,
	float u0 = 0.0f, float v0 = 0.0f, float us = 1.0f, float vs = 1.0f) {
	setCommonUniforms(r, alpha, 0);
	gShader->setInt("uUseTexture", 1);
	gShader->setVec3("uTint", tr, tg, tb);
	gShader->setVec4("uUVRect", u0, v0, us, vs);
	tex.Bind();
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void drawColor(const Rect& r, float red, float green, float blue, float alpha, int shape = 0) {
	setCommonUniforms(r, 1.0f, shape);
	gShader->setInt("uUseTexture", 0);
	gShader->setVec4("uColor", red, green, blue, alpha);
	gShader->setVec4("uUVRect", 0.0f, 0.0f, 1.0f, 1.0f);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

int main()
{
	if (!glfwInit()) {
		std::cout << "Failed to initialize GLFW." << std::endl;
		return -1;
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);
	GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Perfume Vending Machine", monitor, NULL);
	if (window == NULL) return endProgram("Failed to create window.");
	glfwMakeContextCurrent(window);

	glfwSwapInterval(0);
	glfwSetKeyCallback(window, keyboardCallback);

	if (glewInit() != GLEW_OK) return endProgram("Failed to initialize GLEW.");

	glfwGetFramebufferSize(window, &screenW, &screenH);
	glViewport(0, 0, screenW, screenH);

	Shader shaderProgram("default.vert", "default.frag");
	gShader = &shaderProgram;

	GLfloat vertices[] = {
		-0.5f, -0.5f,   0.0f, 0.0f,
		 0.5f, -0.5f,   1.0f, 0.0f,
		 0.5f,  0.5f,   1.0f, 1.0f,
		-0.5f,  0.5f,   0.0f, 1.0f,
	};
	GLuint indices[] = {
		0, 1, 2,
		0, 2, 3,
	};

	VAO VAO1;
	VAO1.Bind();
	VBO VBO1(vertices, sizeof(vertices));
	EBO EBO1(indices, sizeof(indices));
	VAO1.LinkAttrib(VBO1, 0, 2, GL_FLOAT, 4 * sizeof(float), (void*)0);
	VAO1.LinkAttrib(VBO1, 1, 2, GL_FLOAT, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	VAO1.Unbind();
	VBO1.Unbind();
	EBO1.Unbind();

	Texture texBackground("res/background.png", GL_TEXTURE_2D, GL_TEXTURE0);
	Texture texCoin("res/coin.png", GL_TEXTURE_2D, GL_TEXTURE0);

	texBackground.texUnit(shaderProgram, "tex0", 0);
	shaderProgram.Activate();
	shaderProgram.setVec2("uScreen", (float)screenW, (float)screenH);

	glClearColor(0.2f, 0.2f, 0.25f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	while (!glfwWindowShouldClose(window))
	{
		double frameStart = glfwGetTime();

		glClear(GL_COLOR_BUFFER_BIT);
		shaderProgram.Activate();
		VAO1.Bind();

		drawTexture(texBackground, Rect{ screenW / 2.0f, screenH / 2.0f, (float)screenW, (float)screenH });
		float coinSize = screenH * 0.25f;
		drawTexture(texCoin, Rect{ screenW * 0.4f, screenH / 2.0f, coinSize, coinSize });
		drawTexture(texCoin, Rect{ screenW * 0.6f, screenH / 2.0f, coinSize, coinSize }, 0.5f);
		glfwSwapBuffers(window);
		glfwPollEvents();

		while (glfwGetTime() - frameStart < TARGET_FRAME_TIME) {}
	}

	texBackground.Delete();
	texCoin.Delete();

	VAO1.Delete();
	VBO1.Delete();
	EBO1.Delete();
	shaderProgram.Delete();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}