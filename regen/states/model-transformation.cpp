/*
 * model-transformation.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "model-transformation.h"
using namespace regen;

ModelTransformation::ModelTransformation()
: ShaderInputState(), lastPosition_(0.0, 0.0, 0.0)
{
  velocity_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("meshVelocity"));
  velocity_->setUniformData(Vec3f(0.0f));
  setInput(velocity_);

  modelMat_ = ref_ptr<ShaderInputMat4>::manage(new ShaderInputMat4("modelMatrix"));
  modelMat_->setUniformData(Mat4f::identity());
  setInput(modelMat_);
}

void ModelTransformation::set_audioSource(const ref_ptr<AudioSource> &audioSource)
{
  audioSource_ = audioSource;
  if(isAudioSource()) { updateAudioSource(); }
}
GLboolean ModelTransformation::isAudioSource() const
{
  return audioSource_.get() != NULL;
}
void ModelTransformation::updateAudioSource()
{
  const Mat4f &val = modelMat_->getVertex16f(0);
  Vec3f translation(val.x[12], val.x[13], val.x[14]);
  audioSource_->set_position( translation );
}

void ModelTransformation::updateVelocity(GLdouble dt)
{
  if(dt > 1e-6) {
    const Mat4f &val = modelMat_->getVertex16f(0);
    Vec3f position(val.x[12], val.x[13], val.x[14]);
    velocity_->setVertex3f(0, (position - lastPosition_) / dt );
    lastPosition_ = position;
    if(isAudioSource()) {
      audioSource_->set_velocity( velocity_->getVertex3f(0) );
    }
  }
}

const ref_ptr<ShaderInputMat4>& ModelTransformation::modelMat() const
{
  return modelMat_;
}

void ModelTransformation::translate(const Vec3f &translation, GLdouble dt)
{
  Mat4f* val = (Mat4f*)modelMat_->dataPtr();
  //val->translate(translation);
  val->x[12] = translation.x;
  val->x[13] = translation.y;
  val->x[14] = translation.z;
  modelMat_->nextStamp();
  updateVelocity(dt);
  if(isAudioSource()) { updateAudioSource(); }
}
void ModelTransformation::setTranslation(const Vec3f &translation, GLdouble dt)
{
  Mat4f* val = (Mat4f*)modelMat_->dataPtr();
  val->setTranslation(translation);
  modelMat_->nextStamp();
  updateVelocity(dt);
  if(isAudioSource()) { updateAudioSource(); }
}
Vec3f ModelTransformation::translation() const
{
  const Mat4f &mat = modelMat_->getVertex16f(0);
  return Vec3f(mat.x[12], mat.x[13], mat.x[14]);
}

void ModelTransformation::scale(const Vec3f &scaling, GLdouble dt)
{
  Mat4f* val = (Mat4f*)modelMat_->dataPtr();
  val->scale(scaling);
  modelMat_->nextStamp();
}

void ModelTransformation::rotate(const Quaternion &rotation, GLdouble dt)
{
  const Mat4f &val = modelMat_->getVertex16f(0);
  modelMat_->setVertex16f(0, val * rotation.calculateMatrix());
}

void ModelTransformation::set_modelMat(const Mat4f &m, GLdouble dt)
{
  modelMat_->setVertex16f(0, m);
  updateVelocity(dt);
  if(isAudioSource()) { updateAudioSource(); }
}

void ModelTransformation::set_modelMat(
    const Vec3f &translation,
    const Quaternion &rotation,
    GLdouble dt)
{
  modelMat_->setVertex16f(0, rotation.calculateMatrix());
  translate( translation, dt );
}

void ModelTransformation::set_modelMat(
    const Vec3f &translation,
    const Quaternion &rotation,
    const Vec3f &scaling,
    GLdouble dt)
{
  modelMat_->setUniformData( rotation.calculateMatrix() );
  translate( translation, 0.0f );
  scale( scaling, dt );
}
