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

/**
 * FBO and texture used for fluid simulation.
 * For most quantities float pixel formats should be used.
 */
class FluidBuffer : public FrameBufferObject
{
public:
  enum PixelType {
    BYTE, F16, F32
  };

  /**
   * Create a texture for using in fluid simulation.
   */
  static ref_ptr<Texture> createTexture(
      Vec3i size,
      GLint numComponents,
      GLint numTexs,
      PixelType pixelType);

  /**
   * Constructor that generates a texture based on
   * given parameters.
   */
  FluidBuffer(
      const string &name,
      Vec3i size,
      GLuint numComponents,
      GLuint numTexs,
      PixelType pixelType);
  /**
   * Constructor that takes a previously allocated texture.
   */
  FluidBuffer(
      const string &name,
      ref_ptr<Texture> &fluidTexture);

  const string& name();

  /**
   * Texture attached to this buffer.
   */
  Texture* fluidTexture();

  /**
   * Clears all attached textures to zero.
   */
  void clear();
  /**
   * Swap the active texture if there are multiple
   * attached textures.
   */
  void swap();

protected:
  ref_ptr<Texture> fluidTexture_;
  string name_;
};

#endif /* FLUID_BUFFER_H_ */
