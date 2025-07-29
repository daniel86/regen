#include "ubo.h"
#include "regen/scene/shader-input-processor.h"

using namespace regen;

UBO::UBO(const std::string &name, const BufferUpdateFlags &hints) :
		BufferBlock(name, UNIFORM_BUFFER, hints, UNIFORM, BUFFER_MEMORY_STD140) {
}

UBO::UBO(const UBO &other, const std::string &forcedBlockName, const std::string &inputPrefixToAdd) :
		BufferBlock(other, forcedBlockName, inputPrefixToAdd) {
}

void UBO::write(std::ostream &out) const {
	out << "uniform " << name() << " {\n";
	out << "};";
}
