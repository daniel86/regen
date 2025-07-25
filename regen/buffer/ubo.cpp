#include "ubo.h"
#include "regen/scene/shader-input-processor.h"

using namespace regen;

UBO::UBO(const std::string &name, const BufferUpdateFlags &hints) :
		BufferBlock(name, UNIFORM_BUFFER, hints, UNIFORM, BUFFER_MEMORY_STD140) {
	enableInput_ = [this](GLint loc) { enableBufferBlock(loc); };
	isBufferBlock_ = true;
	isVertexAttribute_ = false;
	isVertexAttribute_ = false;
}

void UBO::write(std::ostream &out) const {
	out << "uniform " << name() << " {\n";
	out << "};";
}
