#include "gl-object.h"

using namespace regen;

GLObject::GLObject(
		CreateObjectFunc createObjects,
		ReleaseObjectFunc releaseObjects,
		GLuint numObjects)
		: ids_(new GLuint[numObjects]),
		  numObjects_(numObjects),
		  objectIndex_(0),
		  releaseObjects_(releaseObjects),
		  createObjects_(createObjects),
		  createObjects2_(nullptr) {
	createObjects_(numObjects_, ids_);
}

GLObject::GLObject(
		CreateObjectFunc2 createObjects,
		ReleaseObjectFunc releaseObjects,
		GLenum objectTarget,
		GLuint numObjects)
		: ids_(new GLuint[numObjects]),
		  numObjects_(numObjects),
		  objectIndex_(0),
		  objectTarget_(objectTarget),
		  releaseObjects_(releaseObjects),
		  createObjects_(nullptr),
		  createObjects2_(createObjects) {
	createObjects2_(objectTarget_, numObjects_, ids_);
}

GLObject::GLObject(const GLObject &o)
		: ids_(new GLuint[o.numObjects_]),
		  numObjects_(o.numObjects_),
		  objectIndex_(o.objectIndex_),
		  objectTarget_(o.objectTarget_),
		  releaseObjects_(o.releaseObjects_),
		  createObjects_(o.createObjects_),
		  createObjects2_(o.createObjects2_) {
	if (createObjects2_ != nullptr) {
		createObjects2_(objectTarget_, numObjects_, ids_);
	} else if (createObjects_ != nullptr) {
		createObjects_(numObjects_, ids_);
	} else {
		REGEN_ERROR("No create function defined for GLObject copy constructor.");
	}
}

GLObject::~GLObject() {
	// XXX: The deleted object could be part of RenderState.
	//  After releasing the name another object with the same name could be generated.
	//  Then the new object may is never activated.
	releaseObjects_(numObjects_, ids_);
	delete[] ids_;
}

void GLObject::nextObject() {
	objectIndex_ += 1;
	if (objectIndex_ >= numObjects_) {
		objectIndex_ = 0;
	}
}

GLuint GLObject::objectIndex() const { return objectIndex_; }

void GLObject::set_objectIndex(GLuint bufferIndex) { objectIndex_ = bufferIndex % numObjects_; }

GLuint GLObject::numObjects() const { return numObjects_; }

GLuint GLObject::id() const { return ids_[objectIndex_]; }

GLuint *GLObject::ids() const { return ids_; }
