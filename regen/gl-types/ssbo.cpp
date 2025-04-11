#include "ssbo.h"
#include "ubo.h"
#include "tbo.h"

using namespace regen;

static std::string getName(const BufferObject &other, const std::string &name) {
	if (name.empty()) {
		auto *ssbo = dynamic_cast<const SSBO *>(&other);
		if (ssbo != nullptr) {
			return ssbo->name();
		}
		auto *ubo = dynamic_cast<const UBO *>(&other);
		if (ubo != nullptr) {
			return ubo->name();
		}
		auto *tbo = dynamic_cast<const TBO *>(&other);
		if (tbo != nullptr && tbo->input().get()) {
			return REGEN_STRING("Buffer_" << tbo->input()->name());
		}
	}
	return name;
}

SSBO::SSBO(const std::string &name, BufferUsage usage) :
		BufferBlock(SHADER_STORAGE_BUFFER, usage,
		            BufferBlock::BUFFER,
		            BufferBlock::STD430),
		ShaderInput(name, GL_INVALID_ENUM,
		            0, 0, 0, GL_FALSE) {
	initSSBO();
}

SSBO::SSBO(const BufferObject &other, const std::string &name) :
		BufferBlock(other),
		ShaderInput(getName(other,name), GL_INVALID_ENUM,
		            0, 0, 0, GL_FALSE) {
	target_ = SHADER_STORAGE_BUFFER;
	glTarget_ = glBufferTarget(target_);
	storageQualifier_ = BufferBlock::BUFFER;
	initSSBO();
}

void SSBO::initSSBO() {
	enableInput_ = [this](GLint loc) {
		enableBufferBlock(loc);
	};
	isBufferBlock_ = GL_TRUE;
	isVertexAttribute_ = GL_FALSE;
}

void SSBO::write(std::ostream &out) const {
	out << "buffer " << name() << " {\n";
	out << "};";
}
