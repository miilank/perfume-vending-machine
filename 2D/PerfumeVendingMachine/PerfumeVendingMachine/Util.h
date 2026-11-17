#ifndef UTIL_H
#define UTIL_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Creates a system cursor from an image; (hotspotX, hotspotY) is the clicking point in image pixels
GLFWcursor* loadImageToCursor(const char* filePath, int hotspotX, int hotspotY);

#endif