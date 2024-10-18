/*
 * buffer-object.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "gl-object.h"

using namespace regen;

#include <regen/gl-types/shader-input.h>

GLObject::GLObject(
		CreateObjectFunc createObjects,
		ReleaseObjectFunc releaseObjects,
		GLuint numObjects)
		: ids_(new GLuint[numObjects]),
		  numObjects_(numObjects),
		  objectIndex_(0),
		  releaseObjects_(releaseObjects),
		  createObjects_(createObjects) {
	createObjects_(numObjects_, ids_);
}

GLObject::GLObject(const GLObject &o)
		: ids_(new GLuint[o.numObjects_]),
		  numObjects_(o.numObjects_),
		  objectIndex_(o.objectIndex_),
		  releaseObjects_(o.releaseObjects_),
		  createObjects_(o.createObjects_) {
	createObjects_(numObjects_, ids_);
}

GLObject::~GLObject() {
	// XXX: The deleted object could be part of RenderState.
	//  After releasing the name another object with the same name could be generated.
	//  Then the new object may is never activated.
	releaseObjects_(numObjects_, ids_);
	delete[] ids_;
}

void GLObject::nextObject() { objectIndex_ = (objectIndex_ + 1) % numObjects_; }

GLuint GLObject::objectIndex() const { return objectIndex_; }

void GLObject::set_objectIndex(GLuint bufferIndex) { objectIndex_ = bufferIndex % numObjects_; }

GLuint GLObject::numObjects() const { return numObjects_; }

GLuint GLObject::id() const { return ids_[objectIndex_]; }

GLuint *GLObject::ids() const { return ids_; }

/////////////

GLRectangle::GLRectangle(
		CreateObjectFunc createObjects,
		ReleaseObjectFunc releaseObjects,
		GLuint numObjects)
		: GLObject(createObjects, releaseObjects, numObjects) {
	size_ = ref_ptr<ShaderInput2f>::alloc("rectangleSize");
	sizeInverse_ = ref_ptr<ShaderInput2f>::alloc("rectangleSizeInverse");
	size_->setUniformDataUntyped(NULL);
	sizeInverse_->setUniformDataUntyped(NULL);
	set_rectangleSize(2, 2);
}

void GLRectangle::set_rectangleSize(GLuint width, GLuint height) {
	size_->setVertex(0, Vec2f(width, height));
	sizeInverse_->setVertex(0, Vec2f(1.0 / width, 1.0 / height));
}

const ref_ptr<ShaderInput2f> &GLRectangle::sizeInverse() const { return sizeInverse_; }

const ref_ptr<ShaderInput2f> &GLRectangle::size() const { return size_; }

GLuint GLRectangle::width() const { return size_->getVertex(0).x; }

GLuint GLRectangle::height() const { return size_->getVertex(0).y; }
