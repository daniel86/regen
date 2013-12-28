/*
 * paraboloid-camera.cpp
 *
 *  Created on: Dec 16, 2013
 *      Author: daniel
 */

#include "paraboloid-camera.h"
using namespace regen;

ParaboloidCamera::ParaboloidCamera(
    const ref_ptr<Mesh> &mesh,
    const ref_ptr<Camera> &userCamera,
    GLboolean hasBackFace)
: Camera(GL_FALSE),
  userCamera_(userCamera),
  hasBackFace_(hasBackFace)
{
  GLuint numLayer = (hasBackFace ? 2 : 1);
  shaderDefine("RENDER_TARGET", hasBackFace ? "DUAL_PARABOLOID" : "PARABOLOID");
  shaderDefine("RENDER_LAYER", REGEN_STRING(numLayer));
  shaderDefine("USE_PARABOLOID_PROJECTION", "TRUE");
  updateFrustum(180.0f, 1.0f,
      userCamera_->near()->getVertex(0),
      userCamera_->far()->getVertex(0),
      GL_FALSE);

  // Set matrix array size
  view_->set_elementCount(numLayer);
  viewInv_->set_elementCount(numLayer);
  viewproj_->set_elementCount(numLayer);
  viewprojInv_->set_elementCount(numLayer);

  // Allocate matrices
  proj_->setUniformDataUntyped(NULL);
  projInv_->setUniformDataUntyped(NULL);
  view_->setUniformDataUntyped(NULL);
  viewInv_->setUniformDataUntyped(NULL);
  viewproj_->setUniformDataUntyped(NULL);
  viewprojInv_->setUniformDataUntyped(NULL);

  // Projection is calculated in shaders.
  proj_->setVertex(0, Mat4f::identity());
  projInv_->setVertex(0, Mat4f::identity());

  // Initialize directions.
  direction_->set_elementCount(numLayer);
  direction_->setUniformDataUntyped(NULL);
  direction_->setVertex(0, Vec3f(0.0,0.0, 1.0));
  if(hasBackFace_)
    direction_->setVertex(1, Vec3f(0.0,0.0,-1.0));

  modelMatrix_ = ref_ptr<ShaderInputMat4>::upCast(mesh->findShaderInput("modelMatrix"));
  pos_ = ref_ptr<ShaderInput3f>::upCast(mesh->positions());
  nor_ = ref_ptr<ShaderInput3f>::upCast(mesh->normals());
  matrixStamp_ = 0;
  positionStamp_ = 0;
  normalStamp_ = 0;

  // initially update shadow maps
  update();
}

void ParaboloidCamera::update()
{
  GLuint positionStamp = (pos_.get() == NULL ? 1 : pos_->stamp());
  GLuint normalStamp = (nor_.get() == NULL ? 1 : nor_->stamp());
  GLuint matrixStamp = (modelMatrix_.get() == NULL ? 1 : modelMatrix_->stamp());
  if(positionStamp_ == positionStamp &&
     normalStamp_ == normalStamp &&
     matrixStamp_ == matrixStamp)
  { return; }

  // Compute cube center position.
  Vec3f pos = Vec3f::zero();
  if(modelMatrix_.get() != NULL) {
    pos = modelMatrix_->getVertex(0).transpose().transformVector(pos);
  }
  position_->setVertex(0,pos);

  if(nor_.get() != NULL) {
    Vec3f dir = nor_->getVertex(0);
    if(modelMatrix_.get() != NULL) {
      dir = modelMatrix_->getVertex(0).transpose().rotateVector(dir);
    }
    direction_->setVertex(0, -dir);
    if(hasBackFace_) direction_->setVertex(1,dir);
  }

  // Update view matrices
  for(int i=0; i<1+hasBackFace_; ++i) {
    view_->setVertex(i, Mat4f::lookAtMatrix(
        pos,direction_->getVertex(i),Vec3f::up()));
    viewInv_->setVertex(i, view_->getVertex(i).lookAtInverse());
    viewproj_->setVertex(i, view_->getVertex(i));
    viewprojInv_->setVertex(i, viewInv_->getVertex(i));

  }

  positionStamp_ = positionStamp;
  normalStamp_ = normalStamp;
  matrixStamp_ = matrixStamp;
}

void ParaboloidCamera::enable(RenderState *rs)
{
  update();
  Camera::enable(rs);
}

