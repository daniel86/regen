#include "ubo.h"
#include "regen/scene/shader-input-processor.h"
#include "gl-util.h"

using namespace regen;

UBO::UBO(const std::string &name, BufferUsage usage) :
		BufferBlock(UNIFORM_BUFFER, usage, UNIFORM, STD140),
		ShaderInput(name, GL_INVALID_ENUM, 0, 0, 0, false) {
	enableInput_ = [this](GLint loc) { enableBufferBlock(loc); };
	isBufferBlock_ = true;
	isVertexAttribute_ = false;
	isVertexAttribute_ = false;
}

void UBO::write(std::ostream &out) const {
	out << "uniform " << name() << " {\n";
	out << "};";
}
