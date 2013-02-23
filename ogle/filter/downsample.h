/*
 * downsample.h
 *
 *  Created on: 23.02.2013
 *      Author: daniel
 */

#ifndef DOWNSAMPLE_H_
#define DOWNSAMPLE_H_

#include <ogle/filter/filter.h>

class DownsampleFilter : public Filter {
public:
  DownsampleFilter(GLfloat scale)
  : Filter("downsampleFilter", scale) {}
};

#endif /* DOWNSAMPLE_H_ */
