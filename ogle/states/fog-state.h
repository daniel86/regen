/*
 * fog.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef FOG_H_
#define FOG_H_

#include <ogle/states/state.h>
#include <ogle/gl-types/uniform.h>

/**
 * Provides fog related uniforms and enables
 * fog for shaders generated below this state.
 */
class Fog : public State
{
public:
  Fog(GLfloat far);

  void set_fogColor(const Vec4f &color);
  void set_fogEnd(float end);
  void set_fogScale(float scale);

  virtual void configureShader(ShaderConfiguration *cfg);
protected:
  ref_ptr<UniformVec4> fogColorUniform_;
  ref_ptr<UniformFloat> fogEndUniform_;
  ref_ptr<UniformFloat> fogScaleUniform_;
};

#endif /* FOG_H_ */
