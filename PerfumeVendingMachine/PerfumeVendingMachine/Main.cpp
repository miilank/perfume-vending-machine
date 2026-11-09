#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

#include "shaderClass.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"

const double TARGET_FPS = 75.0;
const double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;

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

	shaderProgram.Activate();
	shaderProgram.setVec2("uScreen", (float)screenW, (float)screenH);
	shaderProgram.setVec4("uUVRect", 0.0f, 0.0f, 1.0f, 1.0f);
	shaderProgram.setVec3("uTint", 1.0f, 1.0f, 1.0f);
	shaderProgram.setInt("uUseTexture", 0);

	glClearColor(0.5f, 0.1f, 0.6f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	while (!glfwWindowShouldClose(window))
	{
		double frameStart = glfwGetTime();

		glClear(GL_COLOR_BUFFER_BIT);
		shaderProgram.Activate();
		VAO1.Bind();

		shaderProgram.setVec2("uPos", screenW / 2.0f, screenH / 2.0f);
		shaderProgram.setVec2("uSize", screenW * 0.4f, screenH * 0.5f);
		shaderProgram.setVec4("uColor", 0.9f, 0.75f, 0.2f, 1.0f);
		shaderProgram.setFloat("uAlpha", 1.0f);
		shaderProgram.setInt("uShape", 0);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		shaderProgram.setVec2("uPos", screenW * 0.7f, screenH / 2.0f);
		shaderProgram.setVec2("uSize", screenH * 0.4f, screenH * 0.4f);
		shaderProgram.setVec4("uColor", 0.2f, 0.6f, 1.0f, 1.0f);
		shaderProgram.setFloat("uAlpha", 0.6f);
		shaderProgram.setInt("uShape", 1);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glfwSwapBuffers(window);
		glfwPollEvents();

		while (glfwGetTime() - frameStart < TARGET_FRAME_TIME) {}
	}

	VAO1.Delete();
	VBO1.Delete();
	EBO1.Delete();
	shaderProgram.Delete();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}