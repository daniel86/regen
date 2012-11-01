/*
 * fog.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef FOG_H_
#define FOG_H_

#include <ogle/states/state.h>
#include <ogle/gl-types/shader-input.h>

/**
 * Provides fog related uniforms and enables
 * fog for shaders generated below this state.
 */
class Fog : public State
{
public:
  Fog(GLfloat far);

  void set_fogColor(const Vec4f &color);
  void set_fogEnd(GLfloat end);
  void set_fogScale(GLfloat scale);

  ref_ptr<ShaderInput4f>& fogColor();
  ref_ptr<ShaderInput1f>& fogEnd();
  ref_ptr<ShaderInput1f>& fogScale();

  virtual void configureShader(ShaderConfig *cfg);
protected:
  ref_ptr<ShaderInput4f> fogColor_;
  ref_ptr<ShaderInput1f> fogEnd_;
  ref_ptr<ShaderInput1f> fogScale_;
};

#endif /* FOG_H_ */
