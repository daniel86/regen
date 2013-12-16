/*
 * parabolid-camera.cpp
 *
 *  Created on: Dec 16, 2013
 *      Author: daniel
 */

#include "parabolid-camera.h"
using namespace regen;

ParabolidCamera::ParabolidCamera(
    const ref_ptr<Mesh> &mesh,
    const ref_ptr<Camera> &userCamera,
    GLboolean hasBackFace)
: Camera(GL_FALSE),
  userCamera_(userCamera),
  hasBackFace_(hasBackFace)
{
  GLuint numLayer = (hasBackFace ? 2 : 1);
  shaderDefine("RENDER_TARGET", hasBackFace ? "DUAL_PARABOLID" : "PARABOLID");
  shaderDefine("RENDER_LAYER", REGEN_STRING(numLayer));
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
  // For now plane between parabolids is in xy-plane.
  direction_->set_elementCount(numLayer);
  direction_->setUniformDataUntyped(NULL);
  direction_->setVertex(0, Vec3f(0.0,0.0, 1.0));
  direction_->setVertex(1, Vec3f(0.0,0.0,-1.0));

  modelMatrix_ = ref_ptr<ShaderInputMat4>::upCast(mesh->findShaderInput("modelMatrix"));
  pos_ = ref_ptr<ShaderInput3f>::upCast(mesh->positions());
  matrixStamp_ = 0;
  positionStamp_ = 0;

  // initially update shadow maps
  update();
}

void ParabolidCamera::update()
{
  GLuint positionStamp = (pos_.get() == NULL ? 1 : pos_->stamp());
  GLuint matrixStamp = (modelMatrix_.get() == NULL ? 1 : modelMatrix_->stamp());
  if(positionStamp_ == positionStamp && matrixStamp_ == matrixStamp)
  { return; }

  // Compute cube center position.
  Vec3f pos = Vec3f::zero();
  if(modelMatrix_.get() != NULL) {
    pos = modelMatrix_->getVertex(0).transpose().transformVector(pos);
  }
  position_->setVertex(0,pos);

  // Update view matrices
  for(int i=0; i<1+hasBackFace_; ++i) {
    view_->setVertex(i, Mat4f::lookAtMatrix(
        pos,direction_->getVertex(i),Vec3f::up()));
    viewInv_->setVertex(i, view_->getVertex(i).lookAtInverse());
    viewproj_->setVertex(i, view_->getVertex(i));
    viewprojInv_->setVertex(i, viewInv_->getVertex(i));

  }

  positionStamp_ = positionStamp;
  matrixStamp_ = matrixStamp;
}

void ParabolidCamera::enable(RenderState *rs)
{
  update();
  Camera::enable(rs);
}

