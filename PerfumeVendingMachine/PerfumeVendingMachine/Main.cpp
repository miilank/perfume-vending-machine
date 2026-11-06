#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

const double TARGET_FPS = 75.0;
const double TARGET_FRAME_TIME = 1.0 / TARGET_FPS;

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
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;

    glViewport(0, 0, mode->width, mode->height);
    glClearColor(0.5f, 0.1f, 0.6f, 1.0f);

    int frames = 0;
    double fpsTimer = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        double frameStart = glfwGetTime();

        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();

        while (glfwGetTime() - frameStart < TARGET_FRAME_TIME) {}

        frames++;
        double now = glfwGetTime();
        if (now - fpsTimer >= 1.0) {
            std::cout << "FPS: " << frames << std::endl;
            frames = 0;
            fpsTimer = now;
        }
    }

    glfwTerminate();
    return 0;
}