#include "gpu-fence.h"
#include <regen/utility/logging.h>

using namespace regen;

GPUFence::~GPUFence() {
	if (fence_) {
		glDeleteSync(fence_);
	}
}

void GPUFence::setFencePoint() {
	if (fence_) {
		glDeleteSync(fence_);
	}
	fence_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

void GPUFence::ensureFencePoint() {
	if (!fence_) {
		setFencePoint();
	}
}

bool GPUFence::wait(bool allowFrameDropping) {
	if (!fence_) {
		return true; // No fence to wait on
	}

	GLenum status = glClientWaitSync(fence_, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
	if (allowFrameDropping) {
		if (status == GL_TIMEOUT_EXPIRED) {
			return false; // Frame dropped
		}
	} else {
		while (status == GL_TIMEOUT_EXPIRED) {
			status = glClientWaitSync(fence_, GL_SYNC_FLUSH_COMMANDS_BIT, 1000); // 1µs timeout
		}
	}

	if (status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED) {
		glDeleteSync(fence_);
		fence_ = nullptr;
	} else {
		REGEN_WARN("Unknown fence status: " << status << " (0x" << std::hex << status << std::dec << ")");
		GL_ERROR_LOG();
	}
	return true;
}

bool GPUFence::isSignaled() {
	if (!fence_) {
		return true; // No fence to check
	}
	GLenum status = glClientWaitSync(fence_, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
	if (status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED) {
		glDeleteSync(fence_);
		fence_ = nullptr;
		return true; // Fence is signaled
	} else if (status == GL_TIMEOUT_EXPIRED) {
		return false; // Fence not signaled yet
	} else {
		REGEN_WARN("Unknown fence status: " << status << " (0x" << std::hex << status << std::dec << ")");
		GL_ERROR_LOG();
		return false; // Error in checking fence status
	}
}
