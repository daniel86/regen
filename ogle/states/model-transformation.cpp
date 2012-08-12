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
  velocity_ = ref_ptr<UniformVec3>::manage(
      new UniformVec3("meshVelocity", 1, Vec3f(0.0f,0.0f,0.0f)));
  joinUniform( ref_ptr<Uniform>::cast(velocity_) );

  modelMat_ = ref_ptr<UniformMat4>::manage(
      new UniformMat4("modelMat", 1, identity4f()));
  joinUniform( ref_ptr<Uniform>::cast(modelMat_) );
}

string ModelTransformationState::name()
{
  return "ModelTransformationState";
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
  Vec3f translation = Vec3f(
        modelMat_->valuePtr().x[12],
        modelMat_->valuePtr().x[13],
        modelMat_->valuePtr().x[14]
  );
  audioSource_->set_position( translation );
}

void ModelTransformationState::updateVelocity(float dt)
{
  if(dt > 1e-6) {
    Vec3f position = Vec3f(
          modelMat_->valuePtr().x[12],
          modelMat_->valuePtr().x[13],
          modelMat_->valuePtr().x[14]
    );
    velocity_->set_value( (position - lastPosition_) / dt );
    lastPosition_ = position;
    if(isAudioSource()) {
      audioSource_->set_velocity( velocity_->value() );
    }
  }
}

void ModelTransformationState::translate(const Vec3f &translation, float dt)
{
  translateMat( modelMat_->valuePtr(), translation );
  updateVelocity(dt);
  if(isAudioSource()) { updateAudioSource(); }
}
void ModelTransformationState::setTranslation(const Vec3f &translation, float dt)
{
  setTranslationMat( modelMat_->valuePtr(), translation );
  updateVelocity(dt);
  if(isAudioSource()) { updateAudioSource(); }
}

void ModelTransformationState::scale(const Vec3f &scaling, float dt)
{
  scaleMat( modelMat_->valuePtr(), scaling );
}

void ModelTransformationState::rotate(const Quaternion &rotation, float dt)
{
  modelMat_->valuePtr() = modelMat_->valuePtr() * rotation.calculateMatrix();
}

void ModelTransformationState::set_modelMat(const Mat4f &m, float dt)
{
  modelMat_->set_value( m );
  updateVelocity(dt);
  if(isAudioSource()) { updateAudioSource(); }
}

void ModelTransformationState::set_modelMat(
    const Vec3f &translation,
    const Quaternion &rotation,
    float dt)
{
  modelMat_->set_value( rotation.calculateMatrix() );
  translate( translation, dt );
}

void ModelTransformationState::set_modelMat(
    const Vec3f &translation,
    const Quaternion &rotation,
    const Vec3f &scaling,
    float dt)
{
  modelMat_->set_value( rotation.calculateMatrix() );
  translate( translation, 0.0f );
  scale( scaling, dt );
}
