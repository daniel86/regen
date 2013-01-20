/*
 * fog.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef FOG_H_
#define FOG_H_

#include <ogle/states/shader-input-state.h>
#include <ogle/gl-types/shader-input.h>

/**
 * Provides fog related uniforms and enables
 * fog for shaders generated for this state.
 */
class Fog : public ShaderInputState
{
public:
  Fog(GLfloat far);

  void set_fogColor(const Vec4f &color);
  const ref_ptr<ShaderInput4f>& fogColor() const;

  void set_fogEnd(GLfloat end);
  const ref_ptr<ShaderInput1f>& fogEnd() const;

  void set_fogScale(GLfloat scale);
  const ref_ptr<ShaderInput1f>& fogScale() const;
protected:
  ref_ptr<ShaderInput4f> fogColor_;
  ref_ptr<ShaderInput1f> fogEnd_;
  ref_ptr<ShaderInput1f> fogScale_;
};

#endif /* FOG_H_ */
