#include "gl-rectangle.h"

using namespace regen;

GLRectangle::GLRectangle(
		CreateObjectFunc createObjects,
		ReleaseObjectFunc releaseObjects,
		GLuint numObjects)
		: GLObject(createObjects, releaseObjects, numObjects) {
	size_ = ref_ptr<ShaderInput2f>::alloc("rectangleSize");
	sizeInverse_ = ref_ptr<ShaderInput2f>::alloc("rectangleSizeInverse");
	size_->setUniformUntyped();
	sizeInverse_->setUniformUntyped();
	set_rectangleSize(2, 2);
}

void GLRectangle::set_rectangleSize(GLuint width, GLuint height) {
	size_->setVertex(0, Vec2f(width, height));
	sizeInverse_->setVertex(0, Vec2f(1.0 / width, 1.0 / height));
}

const ref_ptr<ShaderInput2f> &GLRectangle::sizeInverse() const { return sizeInverse_; }

const ref_ptr<ShaderInput2f> &GLRectangle::size() const { return size_; }

GLuint GLRectangle::width() const { return size_->getVertex(0).r.x; }

GLuint GLRectangle::height() const { return size_->getVertex(0).r.y; }
