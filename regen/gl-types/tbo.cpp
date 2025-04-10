#include "tbo.h"

using namespace regen;

TBO::TBO(BufferUsage usage) :
		BufferObject(TEXTURE_BUFFER, usage) {
}

void TBO::setBufferData(const ref_ptr<regen::ShaderInput> &input) {
	input_ = input;
	if (allocations_.empty()) return;
	auto mapped = input_->mapClientDataRaw(ShaderData::READ);
	auto ref = allocations_[0];
	auto inputSize = input_->inputSize();
	glBufferSubData(GL_TEXTURE_BUFFER, ref->address(), inputSize, mapped.w);
}
