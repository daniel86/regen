/*
 * texture-transfer.h
 *
 *  Created on: 12.03.2012
 *      Author: daniel
 */

#ifndef TEXTURE_TRANSFER_H_
#define TEXTURE_TRANSFER_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <string>
using namespace std;

class Texture;
class ShaderFunctions;

#include <ogle/gl-types/uniform.h>
#include <ogle/states/state.h>

/**
 * Transfers the texel/voxel values to color and alpha.
 * Take a look at implementations if you consider writing a custom
 * transfer function.
 */
class TexelTransfer : public State
{
public:
  TexelTransfer();
  virtual string transfer() = 0;
  virtual void addShaderInputs(ShaderFunctions *shader) = 0;
  string name() const {
    return transferFuncName_;
  }
protected:
  string transferFuncName_;
};

/**
 * Take r value as alpha and color uniforms for the color.
 * Negative and positive r values can get different colors.
 */
class ScalarToAlphaTransfer : public TexelTransfer
{
public:
  ScalarToAlphaTransfer();
  virtual string transfer();
  virtual void addShaderInputs(ShaderFunctions *shader);

  ref_ptr<UniformVec3> fillColorPositive_;
  ref_ptr<UniformVec3> fillColorNegative_;
  ref_ptr<UniformFloat> texelFactor_;
};

/**
 */
class RGBColorfullTransfer : public TexelTransfer
{
public:
  RGBColorfullTransfer();
  virtual string transfer();
  virtual void addShaderInputs(ShaderFunctions *shader);

  ref_ptr<UniformFloat> texelFactor_;
};

/**
 * Visualize level set borders.
 */
class LevelSetTransfer : public TexelTransfer
{
public:
  LevelSetTransfer();
  virtual string transfer();
  virtual void addShaderInputs(ShaderFunctions *shader);

  ref_ptr<UniformFloat> texelFactor_;
};

/**
 * Visualize fire using some parameters and a 1D transfer texture.
 */
class FireTransfer : public TexelTransfer
{
public:
  FireTransfer(ref_ptr<Texture> pattern);
  virtual string transfer();
  virtual void addShaderInputs(ShaderFunctions *shader);

  ref_ptr<UniformInt> rednessFactor_;
  ref_ptr<UniformVec3> smokeColor_;
  ref_ptr<UniformFloat> smokeColorMultiplier_;
  ref_ptr<UniformFloat> smokeAlphaMultiplier_;
  ref_ptr<UniformFloat> fireAlphaMultiplier_;
  ref_ptr<UniformFloat> fireWeight_;
  ref_ptr<UniformFloat> texelFactor_;
protected:
  ref_ptr<Texture> pattern_;
};


#endif /* TEXTURE_TRANSFER_H_ */
