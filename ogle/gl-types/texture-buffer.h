
#ifndef TEXTURE_BUFFER_H_
#define TEXTURE_BUFFER_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <string>
using namespace std;

#include <ogle/algebra/vector.h>
#include <ogle/gl-types/fbo.h>
#include <ogle/gl-types/texture.h>
#include <ogle/gl-types/shader-input.h>

class TextureBuffer : public FrameBufferObject
{
public:
  enum PixelType {
    BYTE, F16, F32
  };

  /**
   * Create a texture.
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
  TextureBuffer(
      const string &name,
      Vec3i size,
      GLuint numComponents,
      GLuint numTexs,
      PixelType pixelType);
  /**
   * Constructor that takes a previously allocated texture.
   */
  TextureBuffer(
      const string &name,
      ref_ptr<Texture> &texture);

  const string& name();

  const ref_ptr<ShaderInputf>& inverseSize();

  /**
   * Texture attached to this buffer.
   */
  ref_ptr<Texture>& texture();

  /**
   * Clears all attached textures to zero.
   */
  void clear(const Vec4f &clearColor, GLint numBuffers);
  /**
   * Swap the active texture if there are multiple
   * attached textures.
   */
  void swap();

protected:
  ref_ptr<Texture> texture_;
  ref_ptr<ShaderInputf> inverseSize_;
  Vec3i size_;
  string name_;

  void initUniforms();
};

#endif /* TEXTURE_BUFFER_H_ */
