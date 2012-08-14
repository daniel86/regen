/*
 * tesselation-state.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef TESSELATION_STATE_H_
#define TESSELATION_STATE_H_

#include <ogle/states/state.h>

/**
 * Provides tesselation configuration and LoD uniform.
 */
class TesselationState : public State
{
public:
  TesselationState(
      const Tesselation &cfg=Tesselation(TESS_PRIMITVE_TRIANGLES, 3));
  /**
   * Tesselation has a range for its levels, maxLevel is currently 64.0.
   * If you set the factor to 0.5 the range will be clamped to [1,maxLevel*0.5]
   * If you set the factor to 32.0 the range will be clamped to [32,maxLevel].
   */
  void set_lodFactor(float factor);
  float lodFactor() const;

  virtual void configureShader(ShaderConfiguration *cfg);

  virtual string name();
protected:
  Tesselation tessConfig_;
  ref_ptr<UniformFloat> lodFactor_;
  ref_ptr<Callable> tessPatchVerticesSetter_;
  ref_ptr<Callable> tessLevelSetter_;
};

#endif /* TESSELATION_STATE_H_ */
