/*
 * fluid-buffer.h
 *
 *  Created on: 09.10.2012
 *      Author: daniel
 */

#ifndef FLUID_BUFFER_H_
#define FLUID_BUFFER_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <string>
using namespace std;

#include <ogle/algebra/vector.h>
#include <ogle/gl-types/fbo.h>
#include <ogle/gl-types/texture.h>

class FluidBuffer : public FrameBufferObject
{
public:
  enum PixelType {
    BYTE, F16, F32
  };

  static ref_ptr<Texture> createTexture(
      Vec3i size,
      GLint numComponents,
      GLint numTexs,
      PixelType pixelType);

  FluidBuffer(
      const string &name,
      Vec3i size,
      GLuint numComponents,
      GLuint numTexs,
      PixelType pixelType);
  FluidBuffer(
      const string &name,
      ref_ptr<Texture> &fluidTexture);

  Texture* fluidTexture();

  void clear();
  void swap();

  const string& name() { return name_; }

protected:
  ref_ptr<Texture> fluidTexture_;
  string name_;
};

#endif /* FLUID_BUFFER_H_ */
