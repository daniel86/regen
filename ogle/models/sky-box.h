/*
 * sky-box.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef SKY_BOX_H_
#define SKY_BOX_H_

#include <ogle/models/cube.h>
#include <ogle/states/camera.h>
#include <ogle/states/texture-state.h>

/**
 * Sky boxes are not translated by camera movement.
 * They are always centered at (0,0,0) in view space.
 */
class SkyBox : public UnitCube
{
public:
  SkyBox(
      ref_ptr<Camera> cam,
      ref_ptr<Texture> tex,
      GLfloat far);
  void resize(GLfloat far);

  // override
  virtual void draw(GLuint numInstances);
  virtual void configureShader(ShaderConfig *cfg);
protected:
  ref_ptr<Texture> tex_;
  ref_ptr<Camera> cam_;
};

#endif /* SKY_BOX_H_ */
