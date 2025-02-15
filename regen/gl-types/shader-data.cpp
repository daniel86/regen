#include "shader-data.h"
#include "shader-input.h"

using namespace regen;

ShaderDataRaw_rw::ShaderDataRaw_rw(ShaderInput *input, int mapMode) :
	input(input), mapMode(mapMode) {
	if (input) {
		auto mapped = input->mapClientData(mapMode);
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

ShaderDataRaw_rw::~ShaderDataRaw_rw() {
	if (w_index >= 0) {
		input->unmapClientData(ShaderData::WRITE, w_index);
	}
	if (r_index >= 0 && r_index != w_index) {
		input->unmapClientData(ShaderData::READ, r_index);
	}
}

void ShaderDataRaw_rw::unmap() {
	if (w_index >= 0) {
		input->unmapClientData(ShaderData::WRITE, w_index);
		w_index = -1;
	}
	if (r_index >= 0 && r_index != w_index) {
		input->unmapClientData(ShaderData::READ, r_index);
		r_index = -1;
	}
}



ShaderDataRaw_ro::ShaderDataRaw_ro(const ShaderInput *input, int mapMode) :
	input(input), mapMode(mapMode) {
	auto mapped = input->mapClientData(mapMode);
	r = mapped.r;
	r_index = mapped.r_index;
}

ShaderDataRaw_ro::~ShaderDataRaw_ro() {
	if (r_index >= 0) {
		input->unmapClientData(ShaderData::READ, r_index);
	}
}

void ShaderDataRaw_ro::unmap() {
	if (r_index >= 0) {
		input->unmapClientData(ShaderData::READ, r_index);
		r_index = -1;
	}
}
