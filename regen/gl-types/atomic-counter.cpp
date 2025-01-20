#include "atomic-counter.h"

using namespace regen;

float uintBitsToFloat(uint32_t uintValue) {
    union {
        uint32_t uintValue;
        float floatValue;
    } converter;
    converter.uintValue = uintValue;
    return converter.floatValue;
}

unsigned int floatBitsToUint(float floatValue) {
	union {
		uint32_t uintValue;
		float floatValue;
	} converter;
	converter.floatValue = floatValue;
	return converter.uintValue;
}

AtomicCounter::AtomicCounter() :
	GLBuffer(GL_ATOMIC_COUNTER_BUFFER) {
}

BoundingBoxCounter::BoundingBoxCounter() :
	AtomicCounter(),
	bounds_(Vec3f(0.0f), Vec3f(0.0f)) {
	auto max_float = floatBitsToUint(std::numeric_limits<float>::max());
	auto min_float = floatBitsToUint(std::numeric_limits<float>::min());
	initialData_[0] = max_float;
	initialData_[1] = max_float;
	initialData_[2] = max_float;
	initialData_[3] = min_float;
	initialData_[4] = min_float;
	initialData_[5] = min_float;

	RenderState::get()->atomicCounterBuffer().push(id());
	glBufferData(GL_ATOMIC_COUNTER_BUFFER,
			sizeof(initialData_), initialData_,
			GL_DYNAMIC_DRAW);
	RenderState::get()->atomicCounterBuffer().pop();
}

Bounds<Vec3f>& BoundingBoxCounter::updateBounds() {
	RenderState::get()->atomicCounterBuffer().push(id());
	auto *ptr = (GLuint *) glMapBufferRange(
			GL_ATOMIC_COUNTER_BUFFER,
			0,
			sizeof(GLuint) * 6,
			GL_MAP_READ_BIT  | GL_MAP_WRITE_BIT);

	auto localData = &bounds_.min.x;
	for (int i = 0; i < 6; ++i) {
		localData[i] = uintBitsToFloat(ptr[i]);
		ptr[i] = initialData_[i];
	}

	glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	RenderState::get()->atomicCounterBuffer().pop();

	return bounds_;
}
