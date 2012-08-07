/*
 * anti-aliasing.h
 *
 *  Created on: 29.07.2012
 *      Author: daniel
 */

#ifndef ANTI_ALIASING_H_
#define ANTI_ALIASING_H_

#include <ogle/shader/filter-shader.h>

struct FXAAConfig {
  float spanMax;
  float reduceMin;
  float reduceMul;
  float subpixShift;
  /**
   * Trims the algorithm from processing darks.
   *   1/32 - visible limit,
   *   1/16 - high quality,
   *   1/12 - upper limit (start of visible unfiltered edges)
   */
  float edgeThresholdMin;
  /**
   * The minimum amount of local contrast required to apply algorithm.
   *    1/3 - too little
   *    1/4 - low quality
   *    1/8 - high quality
   *    1/16 - overkill
   */
  float edgeThreshold;
  FXAAConfig()
  : spanMax(8.0f),
    reduceMin(1.0/128.0),
    reduceMul(1.0/8.0),
    edgeThreshold(1.0/8.0),
    edgeThresholdMin(1.0/16.0)
  {
  }
};

class FXAA : public TextureShader {
public:
  FXAA(
      const vector<string> &args,
      const Texture &tex,
      const FXAAConfig &cfg);
  virtual string code() const;
};


#endif /* ANTI_ALIASING_H_ */
