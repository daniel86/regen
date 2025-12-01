#include "compute-state.h"

using namespace regen;

ComputeState::ComputeState() : State() {
	setGroupSize(localSize_.x, localSize_.y, localSize_.z);
}

void ComputeState::setNumWorkUnits(uint32_t x, uint32_t y, uint32_t z) {
	numWorkUnits_.x = x;
	numWorkUnits_.y = y;
	numWorkUnits_.z = z;
	updateNumWorkGroups();
	shaderDefine("CS_WORK_UNITS_X", REGEN_STRING(numWorkUnits_.x));
	shaderDefine("CS_WORK_UNITS_Y", REGEN_STRING(numWorkUnits_.y));
	shaderDefine("CS_WORK_UNITS_Z", REGEN_STRING(numWorkUnits_.z));
}

void ComputeState::setGroupSize(uint32_t x, uint32_t y, uint32_t z) {
	localSize_.x = x;
	localSize_.y = y;
	localSize_.z = z;
	shaderDefine("CS_LOCAL_SIZE_X", REGEN_STRING(localSize_.x));
	shaderDefine("CS_LOCAL_SIZE_Y", REGEN_STRING(localSize_.y));
	shaderDefine("CS_LOCAL_SIZE_Z", REGEN_STRING(localSize_.z));
	updateNumWorkGroups();
}

void ComputeState::updateNumWorkGroups() {
	numWorkGroups_.x = static_cast<uint32_t>(ceil(
		static_cast<float>((numWorkUnits_.x + localSize_.x - 1)) / static_cast<float>(localSize_.x)));
	numWorkGroups_.y = static_cast<uint32_t>(ceil(
		static_cast<float>((numWorkUnits_.y + localSize_.y - 1)) / static_cast<float>(localSize_.y)));
	numWorkGroups_.z = static_cast<uint32_t>(ceil(
		static_cast<float>((numWorkUnits_.z + localSize_.z - 1)) / static_cast<float>(localSize_.z)));
	shaderDefine("CS_NUM_WORK_GROUPS_X", REGEN_STRING(numWorkGroups_.x));
	shaderDefine("CS_NUM_WORK_GROUPS_Y", REGEN_STRING(numWorkGroups_.y));
	shaderDefine("CS_NUM_WORK_GROUPS_Z", REGEN_STRING(numWorkGroups_.z));
}

void ComputeState::dispatch() {
    glDispatchCompute(
    		numWorkGroups_.x,
			numWorkGroups_.y,
			numWorkGroups_.z);
	if (barrierBits_ > 0) {
    	glMemoryBarrier(barrierBits_);
	}
}

ref_ptr<ComputeState> ComputeState::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto compute = ref_ptr<ComputeState>::alloc();
	auto workUnits = input.getValue<Vec3i>("work-units", Vec3i(256,1,1));
	auto groupSize = input.getValue<Vec3i>("group-size", Vec3i(1,1,1));
	compute->setNumWorkUnits(workUnits.x, workUnits.y, workUnits.z);
	compute->setGroupSize(groupSize.x, groupSize.y, groupSize.z);
	return compute;
}
