/*
 * bones.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include <regen/textures/texture-state.h>
#include <regen/gl-types/gl-util.h>

#include "bones.h"

using namespace regen;

#define USE_BONE_TBO

Bones::Bones(GLuint numBoneWeights, GLuint numBones)
		: HasInputState(TEXTURE_BUFFER, BufferUsage::USAGE_DYNAMIC),
		  Animation(true, true) {
	bufferSize_ = 0u;
	setAnimationName("bones");

	numBoneWeights_ = ref_ptr<ShaderInput1i>::alloc("numBoneWeights");
	numBoneWeights_->setUniformData(numBoneWeights);
	joinShaderInput(numBoneWeights_);

	// prepend '#define HAS_BONES' to loaded shaders
	shaderDefine("HAS_BONES", "TRUE");
	shaderDefine("NUM_BONES_PER_MESH", REGEN_STRING(numBones));
}

void Bones::setBones(const std::list<ref_ptr<AnimationNode> > &bones) {
	GL_ERROR_LOG();
	RenderState *rs = RenderState::get();
	bones_ = bones;
	shaderDefine("NUM_BONES", REGEN_STRING(bones_.size()));

	// create and join bone matrix uniform
	boneMatrices_ = ref_ptr<ShaderInputMat4>::alloc("boneMatrices", bones.size());
	boneMatrices_->set_forceArray(GL_TRUE);
	boneMatrices_->setUniformUntyped();

#ifdef USE_BONE_TBO
	boneMatrixTBO_ = ref_ptr<TBO>::alloc(BufferUsage::USAGE_DYNAMIC);
	boneMatrixTBO_->setBufferInput(boneMatrices_);
	bufferSize_ = boneMatrices_->inputSize();

	// and make the tbo available
	if (texState_.get()) disjoinStates(texState_);
	texState_ = ref_ptr<TextureState>::alloc(boneMatrixTBO_->tboTexture(), "boneMatrices");
	texState_->set_mapping(TextureState::MAPPING_CUSTOM);
	texState_->set_mapTo(TextureState::MAP_TO_CUSTOM);
	joinStates(texState_);
	shaderDefine("USE_BONE_TBO", "TRUE");
#else
	joinShaderInput(boneMatrices_);
	shaderDefine("USE_BONE_TBO", "FALSE");
#endif

	GL_ERROR_LOG();

	// initially calculate the bone matrices
	glAnimate(rs, 0.0f);
}

void Bones::animate(GLdouble dt) {
	if (bufferSize_ <= 0) return;
	auto mapped = boneMatrices_->mapClientData<Mat4f>(ShaderData::WRITE);
	auto *boneMatrixData_ = mapped.w;

	unsigned int i = 0;
	for (auto & bone : bones_) {
		// the bone matrix is actually calculated in the animation thread
		// by NodeAnimation.
		boneMatrixData_[i] = bone->boneTransformationMatrix();
		i += 1;
	}
}

void Bones::glAnimate(RenderState *rs, GLdouble dt) {
	if (bufferSize_ <= 0) return;
#ifdef USE_BONE_TBO
	boneMatrixTBO_->updateTBO();
#endif
}
