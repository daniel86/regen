#include "gl-object.h"

using namespace regen;

GLObject::GLObject(
		CreateObjectFunc createObjects,
		ReleaseObjectFunc releaseObjects,
		uint32_t numObjects)
		: ids_(new uint32_t[numObjects]),
		  copyCounter_(ref_ptr<std::atomic<uint32_t>>::alloc(1)),
		  numObjects_(numObjects),
		  objectIndex_(0),
		  releaseObjects_(releaseObjects),
		  createObjects_(createObjects),
		  createObjects2_(nullptr) {
	createObjects_(static_cast<int>(numObjects_), ids_);
}

GLObject::GLObject(
		CreateObjectFunc2 createObjects,
		ReleaseObjectFunc releaseObjects,
		GLenum objectTarget,
		uint32_t numObjects)
		: ids_(new uint32_t[numObjects]),
		  copyCounter_(ref_ptr<std::atomic<uint32_t>>::alloc(1)),
		  numObjects_(numObjects),
		  objectIndex_(0),
		  objectTarget_(objectTarget),
		  releaseObjects_(releaseObjects),
		  createObjects_(nullptr),
		  createObjects2_(createObjects) {
	createObjects2_(objectTarget_, static_cast<int>(numObjects_), ids_);
}
GLObject::GLObject(const GLObject &o)
		: ids_(o.ids_),
		  copyCounter_(ref_ptr<std::atomic<uint32_t>>::alloc(1)),
		  numObjects_(o.numObjects_),
		  objectIndex_(o.objectIndex_),
		  objectTarget_(o.objectTarget_),
		  releaseObjects_(o.releaseObjects_),
		  createObjects_(o.createObjects_),
		  createObjects2_(o.createObjects2_) {
	// increase the atomic copy counter
	std::atomic<uint32_t> &counter = *(o.copyCounter_.get());
	counter.fetch_add(1);
}

GLObject::~GLObject() {
	// decrease the atomic copy counter
	std::atomic<uint32_t> &counter = *(copyCounter_.get());
	auto oldValue = counter.fetch_sub(1);
	if (oldValue == 1) {
		// we are the last copy, release the resources
		releaseObjects_(static_cast<int>(numObjects_), ids_);
		delete[] ids_;
	}
}

void GLObject::nextObject() {
	objectIndex_ += 1;
	if (objectIndex_ >= numObjects_) {
		objectIndex_ = 0;
	}
}
