#include <regen/textures/texture-state.h>

#include "bones.h"

using namespace regen;

#define USE_BONE_TBO

Bones::Bones(const ref_ptr<BoneTree> &tree,
			const std::vector<ref_ptr<BoneNode>> &boneNodes,
			uint32_t numBoneWeights)
		: State(),
		  Animation(false, true),
		  boneTree_(tree),
		  boneNodes_(boneNodes) {
	const uint32_t numBones = static_cast<uint32_t>(boneNodes_.size());

	bufferSize_ = 0u;
	numInstances_ = boneTree_->numInstances();
	setAnimationName("bones");

	numBoneWeights_ = ref_ptr<ShaderInput1i>::alloc("numBoneWeights");
	numBoneWeights_->setUniformData(numBoneWeights);
	setInput(numBoneWeights_);

	// prepend '#define HAS_BONES' to loaded shaders
	shaderDefine("HAS_BONES", "TRUE");
	shaderDefine("NUM_BONES_PER_MESH", REGEN_STRING(numBones));
	shaderDefine("NUM_BONES", REGEN_STRING(numBones));

	// create and join bone matrix uniform
	boneMatrices_ = ref_ptr<ShaderInputMat4>::alloc("boneMatrices", numBones * numInstances_);
	boneMatrices_->set_forceArray(true);
	boneMatrices_->setUniformUntyped();
	bufferSize_ = boneMatrices_->inputSize();

#ifdef USE_BONE_TBO
	boneMatrixTBO_ = ref_ptr<TBO>::alloc("Bones",
			boneMatrices_->dataType(),
			BufferUpdateFlags::FULL_PER_FRAME);
	boneMatrixTBO_->setClientAccessMode(BUFFER_CPU_WRITE);
	boneMatrixTBO_->setBufferMapMode(BUFFER_MAP_DISABLED);
	boneMatrixTBO_->addStagedInput(boneMatrices_);
	boneMatrixTBO_->update();

	// and make the tbo available
	if (texState_.get()) disjoinStates(texState_);
	texState_ = ref_ptr<TextureState>::alloc(boneMatrixTBO_->tboTexture(), "boneMatrices");
	texState_->set_mapping(TextureState::MAPPING_CUSTOM);
	texState_->set_mapTo(TextureState::MAP_TO_CUSTOM);
	joinStates(texState_);
	shaderDefine("USE_BONE_TBO", "TRUE");
#else
	setInput(boneMatrices_);
	shaderDefine("USE_BONE_TBO", "FALSE");
#endif
	GL_ERROR_LOG();
}

void Bones::cpuUpdate(double dt) {
	if (bufferSize_ <= 0) return;
	auto mapped = boneMatrices_->mapClientData<Mat4f>(BUFFER_GPU_WRITE);
	auto *boneMatrixData_ = mapped.w.data();

	uint32_t matIdx = 0;
	for (uint32_t instanceID=0u; instanceID < numInstances_; ++instanceID) {
		for (auto &boneNode : boneNodes_) {
			boneMatrixData_[matIdx++] = boneTree_->boneMatrix(instanceID, boneNode->nodeIdx);
		}
	}
}
