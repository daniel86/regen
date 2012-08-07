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

class SkyBox : public Cube
{
public:
  SkyBox(
      ref_ptr<Camera> &cam,
      ref_ptr<Texture> &tex,
      GLfloat far);
  virtual void draw();
  void resize(GLfloat far);
protected:
  ref_ptr<Texture> tex_;
  ref_ptr<Camera> cam_;
};

#endif /* SKY_BOX_H_ */
