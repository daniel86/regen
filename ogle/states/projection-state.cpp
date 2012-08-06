/*
 * projection-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "projection-state.h"

PerspectiveProjection::PerspectiveProjection()
: Projection()
{
  inverseMatrixUniform_ = ref_ptr<UniformMat4>::manage(
      new UniformMat4("inverseProjectionMatrix", 1, identity4f()));
  joinStates(inverseMatrixUniform_);

  fovUniform_ = ref_ptr<UniformFloat>::manage(
      new UniformFloat("fov", 1, 45.0));
  joinStates(fovUniform_);

  nearUniform_ = ref_ptr<UniformFloat>::manage(
      new UniformFloat("near", 1, 1.0));
  joinStates(nearUniform_);

  farUniform_ = ref_ptr<UniformFloat>::manage(
      new UniformFloat("far", 1, 200.0));
  joinStates(farUniform_);

  //viewProjectionMatrixUniform_ = ref_ptr<UniformMat4>::manage(
  //    new UniformMat4("viewProjectionMatrix", 1, identity4f()));
  //inverseViewProjectionMatrixUniform_ = ref_ptr<UniformMat4>::manage(
  //    new UniformMat4("inverseViewProjectionMatrix", 1, identity4f()));
  //joinStates( viewProjectionMatrixUniform_ );
  //joinStates( inverseViewProjectionMatrixUniform_ );
}

void PerspectiveProjection::setPerspective(
    GLfloat fov,
    GLfloat near,
    GLfloat far,
    GLfloat aspect)
{
  fovUniform_->set_value( fov );
  nearUniform_->set_value( near );
  farUniform_->set_value( far );
  aspect_ = aspect;

  matrixUniform_->set_value( projectionMatrix(
          fovUniform_->value(),
          aspect_,
          nearUniform_->value(),
          farUniform_->value())
  );
  inverseMatrixUniform_->set_value(
      projectionMatrixInverse(matrixUniform_->value()));

  // update uniforms
  //viewProjectionMatrixUniform_->set_value(
  //    camera_->viewMatrix() * projectionMatrixUniform_->value());
  //inverseViewProjectionMatrixUniform_->set_value(
  //    inverseProjectionMatrixUniform_->value() * camera_->inverseViewMatrix());

  //frustum_.setProjection(
  //    fovUniform_->value(),
  //    aspect_,
  //    nearUniform_->value(),
  //    farUniform_->value());
  //frustum_.calculatePoints(
  //    camera_->position(),
  //    camera_->direction(),
  //    UP_VECTOR);
}

/*
void PerspectiveProjection::updateTransformationMatrices()
{
  // update uniforms
  viewMatrixUniform_->set_value(camera_->viewMatrix());
  viewProjectionMatrixUniform_->set_value(
      camera_->viewMatrix() * projectionMatrixUniform_->value());
  inverseViewMatrixUniform_->set_value(camera_->inverseViewMatrix());
  // Note: $PROJ^-1 * VIEW^-1 = (VIEW * PROJ)^-1$
  inverseViewProjectionMatrixUniform_->set_value(
      inverseProjectionMatrixUniform_->value() * camera_->inverseViewMatrix());
  cameraPositionUniform_->set_value( camera_->position() );
  // update frustum
  frustum_.calculatePoints(camera_->position(), camera_->direction(), UP_VECTOR);
}
*/

///////////////////

OrthogonalProjection::OrthogonalProjection(
    GLfloat right, GLfloat top)
: Projection()
{
  matrixUniform_->set_value(getOrthogonalProjectionMatrix(
      0.0, right, 0.0, top, -1.0, 1.0));
}

void OrthogonalProjection::updateProjection(
    GLfloat right, GLfloat top)
{
  matrixUniform_->set_value(getOrthogonalProjectionMatrix(
      0.0, right, 0.0, top, -1.0, 1.0));
}
