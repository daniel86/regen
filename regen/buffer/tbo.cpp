#include "tbo.h"

using namespace regen;

TBO::TBO(const std::string &name, GLenum texelFormat, const BufferUpdateFlags &hints)
		: StagedBuffer(name, TEXTURE_BUFFER, hints, BUFFER_MEMORY_PACKED) {
	enableInput_ = [](int loc) {
		// nothing to do here for the moment.
		// TBOs are bound b texture state which is handled externally.
	};
	adoptBufferRange_ = [this](uint32_t requiredSize) {
		auto ref = adoptBufferRange(requiredSize);
		tboTexture_->attach(ref);
		return ref;
	};
	tboTexture_ = ref_ptr<TextureBuffer>::alloc(texelFormat);
}

void TBO::write(std::ostream &out) const {
	out << "tbo-" << name();
}
