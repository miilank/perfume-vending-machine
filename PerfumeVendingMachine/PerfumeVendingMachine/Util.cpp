#include "Util.h"

#include <iostream>

// The stb_image library code is compiled here, in exactly one .cpp file.
// All other files include stb_image.h without this define and only see the declarations.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLFWcursor* loadImageToCursor(const char* filePath, int hotspotX, int hotspotY)
{
	// GLFW expects the top row first, so the image must not be flipped like OpenGL textures
	stbi_set_flip_vertically_on_load(false);

	int width, height, channels;
	unsigned char* imageData = stbi_load(filePath, &width, &height, &channels, 4);
	if (imageData == NULL)
	{
		std::cout << "Failed to load cursor: " << filePath << std::endl;
		return nullptr;
	}

	GLFWimage image;
	image.width = width;
	image.height = height;
	image.pixels = imageData;

	GLFWcursor* cursor = glfwCreateCursor(&image, hotspotX, hotspotY);
	stbi_image_free(imageData);
	return cursor;
}