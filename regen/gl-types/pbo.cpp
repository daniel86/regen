#include <regen/gl-types/render-state.h>
#include <regen/gl-types/shader-input.h>
#include <optional>

#include "pbo.h"

using namespace regen;

PBO::PBO(unsigned int numBuffers)
		: GLObject(glGenBuffers, glDeleteBuffers, numBuffers) {
}

void PBO::bindPackBuffer(unsigned int index) const {
	RenderState::get()->pixelPackBuffer().push(ids_[index]);
}

void PBO::bindUnpackBuffer(unsigned int index) const {
	RenderState::get()->pixelUnpackBuffer().push(ids_[index]);
}
