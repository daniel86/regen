#include "vertex-buffer.h"
#include "staging-system.h"

using namespace regen;

static BufferMemoryLayout getMemoryLayout(VertexLayout layout) {
	switch (layout) {
		case regen::VERTEX_LAYOUT_INTERLEAVED:
			return BUFFER_MEMORY_INTERLEAVED;
		case regen::VERTEX_LAYOUT_SEQUENTIAL:
			return BUFFER_MEMORY_PACKED;
	}
	return BUFFER_MEMORY_PACKED;
}

VertexBuffer::VertexBuffer(
		const std::string &name,
		const BufferUpdateFlags &hints,
		VertexLayout vertexLayout)
		: StagedBuffer(name, ARRAY_BUFFER, hints, getMemoryLayout(vertexLayout)) {
	enableInput_ = [this](int loc) {};
	adoptBufferRange_ = [this](uint32_t requiredSize) {
		return adoptBufferRange(requiredSize);
	};
	isBufferBlock_ = true;
}

VertexBuffer::VertexBuffer(const StagedBuffer &other, const std::string &name)
		: StagedBuffer(other, name) {
	enableInput_ = [this](int loc) {};
	isBufferBlock_ = true;
	isVertexAttribute_ = false;
}

VertexBuffer::~VertexBuffer() = default;
