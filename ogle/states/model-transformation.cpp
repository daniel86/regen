/*
 * model-transformation-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "model-transformation.h"

ModelTransformationState::ModelTransformationState()
: State(),
  lastPosition_(0.0, 0.0, 0.0)
{
  velocity_ = ref_ptr<ShaderInput3f>::manage(
      new ShaderInput3f("meshVelocity"));
  velocity_->setUniformData(Vec3f(0.0f));
  joinShaderInput( ref_ptr<ShaderInput>::cast(velocity_) );

  modelMat_ = ref_ptr<ShaderInputMat4>::manage(
      new ShaderInputMat4("modelMatrix"));
  modelMat_->setUniformData(identity4f());
  joinShaderInput( ref_ptr<ShaderInput>::cast(modelMat_) );
}

void ModelTransformationState::set_audioSource(ref_ptr<AudioSource> audioSource)
{
  audioSource_ = audioSource;
  if(isAudioSource()) { updateAudioSource(); }
}
bool ModelTransformationState::isAudioSource() const
{
  return audioSource_.get() != NULL;
}
void ModelTransformationState::updateAudioSource()
{
  Mat4f &val = modelMat_->getVertex16f(0);
  Vec3f translation(val.x[12], val.x[13], val.x[14]);
  audioSource_->set_position( translation );
}

void ModelTransformationState::updateVelocity(float dt)
{
  if(dt > 1e-6) {
    Mat4f &val = modelMat_->getVertex16f(0);
    Vec3f position(val.x[12], val.x[13], val.x[14]);
    velocity_->setUniformData( (position - lastPosition_) / dt );
    lastPosition_ = position;
    if(isAudioSource()) {
      audioSource_->set_velocity( velocity_->getVertex3f(0) );
    }
  }
}

void ModelTransformationState::translate(const Vec3f &translation, float dt)
{
  translateMat( modelMat_->getVertex16f(0), translation );
  updateVelocity(dt);
  if(isAudioSource()) { updateAudioSource(); }
}
void ModelTransformationState::setTranslation(const Vec3f &translation, float dt)
{
  setTranslationMat( modelMat_->getVertex16f(0), translation );
  updateVelocity(dt);
  if(isAudioSource()) { updateAudioSource(); }
}

void ModelTransformationState::scale(const Vec3f &scaling, float dt)
{
  scaleMat( modelMat_->getVertex16f(0), scaling );
}

void ModelTransformationState::rotate(const Quaternion &rotation, float dt)
{
  modelMat_->getVertex16f(0) = modelMat_->getVertex16f(0) * rotation.calculateMatrix();
}

void ModelTransformationState::set_modelMat(const Mat4f &m, float dt)
{
  modelMat_->setUniformData( m );
  updateVelocity(dt);
  if(isAudioSource()) { updateAudioSource(); }
}

void ModelTransformationState::set_modelMat(
    const Vec3f &translation,
    const Quaternion &rotation,
    float dt)
{
  modelMat_->setUniformData( rotation.calculateMatrix() );
  translate( translation, dt );
}

void ModelTransformationState::set_modelMat(
    const Vec3f &translation,
    const Quaternion &rotation,
    const Vec3f &scaling,
    float dt)
{
  modelMat_->setUniformData( rotation.calculateMatrix() );
  translate( translation, 0.0f );
  scale( scaling, dt );
}
