#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cmath>
#include <string>
#include <algorithm>
#include <vector>

#include "shaderClass.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "Texture.h"
#include "Util.h"

// SETTINGS
const double TARGET_FPS = 75.0;
const double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;

const int PERFUME_COUNT = 5;
const float PERFUME_COLORS[PERFUME_COUNT][3] = {
	{ 235 / 255.0f, 200 / 255.0f,  50 / 255.0f },
	{ 225 / 255.0f, 110 / 255.0f, 150 / 255.0f },
	{ 90 / 255.0f, 170 / 255.0f, 100 / 255.0f },
	{ 70 / 255.0f, 140 / 255.0f, 220 / 255.0f },
	{ 140 / 255.0f,  80 / 255.0f, 180 / 255.0f },
};

// Size of machine_body.png; every machine part is placed in these pixel coordinates
const float MACHINE_W = 900.0f;
const float MACHINE_H = 1000.0f;

const float BUTTON_PRESS_TIME = 0.18f;

// TYPES
struct Rect {
	float cx, cy, w, h;
	bool contains(float x, float y) const {
		return x >= cx - w / 2 && x <= cx + w / 2 && y >= cy - h / 2 && y <= cy + h / 2;
	}
};

// GLOBAL STATE
int screenW = 1920, screenH = 1080;

float machineScale = 1.0f;
float machineLeft = 0.0f;
float machineTop = 0.0f;

int selected = -1;

float buttonPressTimer[PERFUME_COUNT] = { 0 };

float mouseX = 0.0f, mouseY = 0.0f;
float timeNow = 0.0f;

GLFWcursor* cursorBottle = nullptr;

// HELPERS
int endProgram(std::string message) {
	std::cout << message << std::endl;
	glfwTerminate();
	return -1;
}

void updateLayout() {
	machineScale = screenH * 0.94f / MACHINE_H;
	float machineWidth = MACHINE_W * machineScale;
	machineLeft = (std::max)(screenW * 0.02f, screenW * 0.40f - machineWidth / 2.0f);
	machineTop = screenH / 2.0f + MACHINE_H * machineScale / 2.0f;
}

// Machine coordinates (origin top-left of machine_body.png, y down) to screen pixels (origin bottom-left, y up)
float toScreenX(float mx) { return machineLeft + mx * machineScale; }
float toScreenY(float my) { return machineTop - my * machineScale; }

Rect machineRect(float x0, float y0, float x1, float y1) {
	return Rect{ toScreenX((x0 + x1) / 2.0f), toScreenY((y0 + y1) / 2.0f),
				 (x1 - x0) * machineScale, (y1 - y0) * machineScale };
}

float slotCenterX(int i) { return 130.0f + 160.0f * i; }

Rect bottleRect(int i) { float c = slotCenterX(i); return machineRect(c - 47.5f, 172.0f, c + 47.5f, 362.0f); }
Rect bottleGlow(int i) { float c = slotCenterX(i); return machineRect(c - 85.0f, 150.0f, c + 85.0f, 385.0f); }
Rect nozzleRect(int i) { float c = slotCenterX(i); return machineRect(c - 25.0f, 413.0f, c + 25.0f, 473.0f); }
Rect buttonRect(int i) { float c = slotCenterX(i); return machineRect(c - 38.0f, 492.0f, c + 38.0f, 568.0f); }

Rect showcaseRect() { return machineRect(50.0f, 130.0f, 850.0f, 400.0f); }
Rect screenRect() { return machineRect(70.0f, 616.0f, 450.0f, 856.0f); }

// GAME LOGIC
void selectPerfume(int i) {
	selected = i;
}

void pressButton(int i) {
	buttonPressTimer[i] = BUTTON_PRESS_TIME;
	selectPerfume(i);
}
void updateMouse(GLFWwindow* window) {
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	int winW, winH;
	glfwGetWindowSize(window, &winW, &winH);
	if (winW <= 0 || winH <= 0) return;
	mouseX = (float)(xpos * screenW / winW);
	mouseY = (float)(screenH - ypos * screenH / winH);
}

void update(float dt) {
	for (int i = 0; i < PERFUME_COUNT; i++)
		buttonPressTimer[i] = (std::max)(0.0f, buttonPressTimer[i] - dt);
}

// CALLBACKS
void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);

	if (action == GLFW_PRESS && key >= GLFW_KEY_1 && key <= GLFW_KEY_5)
		pressButton(key - GLFW_KEY_1);
}

void mouseCallback(GLFWwindow* window, int button, int action, int mods) {
	if (action != GLFW_PRESS) return;
	updateMouse(window);
	if (button != GLFW_MOUSE_BUTTON_LEFT) return;

	for (int i = 0; i < PERFUME_COUNT; i++) {
		if (buttonRect(i).contains(mouseX, mouseY)) { pressButton(i); return; }
		if (bottleRect(i).contains(mouseX, mouseY)) {
			selectPerfume(i);
			return;
		}
	}
}

// DRAWING
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

// MAIN
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
	glfwSetMouseButtonCallback(window, mouseCallback);

	cursorBottle = loadImageToCursor("res/cursor.png", 20, 3);
	glfwSetCursor(window, cursorBottle);

	if (glewInit() != GLEW_OK) return endProgram("Failed to initialize GLEW.");

	glfwGetFramebufferSize(window, &screenW, &screenH);
	glViewport(0, 0, screenW, screenH);
	updateLayout();

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
	Texture texBody("res/machine_body.png", GL_TEXTURE_2D, GL_TEXTURE0);
	Texture texShowcase("res/showcase.png", GL_TEXTURE_2D, GL_TEXTURE0);
	Texture texNozzle("res/nozzle.png", GL_TEXTURE_2D, GL_TEXTURE0);
	Texture texScreen("res/screen.png", GL_TEXTURE_2D, GL_TEXTURE0);
	Texture texName("res/name_label.png", GL_TEXTURE_2D, GL_TEXTURE0);

	std::vector<Texture> texBottles, texButtons, texButtonsPressed, texInfo;
	texInfo.push_back(Texture("res/info_0.png", GL_TEXTURE_2D, GL_TEXTURE0));
	for (int i = 1; i <= PERFUME_COUNT; i++) {
		std::string n = std::to_string(i);
		texBottles.push_back(Texture(("res/bottle_" + n + ".png").c_str(), GL_TEXTURE_2D, GL_TEXTURE0));
		texButtons.push_back(Texture(("res/button_" + n + ".png").c_str(), GL_TEXTURE_2D, GL_TEXTURE0));
		texButtonsPressed.push_back(Texture(("res/button_" + n + "_pressed.png").c_str(), GL_TEXTURE_2D, GL_TEXTURE0));
		texInfo.push_back(Texture(("res/info_" + n + ".png").c_str(), GL_TEXTURE_2D, GL_TEXTURE0));
	}

	texBackground.texUnit(shaderProgram, "tex0", 0);
	shaderProgram.Activate();
	shaderProgram.setVec2("uScreen", (float)screenW, (float)screenH);

	glClearColor(0.2f, 0.2f, 0.25f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	double lastFrame = glfwGetTime();

	while (!glfwWindowShouldClose(window))
	{
		double frameStart = glfwGetTime();
		float dt = (float)(frameStart - lastFrame);
		lastFrame = frameStart;
		if (dt > 0.1f) dt = 0.1f;
		timeNow = (float)frameStart;

		updateMouse(window);
		update(dt);

		glClear(GL_COLOR_BUFFER_BIT);
		shaderProgram.Activate();
		VAO1.Bind();

		float s = machineScale;

		// background and machine
		drawTexture(texBackground, Rect{ screenW / 2.0f, screenH / 2.0f, (float)screenW, (float)screenH });
		Rect body = machineRect(0.0f, 0.0f, MACHINE_W, MACHINE_H);
		drawColor(Rect{ body.cx + 16.0f * s, body.cy - 30.0f * s, body.w - 40.0f * s, body.h - 40.0f * s }, 0.0f, 0.0f, 0.0f, 0.3f);
		drawTexture(texBody, body);

		// Showcase: glow, bottles, glass
		if (selected >= 0) {
			const float* c = PERFUME_COLORS[selected];
			float pulse = 0.55f + 0.15f * sinf(timeNow * 3.0f);
			drawColor(bottleGlow(selected), c[0], c[1], c[2], pulse, 2);
		}
		for (int i = 0; i < PERFUME_COUNT; i++) {
			float shade = (selected >= 0 && i != selected) ? 0.6f : 1.0f;
			drawTexture(texBottles[i], bottleRect(i), 1.0f, shade, shade, shade);
		}
		drawTexture(texShowcase, showcaseRect());

		// Nozzles and buttons
		for (int i = 0; i < PERFUME_COUNT; i++) {
			drawTexture(texNozzle, nozzleRect(i));
			Texture& btn = buttonPressTimer[i] > 0.0f ? texButtonsPressed[i] : texButtons[i];
			drawTexture(btn, buttonRect(i));
		}

		// Info screen
		Rect scr = screenRect();
		drawTexture(texScreen, scr);
		drawTexture(texInfo[selected >= 0 ? selected + 1 : 0], scr);

		// Semi-transparent name label in the top right corner
		{
			float margin = screenH * 0.02f;
			float h = screenH * 0.045f;
			float w = h * texName.width / (float)texName.height;
			float maxW = screenW - toScreenX(MACHINE_W) - 2.0f * margin;
			if (w > maxW && maxW > 0.0f) { w = maxW; h = w * texName.height / (float)texName.width; }
			drawTexture(texName, Rect{ screenW - margin - w / 2.0f, screenH - margin - h / 2.0f, w, h }, 0.75f);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();

		while (glfwGetTime() - frameStart < TARGET_FRAME_TIME) {}
	}

	// Cleanup
	texBackground.Delete();
	texBody.Delete();
	texShowcase.Delete();
	texNozzle.Delete();
	texScreen.Delete();
	texName.Delete();
	for (Texture& t : texBottles) t.Delete();
	for (Texture& t : texButtons) t.Delete();
	for (Texture& t : texButtonsPressed) t.Delete();
	for (Texture& t : texInfo) t.Delete();

	VAO1.Delete();
	VBO1.Delete();
	EBO1.Delete();
	shaderProgram.Delete();

	glfwDestroyCursor(cursorBottle);

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}