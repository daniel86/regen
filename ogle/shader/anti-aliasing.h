/*
 * anti-aliasing.h
 *
 *  Created on: 29.07.2012
 *      Author: daniel
 */

#ifndef ANTI_ALIASING_H_
#define ANTI_ALIASING_H_

#include <ogle/shader/shader-function.h>

class FXAA : public ShaderFunctions {
public:
  struct Config {
    GLfloat spanMax;
    GLfloat reduceMin;
    GLfloat reduceMul;
    /**
     * Trims the algorithm from processing darks.
     *   1/32 - visible limit,
     *   1/16 - high quality,
     *   1/12 - upper limit (start of visible unfiltered edges)
     */
    GLfloat edgeThresholdMin;
    /**
     * The minimum amount of local contrast required to apply algorithm.
     *    1/3 - too little
     *    1/4 - low quality
     *    1/8 - high quality
     *    1/16 - overkill
     */
    GLfloat edgeThreshold;
    Config()
    : spanMax(8.0f),
      reduceMin(1.0/128.0),
      reduceMul(1.0/8.0),
      edgeThreshold(1.0/8.0),
      edgeThresholdMin(1.0/16.0)
    {
    }
  };

  FXAA(const Config &cfg);
  virtual string code() const;
};


#endif /* ANTI_ALIASING_H_ */
