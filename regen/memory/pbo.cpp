#include "regen/gl/render-state.h"
#include "regen/shader/shader-input.h"
#include <optional>

#include "pbo.h"

using namespace regen;

PBO::PBO(unsigned int numBuffers)
		: GLObject(glGenBuffers, glDeleteBuffers, numBuffers) {
}
