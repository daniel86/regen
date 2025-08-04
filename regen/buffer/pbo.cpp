#include "regen/gl-types/render-state.h"
#include "regen/glsl/shader-input.h"
#include <optional>

#include "pbo.h"

using namespace regen;

PBO::PBO(unsigned int numBuffers)
		: GLObject(glGenBuffers, glDeleteBuffers, numBuffers) {
}
