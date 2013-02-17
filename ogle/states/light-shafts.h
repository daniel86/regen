/*
 * light-shafts.h
 *
 *  Created on: 07.02.2013
 *      Author: daniel
 */

#ifndef LIGHT_SHAFTS_H_
#define LIGHT_SHAFTS_H_

class SkyLightShaft : public State
{
public:
  SkyLightShaft();

protected:
  ref_ptr<ShaderInput1f> scatteringDensity_;
  ref_ptr<ShaderInput1f> scatteringSamples_;
  ref_ptr<ShaderInput1f> scatteringExposure_;
  ref_ptr<ShaderInput1f> scatteringDecay_;
  ref_ptr<ShaderInput1f> scatteringWeight_;
};

SkyLightShaft::SkyLightShaft()
: State()
{
  joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
}

#endif /* LIGHT_SHAFTS_H_ */
