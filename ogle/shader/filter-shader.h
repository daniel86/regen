/*
 * texture-shader.h
 *
 *  Created on: 02.02.2011
 *      Author: daniel
 */

#ifndef _TEXTURE_SHADER_H_
#define _TEXTURE_SHADER_H_

#include <ogle/shader/shader-function.h>

class Scene;

/**
 * Baseclass for filter.
 */
class TextureShader : public ShaderFunctions {
public:
  TextureShader(
      const string &name,
      const vector<string> &args,
      const Texture &tex);
protected:
  string samplerType_;
};

//////////

class Downsample : public TextureShader {
public:
  Downsample(const vector<string> &args, const Texture &tex);
  virtual string code() const;
};

//////////

/**
 * The well known sepia filter.
 */
class Sepia : public TextureShader {
public:
  Sepia(const vector<string> &args, const Texture &tex);
  virtual string code() const;
};

////////////

/**
 * Outputs texture greyscaled.
 */
class GreyScaleFilter : public TextureShader {
public:
  GreyScaleFilter(const vector<string> &args, const Texture &tex);
  virtual string code() const;
};

/**
 * Make some tiles from the texture.
 */
class Tiles : public TextureShader {
public:
  Tiles(const vector<string> &args, const Texture &tex);
  virtual string code() const;
};

/////////

struct ConvolutionKernel
{
  GLuint size_;
  GLfloat *weights_;
  Vec2f *offsets_;
  ConvolutionKernel(const ConvolutionKernel &k)
  : size_(k.size_), weights_(k.weights_), offsets_(k.offsets_) {}
  ConvolutionKernel(
      GLuint size,
      GLfloat *weigths,
      Vec2f *offsets);
};

class ConvolutionShader : public TextureShader
{
public:
  ConvolutionShader(
      const string &name,
      const vector<string> &args,
      const ConvolutionKernel &k,
      const Texture &tex);
  virtual string code() const;
protected:
  string name_;
  string convolutionCode_;
  unsigned int kernelSize_;
};

ConvolutionKernel sharpenKernel(Texture &tex,
    GLuint pixelsPerSide=2, GLfloat stepFactor=1.0f);
ConvolutionKernel edgeDetectKernel(Texture &tex,
    GLuint pixelsPerSide=2, GLfloat stepFactor=1.0f);
ConvolutionKernel meanKernel(Texture &tex,
    GLuint pixelsPerSide=2, GLfloat stepFactor=1.0f);
ConvolutionKernel bloomKernel(Texture &tex,
    GLuint pixelsPerSide=2, GLfloat stepFactor=1.0f);

struct BlurConfig {
  /**
   * Blur will lookup pixelsPerSide*2+1 horizontal
   * and the same vertical.
   */
  GLuint pixelsPerSide;
  /**
   * The sigma value for the gaussian function: higher value means more blur
   *   A good value for 9x9 is around 3 to 5;
   *   A good value for 7x7 is around 2.5 to 4;
   *   A good value for 5x5 is around 2 to 3.5
   */
  GLfloat sigma;
  /**
   * stepFactor is a factor for the offsets
   * offsets are calculated like this: offset(i) = i*stepFactor*pixelSize
   */
  GLfloat stepFactor;
  BlurConfig()
  : pixelsPerSide(5),
    sigma(3.0f),
    stepFactor(1.0)
  {
  }
};

/**
 * The sigma value for the gaussian function: higher value means more blur
 *   A good value for 9x9 is around 3 to 5;
 *   A good value for 7x7 is around 2.5 to 4;
 *   A good value for 5x5 is around 2 to 3.5
 */
ConvolutionKernel blurHorizontalKernel(Texture &tex, const BlurConfig &cfg);
ConvolutionKernel blurVerticalKernel(Texture &tex, const BlurConfig &cfg);

class BloomShader : public ConvolutionShader {
public:
  BloomShader(const vector<string> &args,
      const ConvolutionKernel &k,
      const Texture &tex);
  virtual string code() const;
};

////////

class TonemapShader : public TextureShader {
public:
  TonemapShader(const vector<string> &args, const Texture &tex);
  virtual string code() const;
};

#endif /* _TEXTURE_SHADER_H_ */
