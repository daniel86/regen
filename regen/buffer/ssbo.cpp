#include "ssbo.h"
#include "tbo.h"

using namespace regen;

SSBO::SSBO(const std::string &name, const BufferUpdateFlags &hints, int memoryMask) :
		BufferBlock(name, SHADER_STORAGE_BUFFER, hints,
		            BufferBlock::BUFFER,
		            BUFFER_MEMORY_STD430),
		memoryMask_(memoryMask) {
}

SSBO::SSBO(const StagedBuffer &other, const std::string &name) :
		BufferBlock(other, name) {
	flags_.target = SHADER_STORAGE_BUFFER;
	glTarget_ = glBufferTarget(flags_.target);
	blockQualifier_ = BufferBlock::BUFFER;
	auto *otherSSBO = dynamic_cast<const SSBO *>(&other);
	if (otherSSBO != nullptr) {
		memoryMask_ = otherSSBO->memoryMask_;
	} else {
		memoryMask_ = 0;
	}
}

void SSBO::write(std::ostream &out) const {
	out << "buffer " << name() << " {\n";
	out << "};";
}
