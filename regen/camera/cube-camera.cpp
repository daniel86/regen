/*
 * cube-camera.cpp
 *
 *  Created on: Dec 15, 2013
 *      Author: daniel
 */

#include "cube-camera.h"
using namespace regen;

CubeCamera::CubeCamera(
    const ref_ptr<Mesh> &mesh,
    const ref_ptr<Camera> &userCamera)
: Camera(GL_FALSE),
  userCamera_(userCamera)
{
  shaderDefine("RENDER_TARGET", "CUBE");
  shaderDefine("RENDER_LAYER", "6");
  updateFrustum(
      userCamera_->fov()->getVertex(0),
      userCamera_->aspect()->getVertex(0),
      userCamera_->near()->getVertex(0),
      userCamera_->far()->getVertex(0),
      GL_FALSE);

  view_->set_elementCount(6);
  view_->setUniformDataUntyped(NULL);
  viewInv_->set_elementCount(6);
  viewInv_->setUniformDataUntyped(NULL);
  proj_->setUniformDataUntyped(NULL);
  projInv_->setUniformDataUntyped(NULL);
  viewproj_->set_elementCount(6);
  viewproj_->setUniformDataUntyped(NULL);
  viewprojInv_->set_elementCount(6);
  viewprojInv_->setUniformDataUntyped(NULL);

  modelMatrix_ = ref_ptr<ShaderInputMat4>::upCast(mesh->findShaderInput("modelMatrix"));
  matrixStamp_ = 0;

  pos_ = ref_ptr<ShaderInput3f>::upCast(mesh->positions());
  positionStamp_ = 0;

  for(GLuint i=0; i<6; ++i) isCubeFaceVisible_[i] = GL_TRUE;

  // initially update shadow maps
  update();
}

void CubeCamera::set_isCubeFaceVisible(GLenum face, GLboolean visible)
{ isCubeFaceVisible_[face - GL_TEXTURE_CUBE_MAP_POSITIVE_X] = visible; }

void CubeCamera::update()
{
  GLuint positionStamp = (pos_.get() == NULL ? 1 : pos_->stamp());
  GLuint matrixStamp = (modelMatrix_.get() == NULL ? 1 : modelMatrix_->stamp());
  if(positionStamp_ == positionStamp && matrixStamp_ == matrixStamp)
  { return; }

  Vec3f pos = Vec3f::zero();
  if(modelMatrix_.get() != NULL) {
    pos = modelMatrix_->getVertex(0).transpose().transformVector(pos);
  }
  position_->setVertex(0,pos);
  // XXX
  direction_->setVertex(0,Vec3f(0.0,0.0,1.0));

  GLfloat near = userCamera_->near()->getVertex(0);
  GLfloat far = userCamera_->far()->getVertex(0);

  proj_->setVertex(0, Mat4f::projectionMatrix(90.0f, 1.0f, near, far));
  projInv_->setVertex(0, proj_->getVertex(0).projectionInverse());
  Mat4f::cubeLookAtMatrices(pos, (Mat4f*)view_->clientDataPtr());

  for(register GLuint i=0; i<6; ++i) {
    if(!isCubeFaceVisible_[i]) { continue; }
    viewInv_->setVertex(i, view_->getVertex(i).lookAtInverse());
    viewproj_->setVertex(i, view_->getVertex(i)*proj_->getVertex(0));
    viewprojInv_->setVertex(i, projInv_->getVertex(0)*viewInv_->getVertex(i));
  }

  positionStamp_ = positionStamp;
  matrixStamp_ = matrixStamp;
}

void CubeCamera::enable(RenderState *rs)
{
  update();
  Camera::enable(rs);
}
