#include "tbo.h"

using namespace regen;

TBO::TBO(BufferUsage usage) :
		BufferObject(TEXTURE_BUFFER, usage) {
}

void TBO::setBufferInput(const ref_ptr<regen::ShaderInput> &input) {
	input_ = input;
	tboRef_ = allocBytes(input_->inputSize());
	if (!tboRef_.get()) {
		REGEN_WARN("Unable to allocate TBO.");
		return;
	}

	// attach vbo to texture
	tboTexture_ = ref_ptr<TextureBuffer>::alloc(input->dataType());
	tboTexture_->attach(tboRef_);
}

void TBO::resizeTBO() {
	if (tboRef_.get()) {
		free(tboRef_.get());
	}
	tboRef_ = allocBytes(input_->inputSize());
	if (tboRef_.get()) {
		tboTexture_->attach(tboRef_);
	}
}

ref_ptr<BufferReference>& TBO::tboRef() {
	if (tboRef_.get()) {
		return tboRef_;
	} else if (!allocations_.empty()) {
		return allocations_[0];
	}
	return tboRef_;
}

void TBO::updateTBO() {
	if (!input_.get() || lastStamp_ == input_->stamp()) {
		return;
	}
	lastStamp_ = input_->stamp();
	if (allocatedSize_ < input_->inputSize()) {
		resizeTBO();
	}
	auto ref = tboRef();
	if (!ref.get()) {
		return;
	}
	auto mapped = input_->mapClientDataRaw(ShaderData::READ);
	glNamedBufferSubData(
			ref->bufferID(),
			ref->address(),
			input_->inputSize(),
			mapped.r);
}
