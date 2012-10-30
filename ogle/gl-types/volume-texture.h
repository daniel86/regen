/*
 * volume-texture.h
 *
 *  Created on: 10.03.2012
 *      Author: daniel
 */

#ifndef VOLUME_TEXTURE_H_
#define VOLUME_TEXTURE_H_

#include <ogle/gl-types/texture.h>
#include <ogle/algebra/vector.h>
#include <ogle/exceptions/io-exceptions.h>

/**
 * A 3 dimensional texture.
 */
class Texture3D : public Texture {
public:
  Texture3D(GLuint numTextures=1,
      GLenum targetType=GL_TEXTURE_3D)
  : Texture(numTextures)
  {
    dim_ = 3;
    targetType_ = targetType;
  }
  void set_numTextures(GLuint numTextures) {
    numTextures_ = numTextures;
  }
  GLuint numTextures() {
    return numTextures_;
  }
  virtual void texImage() const {
    glTexImage3D(targetType_,
        0, // mipmap level
        internalFormat_,
        width_,
        height_,
        numTextures_,
        border_,
        format_,
        pixelType_,
        data_);
  }
  virtual void texSubImage(int layer, GLubyte *subData) const {
    glTexSubImage3D(
            targetType_,
            0, 0, 0, layer,
            width_,
            height_,
            1,
            format_,
            pixelType_,
            subData);
  }
  string samplerType() const {
    return "sampler3D";
  }

protected:
  GLuint numTextures_;

private:
  Texture3D(const Texture3D&);
};

class DepthTexture3D : public Texture3D {
public:
  DepthTexture3D(
      GLuint numTextures=1,
      GLenum internalFormat=GL_DEPTH_COMPONENT,
      GLenum pixelType=GL_UNSIGNED_BYTE,
      GLenum targetType=GL_TEXTURE_3D) : Texture3D(numTextures, targetType)
  {
    format_  = GL_DEPTH_COMPONENT;
    internalFormat_ = internalFormat;
    pixelType_ = pixelType;
  }
private:
    DepthTexture3D(const DepthTexture3D&);
};

/**
 * Array texture of two dimensional textures.
 */
class Texture2DArray : public Texture3D {
public:
  Texture2DArray(
      GLuint numTextures=1,
      GLenum format=GL_RGBA,
      GLenum internalFormat=GL_RGBA,
      GLenum pixelType=GL_UNSIGNED_INT,
      GLint border=0,
      GLuint width=0, GLuint height=0)
  : Texture3D(numTextures, GL_TEXTURE_2D_ARRAY)
  {
    format_ = format;
    internalFormat_ = internalFormat;
    pixelType_ = pixelType;
    border_ = border;
    width_ = width;
    height_ = height;
    numTextures_ = 0;
  }
  virtual string samplerType() const {
    return "sampler2DArray";
  }
private:
    Texture2DArray(const Texture2DArray&);
};

////////

typedef struct {
  string path;
  GLuint width, height, depth;
  GLuint numComponents;
  GLuint bytesPerComponent;
}RAWTextureFile;

class DataSetError : public runtime_error {
public: DataSetError(const string &message) : runtime_error(message) {}
};

class RAWTexture3D : public Texture3D {
public:
  RAWTexture3D();

  void loadRAWFile(const RAWTextureFile &raw) throw (FileNotFoundException);
};

class PyroclasticVolume : public Texture3D {
public:
  // create a volume texture with n^3 texels and base radius r
  PyroclasticVolume(int n, float r);
};

#endif /* VOLUME_TEXTURE_H_ */
