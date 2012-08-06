/*
 * tesselation-shader.h
 *
 *  Created on: 11.04.2012
 *      Author: daniel
 */

#ifndef TESSELATION_SHADER_H_
#define TESSELATION_SHADER_H_

#include <ogle/shader/shader-function.h>

class TesselationControlNDC : public ShaderFunctions {
public:
  TesselationControlNDC(
      const vector<string> &args,
      bool useDisplacement);
  void set_lodMetric(TessLodMetric metric) {
    metric_ = metric;
  }
  TessLodMetric lodMetric() const {
    return metric_;
  }
protected:
  bool useDisplacement_;
  TessLodMetric metric_;
};

class QuadTesselationControl : public TesselationControlNDC
{
public:
  QuadTesselationControl(
      const vector<string> &args,
      bool useDisplacement);
  virtual string code() const;
};

class TriangleTesselationControl : public TesselationControlNDC
{
public:
  TriangleTesselationControl(
      const vector<string> &args,
      bool useDisplacement);
  virtual string code() const;
};

#endif /* TESSELATION_SHADER_H_ */
