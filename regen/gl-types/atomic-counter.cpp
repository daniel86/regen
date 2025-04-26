#include "atomic-counter.h"
#include "regen/utility/conversion.h"

using namespace regen;

AtomicCounter::AtomicCounter() :
		BufferObjectT(BUFFER_USAGE_DYNAMIC_DRAW) {
}

BoundingBoxCounter::BoundingBoxCounter() :
		AtomicCounter(),
		bounds_(Vec3f(0.0f), Vec3f(0.0f)) {
	auto max_float = conversion::floatBitsToUint(std::numeric_limits<float>::max());
	auto min_float = conversion::floatBitsToUint(std::numeric_limits<float>::lowest());
	initialData_[0] = max_float;
	initialData_[1] = max_float;
	initialData_[2] = max_float;
	initialData_[3] = min_float;
	initialData_[4] = min_float;
	initialData_[5] = min_float;

	ref_ = allocBytes(sizeof(initialData_));
	setBufferData(ref_, initialData_);
}

Bounds<Vec3f> &BoundingBoxCounter::updateBounds() {
	RenderState::get()->atomicCounterBuffer().push(ref_->bufferID());
	auto *ptr = (GLuint *) glMapBufferRange(
			GL_ATOMIC_COUNTER_BUFFER,
			ref_->address(),
			sizeof(GLuint) * 6,
			GL_MAP_READ_BIT | GL_MAP_WRITE_BIT);

	auto localData = &bounds_.min.x;
	for (int i = 0; i < 6; ++i) {
		localData[i] = conversion::uintToFloat(ptr[i]);
		ptr[i] = initialData_[i];
	}

	glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	RenderState::get()->atomicCounterBuffer().pop();

	return bounds_;
}
