#include "client-allocator.h"
#include "regen/regen.h"

using namespace regen;

ClientBufferRef ClientBufferAllocator::createAllocator(uint32_t /*poolIdx*/, uint32_t size) {
	return (byte*)std::malloc(size);
}

void ClientBufferAllocator::deleteAllocator(uint32_t /*poolIndex*/, ClientBufferRef ref) {
	if (!ref) return;
	std::free(ref);
}

void* ClientBufferAllocator::mapAllocator(uint32_t /*poolIdx*/, uint32_t /*size*/, ClientBufferRef ref) {
	return ref;
}

void ClientBufferAllocator::unmapAllocator(uint32_t /*poolIdx*/, ClientBufferRef /*ref*/) {
	// nothing to do here.
}

void ClientBufferAllocator::orphanAllocatorRange(ClientBufferRef /*ref*/, uint32_t /*offset*/, uint32_t /*size*/) {
	// nothing to do here.
}
