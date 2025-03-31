#include "compute-state.h"

using namespace regen;

ComputeState::ComputeState() : State() {
	setGroupSize(localSize_.x, localSize_.y, localSize_.z);
}

void ComputeState::setNumWorkUnits(int x, int y, int z) {
	numWorkUnits_.x = x;
	numWorkUnits_.y = y;
	numWorkUnits_.z = z;
	updateNumWorkGroups();
}

void ComputeState::setGroupSize(int x, int y, int z) {
	localSize_.x = x;
	localSize_.y = y;
	localSize_.z = z;
	shaderDefine("CS_LOCAL_SIZE_X", REGEN_STRING(localSize_.x));
	shaderDefine("CS_LOCAL_SIZE_Y", REGEN_STRING(localSize_.y));
	shaderDefine("CS_LOCAL_SIZE_Z", REGEN_STRING(localSize_.z));
	updateNumWorkGroups();
}

void ComputeState::updateNumWorkGroups() {
	numWorkGroups_.x = (numWorkUnits_.x + localSize_.x - 1) / localSize_.x;
	numWorkGroups_.y = (numWorkUnits_.y + localSize_.y - 1) / localSize_.y;
	numWorkGroups_.z = (numWorkUnits_.z + localSize_.z - 1) / localSize_.z;
}

void ComputeState::enable(RenderState *rs) {
	State::enable(rs);
    glDispatchCompute(
    		numWorkGroups_.x,
			numWorkGroups_.y,
			numWorkGroups_.z);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

ref_ptr<ComputeState> ComputeState::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto compute = ref_ptr<ComputeState>::alloc();
	auto workUnits = input.getValue<Vec3i>("work-units", Vec3i(256,1,1));
	auto groupSize = input.getValue<Vec3i>("group-size", Vec3i(1,1,1));
	compute->setNumWorkUnits(workUnits.x, workUnits.y, workUnits.z);
	compute->setGroupSize(groupSize.x, groupSize.y, groupSize.z);
	return compute;
}
