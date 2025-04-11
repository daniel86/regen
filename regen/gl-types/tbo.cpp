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
	auto rs = RenderState::get();
	rs->textureBuffer().push(tboRef_->bufferID());
	tboTexture_ = ref_ptr<TextureBuffer>::alloc(input->dataType());
	tboTexture_->begin(rs);
	tboTexture_->attach(tboRef_);
	tboTexture_->end(rs);
	rs->textureBuffer().pop();
}

void TBO::resizeTBO() {
	if (tboRef_.get()) {
		free(tboRef_.get());
	}
	tboRef_ = allocBytes(input_->inputSize());
	if (tboRef_.get()) {
		auto rs = RenderState::get();
		tboTexture_->begin(rs);
		tboTexture_->attach(tboRef_);
		tboTexture_->end(rs);
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
	auto rs = RenderState::get();
	rs->textureBuffer().push(ref->bufferID());
	auto mapped = input_->mapClientDataRaw(ShaderData::READ);
	glBufferSubData(
			GL_TEXTURE_BUFFER,
			ref->address(),
			input_->inputSize(),
			mapped.r);
	rs->textureBuffer().pop();
}
