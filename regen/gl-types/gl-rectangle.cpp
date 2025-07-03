#include "gl-rectangle.h"

using namespace regen;

GLRectangle::GLRectangle(
		CreateObjectFunc createObjects,
		ReleaseObjectFunc releaseObjects,
		GLuint numObjects)
		: GLObject(createObjects, releaseObjects, numObjects) {
	set_rectangleSize(2, 2);
}

GLRectangle::GLRectangle(
		CreateObjectFunc2 createObjects,
		ReleaseObjectFunc releaseObjects,
		GLenum objectTarget,
		GLuint numObjects)
		: GLObject(createObjects, releaseObjects, objectTarget, numObjects) {
	set_rectangleSize(2, 2);
}

void GLRectangle::set_rectangleSize(uint32_t width, uint32_t height) {
	sizeUI_.x = width;
	sizeUI_.y = height;
	size_.x = static_cast<float>(width);
	size_.y = static_cast<float>(height);
	sizeInverse_.x = 1.0f / size_.x;
	sizeInverse_.y = 1.0f / size_.y;
}
