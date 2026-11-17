#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "shaderClass.h"
#include "VAO.h"
#include "VBO.h"
#include "EBO.h"
#include "Texture.h"
#include "Util.h"

// ---------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------
const double TARGET_FPS = 75.0;
const double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;

const int PERFUME_COUNT = 5;
const int COIN_VALUE = 50;
const int PRICES[PERFUME_COUNT] = { 100, 150, 100, 150, 200 };
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

const float COIN_ANIM_TIME = 0.45f;   // coin flying into the slot
const float CARD_MOVE_TIME = 0.3f;    // card moving to the reader - from the place where card is set, to the center of the card slot
const float CARD_PROCESS_TIME = 1.0f; // card waiting at the reader
const float SPRAY_TIME = 1.5f;       
const float BUTTON_PRESS_TIME = 0.18f;
const float ERROR_TIME = 0.6f;

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------
struct Rect {
	float cx, cy, w, h;
	bool contains(float x, float y) const {
		return x >= cx - w / 2 && x <= cx + w / 2 && y >= cy - h / 2 && y <= cy + h / 2;
	}
};

enum MachineState {
	STATE_IDLE,            // nothing selected
	STATE_SELECTED,        // perfume selected, nothing paid yet
	STATE_PAYING,          // part of the price paid with coins
	STATE_CARD_PROCESSING, // card is at the reader
	STATE_READY,           // paid, waiting for the button
	STATE_SPRAYING         // spray animation
};

enum Hand { HAND_EMPTY, HAND_COIN, HAND_CARD };

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------
int screenW = 1920, screenH = 1080;

// Whe re the machine is on the screen and how big it is (set in updateLayout)
float machineScale = 1.0f;
float machineLeft = 0.0f;
float machineTop = 0.0f;

MachineState state = STATE_IDLE;
Hand hand = HAND_EMPTY;
int selected = -1; // index of the selected perfume, -1 if none
int paid = 0;

// Timers that count down to zero; while they are above zero something is shown
float buttonPressTimer[PERFUME_COUNT] = { 0 };
float errorTimer = 0.0f;

// Animations: how long they have been running and where the object started
bool coinAnimActive = false;
float coinAnimT = 0.0f;
float coinStartX = 0.0f, coinStartY = 0.0f;

float cardT = 0.0f;
float cardStartX = 0.0f, cardStartY = 0.0f;

float sprayT = 0.0f;

float mouseX = 0.0f, mouseY = 0.0f;

GLFWcursor* cursorBottle = nullptr;
GLFWcursor* cursorCoin = nullptr;
GLFWcursor* cursorCard = nullptr;
Hand cursorHand = HAND_EMPTY; // which cursor is set right now

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
int endProgram(std::string message) {
	std::cout << message << std::endl;
	glfwTerminate();
	return -1;
}

// Goes from a to b as t goes from 0 to 1; every animation is built on this - SMOOTH from a to b
float lerpf(float a, float b, float t) {
	return a + (b - a) * t;
}

// Decides how big the machine is and where it stands, so it looks the same on any resolution.
// The machine takes 94% of the screen height and leaves room on the right for the wallet.
void updateLayout() {
	machineScale = screenH * 0.94f / MACHINE_H;
	float machineWidth = MACHINE_W * machineScale;
	machineLeft = (std::max)(screenW * 0.02f, screenW * 0.40f - machineWidth / 2.0f);
	machineTop = screenH / 2.0f + MACHINE_H * machineScale / 2.0f;
}

// Machine coordinates (origin top-left of machine_body.png, y down) to screen pixels (origin bottom-left, y up)
float toScreenX(float mx) { return machineLeft + mx * machineScale; }
float toScreenY(float my) { return machineTop - my * machineScale; }

// A part of the machine measured on machine_body.png, turned into a rectangle on the screen
Rect machineRect(float x0, float y0, float x1, float y1) {
	return Rect{ toScreenX((x0 + x1) / 2.0f), toScreenY((y0 + y1) / 2.0f),
				 (x1 - x0) * machineScale, (y1 - y0) * machineScale };
}

// Center of the column of perfume i (bottle, nozzle and button are in the same column)
float slotCenterX(int i) { return 130.0f + 160.0f * i; }

// Where each part of the machine is. Drawing and mouse clicks use the same functions,
// so what the user sees is always exactly what can be clicked.
Rect bottleRect(int i) { float c = slotCenterX(i); return machineRect(c - 47.5f, 172.0f, c + 47.5f, 362.0f); } // bottle at column i
Rect nozzleRect(int i) { float c = slotCenterX(i); return machineRect(c - 25.0f, 413.0f, c + 25.0f, 473.0f); } // nozzle at column i
Rect buttonRect(int i) { float c = slotCenterX(i); return machineRect(c - 38.0f, 492.0f, c + 38.0f, 568.0f); } // button at column i

Rect showcaseRect() { return machineRect(50.0f, 130.0f, 850.0f, 400.0f); }
Rect screenRect() { return machineRect(70.0f, 616.0f, 450.0f, 856.0f); }
Rect coinSlotRect() { return machineRect(500.0f, 650.0f, 620.0f, 830.0f); }
Rect cardReaderRect() { return machineRect(640.0f, 650.0f, 840.0f, 790.0f); }

// The wallet is not a part of the machine, it is placed in the free space to the right of it
Rect walletMatRect() {
	float right = toScreenX(MACHINE_W);
	float space = screenW - right;
	float w = (std::min)(space * 0.8f, screenH * 0.24f);
	return Rect{ right + space / 2.0f, screenH * 0.36f, w, screenH * 0.42f };
}

// Coin in the upper part of the wallet; the user picks it up from here
Rect walletCoinRect() {
	Rect mat = walletMatRect();
	float size = (std::min)(screenH * 0.09f, mat.w * 0.5f);
	return Rect{ mat.cx, mat.cy + mat.h * 0.22f, size, size };
}

// Card in the lower part of the wallet, keeping the 256x160 shape of the image
Rect walletCardRect() {
	Rect mat = walletMatRect();
	float w = (std::min)(screenH * 0.16f, mat.w * 0.85f);
	return Rect{ mat.cx, mat.cy - mat.h * 0.18f, w, w * 160.0f / 256.0f };
}

// ---------------------------------------------------------------------------
// Game logic
// ---------------------------------------------------------------------------

// While something is animating the machine ignores new actions,
// so two things can never start at the same time
bool isBusy() {
	return coinAnimActive || state == STATE_CARD_PROCESSING || state == STATE_SPRAYING;
}

void showError() {
	errorTimer = ERROR_TIME;
}

// Back to the start after a purchase, ready for the next customer
void resetMachine() {
	state = STATE_IDLE;
	selected = -1;
	paid = 0;
}

// The perfume can be chosen or changed only before anything is paid
void selectPerfume(int i) {
	if (state == STATE_IDLE || state == STATE_SELECTED) {
		selected = i;
		state = STATE_SELECTED;
	}
	else if (i != selected) {
		showError();
	}
}

void startSpray() {
	state = STATE_SPRAYING;
	sprayT = 0.0f;
}

// A button has two roles: before paying it selects the perfume,
// after paying the button of the selected perfume starts the spray
void pressButton(int i) {
	buttonPressTimer[i] = BUTTON_PRESS_TIME;
	if (isBusy()) return;

	if (state == STATE_READY) {
		if (i == selected) startSpray();
		else showError();
	}
	else if (state == STATE_PAYING) {
		showError();
	}
	else {
		selectPerfume(i);
	}
}

// The coin only starts flying here; the money is added in update() when it reaches the slot
void insertCoin() {
	if (selected < 0 || paid >= PRICES[selected] || isBusy()) {
		showError();
		return;
	}
	coinAnimActive = true;
	coinAnimT = 0.0f;
	coinStartX = mouseX;
	coinStartY = mouseY;
}

// The card leaves the hand and goes to the reader; update() finishes the payment
void useCard() {
	if (selected < 0 || paid >= PRICES[selected] || isBusy()) {
		showError();
		return;
	}
	state = STATE_CARD_PROCESSING;
	cardT = 0.0f;
	cardStartX = mouseX;
	cardStartY = mouseY;
	hand = HAND_EMPTY;
}

// GLFW gives the mouse in window coordinates with y going down;
// this turns it into the same pixels that are used for drawing
void updateMouse(GLFWwindow* window) {
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	int winW, winH;
	glfwGetWindowSize(window, &winW, &winH);
	if (winW <= 0 || winH <= 0) return;
	mouseX = (float)(xpos * screenW / winW);
	mouseY = (float)(screenH - ypos * screenH / winH);
}

// Moves the time of the program forward by dt. Timers run out, animations progress,
// and when an animation ends the machine moves to its next phase.
// Drawing only shows what this function decided.
void update(float dt) {
	for (int i = 0; i < PERFUME_COUNT; i++)
		buttonPressTimer[i] = (std::max)(0.0f, buttonPressTimer[i] - dt);
	errorTimer = (std::max)(0.0f, errorTimer - dt);

	// Coin reached the slot: add the money, then either ready or waiting for more
	if (coinAnimActive) {
		coinAnimT += dt;
		if (coinAnimT >= COIN_ANIM_TIME) {
			coinAnimActive = false;
			paid += COIN_VALUE;
			if (paid >= PRICES[selected])
				state = STATE_READY;
			else
				state = STATE_PAYING;
		}
	}

	// Card finished: it pays whatever is left, no matter how many coins went in before
	if (state == STATE_CARD_PROCESSING) {
		cardT += dt;
		if (cardT >= CARD_MOVE_TIME + CARD_PROCESS_TIME) {
			paid = PRICES[selected];
			state = STATE_READY;
		}
	}

	// Spray finished: the purchase is over
	if (state == STATE_SPRAYING) {
		sprayT += dt;
		if (sprayT >= SPRAY_TIME)
			resetMachine();
	}
}

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------

// Esc closes the program in any state, keys 1-5 work like the five buttons
void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);

	if (action == GLFW_PRESS && key >= GLFW_KEY_1 && key <= GLFW_KEY_5)
		pressButton(key - GLFW_KEY_1);
}

// Turns a click into an action, depending on what the user is holding and where they clicked
void mouseCallback(GLFWwindow* window, int button, int action, int mods) {
	if (action != GLFW_PRESS) return;
	updateMouse(window);

	// Right click puts back whatever is in the hand
	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		hand = HAND_EMPTY;
		return;
	}
	if (button != GLFW_MOUSE_BUTTON_LEFT) return;

	// Holding a coin: the slot takes it, a click anywhere else drops it
	// and is then handled as a normal click (so a button can still be pressed)
	if (hand == HAND_COIN) {
		if (coinSlotRect().contains(mouseX, mouseY)) { insertCoin(); return; }
		if (walletCoinRect().contains(mouseX, mouseY)) return;
		hand = HAND_EMPTY;
	}
	// Holding the card: the reader takes it, a click on the wallet puts it back
	if (hand == HAND_CARD) {
		if (cardReaderRect().contains(mouseX, mouseY)) { useCard(); return; }
		hand = HAND_EMPTY;
		if (walletCardRect().contains(mouseX, mouseY)) return;
	}

	// Picking up from the wallet (the card is not there while it is at the reader)
	if (walletCoinRect().contains(mouseX, mouseY)) { hand = HAND_COIN; return; }
	if (walletCardRect().contains(mouseX, mouseY) && state != STATE_CARD_PROCESSING) { hand = HAND_CARD; return; }

	// Buttons and bottles of the machine
	for (int i = 0; i < PERFUME_COUNT; i++) {
		if (buttonRect(i).contains(mouseX, mouseY)) { pressButton(i); return; }
		if (bottleRect(i).contains(mouseX, mouseY)) {
			if (!isBusy()) selectPerfume(i);
			return;
		}
	}
}

// ---------------------------------------------------------------------------
// Drawing
// ---------------------------------------------------------------------------
Shader* gShader = nullptr;

// Everything on the screen is the same unit square; these uniforms tell the shader
// where to put it, how big it is and how transparent
void setCommonUniforms(const Rect& r, float alpha, int shape) {
	gShader->setVec2("uPos", r.cx, r.cy);
	gShader->setVec2("uSize", r.w, r.h);
	gShader->setFloat("uAlpha", alpha);
	gShader->setInt("uShape", shape);
}

// Draws an image in a rectangle. The tint darkens or colors it (bottles, buttons, digits, spray),
// and the UV rectangle picks a part of the image (one digit from the digit atlas).
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

// Draws a rectangle of one color: shadow, wallet mat, red error flash
void drawColor(const Rect& r, float red, float green, float blue, float alpha, int shape = 0) {
	setCommonUniforms(r, 1.0f, shape);
	gShader->setInt("uUseTexture", 0);
	gShader->setVec4("uColor", red, green, blue, alpha);
	gShader->setVec4("uUVRect", 0.0f, 0.0f, 1.0f, 1.0f);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

// Text that never changes ("Price", "Paid", "RSD") is a ready image.
// Draws it with a given height (in machine units), keeping its aspect ratio; returns the right edge
float drawLabel(Texture& tex, float mx, float myCenter, float mh) {
	float mw = mh * tex.width / (float)tex.height;
	drawTexture(tex, machineRect(mx, myCenter - mh / 2, mx + mw, myCenter + mh / 2));
	return mx + mw;
}

// Amounts change all the time, so they are built digit by digit
// from the digit atlas "0123456789./" (12 cells); returns the right edge
float drawNumber(Texture& digits, int value, float mx, float myCenter, float mh,
	float tr, float tg, float tb) {
	std::string s = std::to_string(value);
	float cellW = mh * 64.0f / 96.0f;
	float advance = cellW * 0.7f;
	float x = mx;
	for (char c : s) {
		int idx = c - '0';
		drawTexture(digits, machineRect(x, myCenter - mh / 2, x + cellW, myCenter + mh / 2),
			1.0f, tr, tg, tb, idx / 12.0f, 0.0f, 1.0f / 12.0f, 1.0f);
		x += advance;
	}
	return x + (cellW - advance);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main()
{
	// Window over the whole monitor with an OpenGL 3.3 core context
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

	// V-sync off, the frame limiter at the end of the loop keeps 75 FPS
	glfwSwapInterval(0);
	glfwSetKeyCallback(window, keyboardCallback);
	glfwSetMouseButtonCallback(window, mouseCallback);

	// Three cursors: the default perfume bottle, and the coin or card while holding them
	cursorBottle = loadImageToCursor("res/cursor.png", 20, 3);
	cursorCoin = loadImageToCursor("res/cursor_coin.png", 32, 32);
	cursorCard = loadImageToCursor("res/cursor_card.png", 48, 30);
	glfwSetCursor(window, cursorBottle);

	// GLEW needs the context, so it comes after glfwMakeContextCurrent
	if (glewInit() != GLEW_OK) return endProgram("Failed to initialize GLEW.");

	glfwGetFramebufferSize(window, &screenW, &screenH);
	glViewport(0, 0, screenW, screenH);
	updateLayout();

	// Shader
	Shader shaderProgram("default.vert", "default.frag");
	gShader = &shaderProgram;

	// The only shape in the program: a 1x1 square centered at (0,0), position + UV per vertex.
	// Every element on the screen is this square, moved and scaled by uniforms.
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

	// Textures
	Texture texBackground("res/background.png", GL_TEXTURE_2D, GL_TEXTURE0);
	Texture texBody("res/machine_body.png", GL_TEXTURE_2D, GL_TEXTURE0);
	Texture texShowcase("res/showcase.png", GL_TEXTURE_2D, GL_TEXTURE0);
	Texture texNozzle("res/nozzle.png", GL_TEXTURE_2D, GL_TEXTURE0);
	Texture texScreen("res/screen.png", GL_TEXTURE_2D, GL_TEXTURE0);
	Texture texCoinSlot("res/coin_slot.png", GL_TEXTURE_2D, GL_TEXTURE0);
	Texture texCardReader("res/card_reader.png", GL_TEXTURE_2D, GL_TEXTURE0);
	Texture texCoin("res/coin.png", GL_TEXTURE_2D, GL_TEXTURE0);
	Texture texCard("res/card.png", GL_TEXTURE_2D, GL_TEXTURE0);
	Texture texSpray("res/spray_cloud.png", GL_TEXTURE_2D, GL_TEXTURE0);
	Texture texDigits("res/digits.png", GL_TEXTURE_2D, GL_TEXTURE0);
	Texture texLabelPrice("res/label_price.png", GL_TEXTURE_2D, GL_TEXTURE0);
	Texture texLabelPaid("res/label_paid.png", GL_TEXTURE_2D, GL_TEXTURE0);
	Texture texLabelRsd("res/label_rsd.png", GL_TEXTURE_2D, GL_TEXTURE0);
	Texture texName("res/name_label.png", GL_TEXTURE_2D, GL_TEXTURE0);

	// One bottle, button, pressed button and info screen per perfume; info_0 is "Choose a perfume"
	std::vector<Texture> texBottles, texButtons, texButtonsPressed, texInfo;
	texInfo.push_back(Texture("res/info_0.png", GL_TEXTURE_2D, GL_TEXTURE0));
	for (int i = 1; i <= PERFUME_COUNT; i++) {
		std::string n = std::to_string(i);
		texBottles.push_back(Texture(("res/bottle_" + n + ".png").c_str(), GL_TEXTURE_2D, GL_TEXTURE0));
		texButtons.push_back(Texture(("res/button_" + n + ".png").c_str(), GL_TEXTURE_2D, GL_TEXTURE0));
		texButtonsPressed.push_back(Texture(("res/button_" + n + "_pressed.png").c_str(), GL_TEXTURE_2D, GL_TEXTURE0));
		texInfo.push_back(Texture(("res/info_" + n + ".png").c_str(), GL_TEXTURE_2D, GL_TEXTURE0));
	}

	// All textures use unit 0 and the screen size never changes, so these are set once
	texBackground.texUnit(shaderProgram, "tex0", 0);
	shaderProgram.Activate();
	shaderProgram.setVec2("uScreen", (float)screenW, (float)screenH);

	// Transparency: a new pixel is mixed with what is already drawn, using its alpha
	glClearColor(0.2f, 0.2f, 0.25f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	double lastFrame = glfwGetTime();

	while (!glfwWindowShouldClose(window))
	{
		// Time since the last frame; capped so animations do not jump after a pause
		double frameStart = glfwGetTime();
		float dt = (float)(frameStart - lastFrame);
		lastFrame = frameStart;
		if (dt > 0.1f) dt = 0.1f;

		updateMouse(window);
		update(dt);

		// The cursor shows what the user is holding; it is changed only when that changes
		if (hand != cursorHand) {
			if (hand == HAND_COIN) glfwSetCursor(window, cursorCoin);
			else if (hand == HAND_CARD) glfwSetCursor(window, cursorCard);
			else glfwSetCursor(window, cursorBottle);
			cursorHand = hand;
		}

		// The whole scene is drawn again every frame, from the back to the front
		glClear(GL_COLOR_BUFFER_BIT);
		shaderProgram.Activate();
		VAO1.Bind();

		float s = machineScale;

		// Background, shadow of the machine, machine
		drawTexture(texBackground, Rect{ screenW / 2.0f, screenH / 2.0f, (float)screenW, (float)screenH });
		Rect body = machineRect(0.0f, 0.0f, MACHINE_W, MACHINE_H);
		drawColor(Rect{ body.cx + 16.0f * s, body.cy - 30.0f * s, body.w - 40.0f * s, body.h - 40.0f * s }, 0.0f, 0.0f, 0.0f, 0.3f);
		drawTexture(texBody, body);

		// Showcase: bottles (the not selected ones are darker), then the glass in front of them
		for (int i = 0; i < PERFUME_COUNT; i++) {
			float shade = (selected >= 0 && i != selected) ? 0.6f : 1.0f;
			drawTexture(texBottles[i], bottleRect(i), 1.0f, shade, shade, shade);
		}
		drawTexture(texShowcase, showcaseRect());

		// Nozzles and buttons; a pressed button is shown for a moment, the not selected ones are darker
		for (int i = 0; i < PERFUME_COUNT; i++) {
			drawTexture(texNozzle, nozzleRect(i));
			Texture& btn = buttonPressTimer[i] > 0.0f ? texButtonsPressed[i] : texButtons[i];
			float shade = (selected >= 0 && i != selected) ? 0.6f : 1.0f;
			drawTexture(btn, buttonRect(i), 1.0f, shade, shade, shade);
		}

		// Info screen: "Choose a perfume" or the description of the selected one, red flash on error
		Rect scr = screenRect();
		drawTexture(texScreen, scr);
		drawTexture(texInfo[selected >= 0 ? selected + 1 : 0], scr);
		if (errorTimer > 0.0f)
			drawColor(scr, 0.9f, 0.1f, 0.1f, 0.35f * errorTimer / ERROR_TIME);

		// Price of the selected perfume and how much is paid so far
		float rowPrice = 893.0f, rowPaid = 937.0f, labelH = 34.0f, digitH = 40.0f;
		drawLabel(texLabelPrice, 74.0f, rowPrice, labelH);
		if (selected >= 0) {
			float x = drawNumber(texDigits, PRICES[selected], 230.0f, rowPrice, digitH, 1.0f, 0.85f, 0.4f);
			drawLabel(texLabelRsd, x + 6.0f, rowPrice, labelH);
		}
		drawLabel(texLabelPaid, 74.0f, rowPaid, labelH);
		{
			float x = drawNumber(texDigits, paid, 230.0f, rowPaid, digitH, 0.55f, 1.0f, 0.6f);
			drawLabel(texLabelRsd, x + 6.0f, rowPaid, labelH);
		}

		// Payment devices
		Rect slot = coinSlotRect();
		Rect reader = cardReaderRect();
		drawTexture(texCoinSlot, slot);
		drawTexture(texCardReader, reader);

		// Coin flying into the slot: it moves, shrinks and fades out
		if (coinAnimActive) {
			float t = coinAnimT / COIN_ANIM_TIME;
			float size = lerpf(screenH * 0.07f, 60.0f * s, t);
			Rect r{ lerpf(coinStartX, toScreenX(560.0f), t), lerpf(coinStartY, toScreenY(720.0f), t), size, size };
			drawTexture(texCoin, r, 1.0f - t);
		}

		// Card moving to the reader and staying there while the payment is processed
		if (state == STATE_CARD_PROCESSING) {
			float t = (std::min)(1.0f, cardT / CARD_MOVE_TIME);
			float w = lerpf(screenH * 0.14f, 170.0f * s, t);
			Rect r{ lerpf(cardStartX, reader.cx, t), lerpf(cardStartY, reader.cy, t), w, w * 160.0f / 256.0f };
			drawTexture(texCard, r);
		}

		// Spray: three clouds in the color of the perfume leave the nozzle 0.2 s apart,
		// one to the left, one straight and one to the right; each grows and fades out
		if (state == STATE_SPRAYING) {
			const float* c = PERFUME_COLORS[selected];
			float tr = lerpf(1.0f, c[0], 0.45f), tg = lerpf(1.0f, c[1], 0.45f), tb = lerpf(1.0f, c[2], 0.45f);
			Rect nozzle = nozzleRect(selected);
			for (int k = 0; k < 3; k++) {
				float t = (sprayT - k * 0.2f) / 1.1f;
				if (t <= 0.0f || t >= 1.0f) continue;
				float size = lerpf(30.0f, 220.0f, t) * s;
				float x = nozzle.cx + (k - 1) * 30.0f * s * t;
				float y = toScreenY(469.0f) - 160.0f * s * t;
				drawTexture(texSpray, Rect{ x, y, size, size }, 0.8f * (1.0f - t), tr, tg, tb);
			}
		}

		// Wallet: the card is faded while it is in the hand or at the reader
		Rect mat = walletMatRect();
		drawColor(mat, 0.1f, 0.08f, 0.06f, 0.35f);
		drawTexture(texCoin, walletCoinRect());
		bool cardAway = hand == HAND_CARD || state == STATE_CARD_PROCESSING;
		drawTexture(texCard, walletCardRect(), cardAway ? 0.25f : 1.0f);

		// Semi-transparent name label in the top right corner, drawn last so it is above everything
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

		// Frame limiter: wait until 1/75 s has passed since the frame started
		while (glfwGetTime() - frameStart < TARGET_FRAME_TIME) {}
	}

	// Cleanup
	texBackground.Delete();
	texBody.Delete();
	texShowcase.Delete();
	texNozzle.Delete();
	texScreen.Delete();
	texCoinSlot.Delete();
	texCardReader.Delete();
	texCoin.Delete();
	texCard.Delete();
	texSpray.Delete();
	texDigits.Delete();
	texLabelPrice.Delete();
	texLabelPaid.Delete();
	texLabelRsd.Delete();
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
	glfwDestroyCursor(cursorCoin);
	glfwDestroyCursor(cursorCard);

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}