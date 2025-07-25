#include "shader-data.h"
#include "shader-input.h"

using namespace regen;

ShaderDataRaw_rw::ShaderDataRaw_rw(ClientBuffer *clientBuffer, int32_t mapMode, uint32_t offset, uint32_t size) :
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

ShaderDataRaw_rw::ShaderDataRaw_rw(ClientBuffer *clientBuffer, int32_t mapMode) :
	ShaderDataRaw_rw(clientBuffer, mapMode, 0, clientBuffer ? clientBuffer->dataSize() : 0) {
}

ShaderDataRaw_rw::~ShaderDataRaw_rw() {
	if (w_index >= 0) {
		clientBuffer->unmapRange(ClientMappingMode::WRITE, mapOffset, mapSize, w_index);
	}
	if (r_index >= 0 && r_index != w_index) {
		clientBuffer->unmapRange(ClientMappingMode::READ, mapOffset, mapSize, r_index);
	}
}

void ShaderDataRaw_rw::unmap() {
	if (w_index >= 0) {
		clientBuffer->unmapRange(ClientMappingMode::WRITE, mapOffset, mapSize, w_index);
		w_index = -1;
	}
	if (r_index >= 0 && r_index != w_index) {
		clientBuffer->unmapRange(ClientMappingMode::READ, mapOffset, mapSize, r_index);
		r_index = -1;
	}
}



ShaderDataRaw_ro::ShaderDataRaw_ro(const ClientBuffer *clientBuffer, int32_t mapMode, uint32_t offset, uint32_t size) :
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

ShaderDataRaw_ro::ShaderDataRaw_ro(const ClientBuffer *clientBuffer, int32_t mapMode) :
	ShaderDataRaw_ro(clientBuffer, mapMode, 0, clientBuffer ? clientBuffer->dataSize() : 0) {
}

ShaderDataRaw_ro::~ShaderDataRaw_ro() {
	if (r_index >= 0) {
		clientBuffer->unmapRange(ClientMappingMode::READ, mapOffset, mapSize, r_index);
	}
}

void ShaderDataRaw_ro::unmap() {
	if (r_index >= 0) {
		clientBuffer->unmapRange(ClientMappingMode::READ, mapOffset, mapSize, r_index);
		r_index = -1;
	}
}
