#include "gpu-fence.h"
#include <regen/utility/logging.h>

using namespace regen;

uint64_t GPUFence::WAIT_TIMEOUT = 1'000'000; // 1ms timeout

GPUFence::~GPUFence() {
	if (fence_) {
		glDeleteSync(fence_);
	}
}

void GPUFence::setFencePoint() {
	if (fence_) {
		// Check if an old fence exists that did not signal yet.
		// If this is the case, we skip creating a new fence
		GLenum status = glClientWaitSync(fence_, 0, 0);
		if (status == GL_ALREADY_SIGNALED || status == GL_CONDITION_SATISFIED) {
			glDeleteSync(fence_);
			fence_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		}
	} else {
		fence_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	}
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
	// poll the fence status
	GLenum status = glClientWaitSync(fence_, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
	if (allowFrameDropping) {
		if (status == GL_TIMEOUT_EXPIRED) {
			return false; // Frame dropped
		}
	} else {
		// keep waiting and polling until the fence is signaled or an error occurs.
		// Note: we use a wait timeout here to avoid wasting too much time in the loop.
		while (status == GL_TIMEOUT_EXPIRED) {
			status = glClientWaitSync(fence_,
				GL_SYNC_FLUSH_COMMANDS_BIT, WAIT_TIMEOUT);
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
