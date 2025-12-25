#include "vbo.h"

#include "staging-system.h"

using namespace regen;

static std::string getVBO_Name() {
	static uint32_t vboCounter = 0;
	return REGEN_STRING("VBO_" << vboCounter++);
}

static BufferMemoryLayout getVBO_Layout(const BufferUpdateFlags &hints) {
	return hints.compute == BUFFER_NO_COMPUTE ? BUFFER_MEMORY_PACKED : BUFFER_MEMORY_STD430;
}

VBO::VBO(BufferTarget target, const BufferUpdateFlags &hints)
		: StagedBuffer(getVBO_Name(), target, hints, getVBO_Layout(hints)) {
	// set default storage flags
	//flags_.mapMode = BUFFER_MAP_DISABLED;
	// default case: mesh data is loaded from CPU and written to GPU
	//flags_.accessMode = BUFFER_CPU_WRITE;
	// No-op for VBOs
	enableInput_ = [](int /*loc*/) {};
}

ref_ptr<BufferReference> &VBO::alloc(const std::vector<ref_ptr<ShaderInput>> &attributes) {
	// Clean up last allocation
	if (shared_->isGloballyStaged_) {
		StagingSystem::instance().removeBufferBlock(this);
		for (const auto &att: stagedInputs_) {
			clientBuffer_->removeSegment(att.input->clientBuffer());
		}
	}
	estimatedSize_ = 0;
	hasClientData_ = true;
	inputs_.clear();
	stagedInputs_.clear();
	allocations_.clear();

	// Populate the VBO with the given attributes.
	for (const auto &att: attributes) {
		addStagedInput(att);
	}
	updateStagedInputs();

	// Add the VBO to the staging system, create a StagingBuffer instance.
	createStagingBuffer();

	// Create or adopt a buffer range for the total size of all attributes
	// used as a buffer sourced in draw calls.
	updateDrawBuffer();

	REGEN_INFO("Allocated VBO with "
		<< drawBufferRef_->allocatedSize() / 1024.0 << " KiB at buffer "
		<< drawBufferRef_->bufferID() << " offset " << drawBufferRef_->address() <<
		" num vertices: " << numVertices());

	return drawBufferRef_;
}

void VBO::write(std::ostream &out) const {
	out << "VBO \"" << name() << "\" {\n";
	out << "  Size: " << adoptedSize_ << " bytes\n";
	out << "}\n";
}
