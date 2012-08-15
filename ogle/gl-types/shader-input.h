/*
 * shader-input.h
 *
 *  Created on: 15.08.2012
 *      Author: daniel
 */

#ifndef SHADER_INPUT_H_
#define SHADER_INPUT_H_

#include <string>
using namespace std;

#include <ogle/gl-types/vertex-attribute.h>

enum FragmentInterpolation {
  // means that there is no interpolation.
  // The value given to the fragment shader is based on the provoking vertex conventions
  FRAGMENT_INTERPOLATION_FLAT,
  // means that there will be linear interpolation in window-space.
  FRAGMENT_INTERPOLATION_NOPERSPECTIVE,
  // the default, means to do perspective-correct interpolation.
  FRAGMENT_INTERPOLATION_SMOOTH,
  // only matters when multisampling. If this qualifier is not present,
  // then the value is interpolated to the pixel's center, anywhere in the pixel,
  // or to one of the pixel's samples. This sample may lie outside of the actual
  // primitive being rendered, since a primitive can cover only part of a pixel's area.
  // The centroid qualifier is used to prevent this;
  // the interpolation point must fall within both the pixel's area and the primitive's area.
  FRAGMENT_INTERPOLATION_CENTROID,
  FRAGMENT_INTERPOLATION_DEFAULT
};

class ShaderInput
{
public:
  ShaderInput(ref_ptr<VertexAttribute> &att)
  : att_(att),
    isConstant_(GL_FALSE),
    forceArray_(GL_FALSE),
    fragmentInterpolation_(FRAGMENT_INTERPOLATION_DEFAULT)
  {
  }
  GLboolean isVertexAttribute() const
  {
    return (att_->numInstances()>1 || att_->numVertices()>1);
  }

  void set_isConstant(GLboolean isConstant)
  {
    isConstant_ = isConstant;
  }
  GLboolean isConstant() const
  {
    return !isVertexAttribute() && isConstant_;
  }

  void set_interpolationMode(FragmentInterpolation fragmentInterpolation)
  {
    fragmentInterpolation_ = fragmentInterpolation;
  }
  FragmentInterpolation interpolationMode()
  {
    return fragmentInterpolation_;
  }

  void set_forceArray(GLboolean forceArray)
  {
    forceArray_ = forceArray;
  }
  GLboolean forceArray()
  {
    return forceArray_;
  }

  const string& name()
  {
    return att_->name();
  }
  GLenum dataType()
  {
    return att_->dataType();
  }
  GLuint valsPerElement()
  {
    return att_->valsPerElement();
  }
  GLuint numArrayElements()
  {
    return att_->elementCount();
  }
  byte* dataPtr()
  {
    return att_->dataPtr();
  }
protected:
  ref_ptr<VertexAttribute> att_;
  GLboolean isConstant_;
  GLboolean forceArray_;
  FragmentInterpolation fragmentInterpolation_;
};

#endif /* SHADER_INPUT_H_ */
