/*
 * fire-transfer.h
 *
 *  Created on: 16.08.2012
 *      Author: daniel
 */

#ifndef FIRE_TRANSFER_H_
#define FIRE_TRANSFER_H_

/**
 * Visualize fire using some parameters and a 1D transfer texture.
 */
class FireTransfer : public TexelTransfer
{
public:
  FireTransfer(ref_ptr<Texture> pattern);
  virtual string transfer();
  virtual void addShaderInputs(ShaderFunctions *shader);

  ref_ptr<ShaderInput1i> rednessFactor_;
  ref_ptr<ShaderInput3f> smokeColor_;
  ref_ptr<ShaderInput1f> smokeColorMultiplier_;
  ref_ptr<ShaderInput1f> smokeAlphaMultiplier_;
  ref_ptr<ShaderInput1f> fireAlphaMultiplier_;
  ref_ptr<ShaderInput1f> fireWeight_;
  ref_ptr<ShaderInput1f> texelFactor_;
protected:
  ref_ptr<Texture> pattern_;
};

#endif /* FIRE_TRANSFER_H_ */
