/*
 * model-transformation.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "model-transformation.h"
#include "regen/gl-types/uniform-block.h"

using namespace regen;

ModelTransformation::ModelTransformation()
		: State(),
		  HasInput(VBO::USAGE_DYNAMIC),
		  lastPosition_(0.0, 0.0, 0.0) {
	modelMat_ = ref_ptr<ShaderInputMat4>::alloc("modelMatrix");
	modelMat_->setUniformData(Mat4f::identity());

	velocity_ = ref_ptr<ShaderInput3f>::alloc("meshVelocity");
	velocity_->setUniformData(Vec3f(0.0f));

	auto uniforms = ref_ptr<UniformBlock>::alloc("ModelTransformation");
	uniforms->addUniform(modelMat_);
	uniforms->addUniform(velocity_);
	setInput(uniforms);
}

const ref_ptr<ShaderInputMat4> &ModelTransformation::get() const { return modelMat_; }

void ModelTransformation::set_audioSource(const ref_ptr<AudioSource> &audioSource) { audioSource_ = audioSource; }

GLboolean ModelTransformation::isAudioSource() const { return audioSource_.get() != nullptr; }

void ModelTransformation::enable(RenderState *rs) {
	if (isAudioSource()) {
		boost::posix_time::ptime time(
				boost::posix_time::microsec_clock::local_time());
		GLdouble dt = ((GLdouble) (time - lastTime_).total_microseconds()) / 1000.0;
		lastTime_ = time;

		if (dt > 1e-6) {
			const Mat4f &val = modelMat_->getVertex(0);
			velocity_->setVertex(0, (val.position() - lastPosition_) / dt);
			lastPosition_ = val.position();
			if (isAudioSource()) {
				audioSource_->set3f(AL_VELOCITY, velocity_->getVertex(0));
			}
			audioSource_->set3f(AL_POSITION, modelMat_->getVertex(0).position());
		}
	}
	State::enable(rs);
}
