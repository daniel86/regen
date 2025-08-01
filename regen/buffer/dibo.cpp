#include "dibo.h"

using namespace regen;

DrawIndirectBuffer::DrawIndirectBuffer(const std::string &name, const BufferUpdateFlags &hints) :
		SSBO(name, hints, SSBO::RESTRICT) {
	flags_.target = DRAW_INDIRECT_BUFFER;
	glTarget_ = glBufferTarget(flags_.target);
}
