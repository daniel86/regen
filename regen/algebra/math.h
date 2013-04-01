/*
 * math.h
 *
 *  Created on: 01.04.2013
 *      Author: daniel
 */

#ifndef MATH_H_
#define MATH_H_

// = 360.0/(2.0*pi)
#define RAD_TO_DEGREE 57.29577951308232
// = 2.0*pi/360.0
#define DEGREE_TO_RAD 0.0174532925199432

namespace regen {
namespace math {
  /**
   * Check if floating point values are equal.
   */
  static inline GLboolean isApprox(const GLfloat &a, const GLfloat &b, GLfloat delta=1e-6)
  { return abs(a-b)<=delta; }

  /**
   * linearly interpolate between two values.
   */
  static inline GLdouble mix(GLdouble x, GLdouble y, GLdouble a)
  { return x*(1.0-a) + y*a; }

  /**
   * constrain a value to lie between two further values.
   */
  static inline GLfloat clamp(GLfloat x, GLfloat min, GLfloat max)
  {
    if(x>max)      return max;
    else if(x<min) return min;
    else           return x;
  }
}} // namespace

#endif /* MATH_H_ */
