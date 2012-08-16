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

#include <ogle/gl-types/shader-input.h>
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

  ref_ptr<ShaderInput3f> fillColorPositive_;
  ref_ptr<ShaderInput3f> fillColorNegative_;
  ref_ptr<ShaderInput1f> texelFactor_;
};

/**
 */
class RGBColorfullTransfer : public TexelTransfer
{
public:
  RGBColorfullTransfer();
  virtual string transfer();
  virtual void addShaderInputs(ShaderFunctions *shader);

  ref_ptr<ShaderInput1f> texelFactor_;
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

  ref_ptr<ShaderInput1f> texelFactor_;
};


#endif /* TEXTURE_TRANSFER_H_ */
