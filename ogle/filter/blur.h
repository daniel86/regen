/*
 * blur.h
 *
 *  Created on: 23.02.2013
 *      Author: daniel
 */

#ifndef BLUR_H_
#define BLUR_H_

#include <ogle/filter/filter.h>

class BlurSeparableFilter : public Filter
{
public:
  enum Direction { HORITONTAL, VERTICAL };

  BlurSeparableFilter(Direction dir, GLfloat scale=1.0f);

  /**
   * The sigma value for the gaussian function: higher value means more blur.
   */
  const ref_ptr<ShaderInput1f>& sigma() const;
  /**
   * Half number of texels to consider..
   */
  const ref_ptr<ShaderInput1f>& numPixels() const;

protected:
  Direction dir_;
  ref_ptr<ShaderInput1f> sigma_;
  ref_ptr<ShaderInput1f> numPixels_;
};

#endif /* BLUR_H_ */
