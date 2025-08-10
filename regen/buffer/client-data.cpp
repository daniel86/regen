#include "client-data.h"
#include "client-buffer.h"
#include <regen/buffer/buffer-enums.h>

using namespace regen;

ClientDataRaw_rw::ClientDataRaw_rw(ClientBuffer *clientBuffer, int32_t mapMode, uint32_t offset, uint32_t size) :
	clientBuffer(clientBuffer),
	mapMode(mapMode),
	mapOffset(offset),
	mapSize(size) {
	if (clientBuffer) {
		auto mapped = clientBuffer->mapRange(mapMode, mapOffset, size);
		r = mapped.r;
		w = mapped.w;
		r_index = mapped.r_index;
		w_index = mapped.w_index;
	} else {
		r = nullptr;
		w = nullptr;
		r_index = -1;
		w_index = -1;
	}
}

ClientDataRaw_rw::ClientDataRaw_rw(ClientBuffer *clientBuffer, int32_t mapMode) :
		ClientDataRaw_rw(clientBuffer, mapMode, 0, clientBuffer ? clientBuffer->dataSize() : 0) {
}

ClientDataRaw_rw::~ClientDataRaw_rw() {
	if (w_index >= 0) {
		clientBuffer->unmapRange(BUFFER_GPU_WRITE, mapOffset, mapSize, w_index);
	}
	if (r_index >= 0 && r_index != w_index) {
		clientBuffer->unmapRange(BUFFER_GPU_READ, mapOffset, mapSize, r_index);
	}
}

void ClientDataRaw_rw::unmap() {
	if (w_index >= 0) {
		clientBuffer->unmapRange(BUFFER_GPU_WRITE, mapOffset, mapSize, w_index);
		w_index = -1;
	}
	if (r_index >= 0 && r_index != w_index) {
		clientBuffer->unmapRange(BUFFER_GPU_READ, mapOffset, mapSize, r_index);
		r_index = -1;
	}
}



ClientDataRaw_ro::ClientDataRaw_ro(const ClientBuffer *clientBuffer, int32_t mapMode, uint32_t offset, uint32_t size) :
	clientBuffer(clientBuffer),
	mapMode(mapMode),
	mapOffset(offset),
	mapSize(size) {
	if (clientBuffer) {
		auto mapped = clientBuffer->mapRange(mapMode, mapOffset, mapSize);
		r = mapped.r;
		r_index = mapped.r_index;
	} else {
		r = nullptr;
		r_index = -1;
	}
}

ClientDataRaw_ro::ClientDataRaw_ro(const ClientBuffer *clientBuffer, int32_t mapMode) :
		ClientDataRaw_ro(clientBuffer, mapMode, 0, clientBuffer ? clientBuffer->dataSize() : 0) {
}

ClientDataRaw_ro::~ClientDataRaw_ro() {
	if (r_index != -1) {
		clientBuffer->unmapRange(BUFFER_GPU_READ, mapOffset, mapSize, r_index);
	}
}

void ClientDataRaw_ro::unmap() {
	if (r_index != -1) {
		clientBuffer->unmapRange(BUFFER_GPU_READ, mapOffset, mapSize, r_index);
		r_index = -1;
	}
}
