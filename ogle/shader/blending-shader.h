/*
 * blending.h
 *
 *  Created on: 02.02.2011
 *      Author: daniel
 */

#ifndef _BLENDING_H_
#define _BLENDING_H_

#include <ogle/shader/shader-function.h>
#include <ogle/gl-types/texture.h>

/**
 * Inverts incomming color.
 */
class InvertBlender : public ShaderFunctions {
public:
  InvertBlender(const vector<string> &args) :
          ShaderFunctions("invertShader", args) {};
  virtual string code() const;
};

/**
 * Changes brightness of incomming color.
 */
class BrightnessBlender : public ShaderFunctions {
public:
  BrightnessBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Changes contrast of incomming color.
 */
class ContrastBlender : public ShaderFunctions {
public:
  ContrastBlender(const vector<string> &args);
  virtual string code() const;
};

///////////////

/**
 * Mixes together two colors.
 */
class BlenderCol2 : public ShaderFunctions {
public:
  BlenderCol2(
          const string &name,
          const vector<string> &args);
};

/**
 * Mixes together two colors.
 */
class MixBlender : public BlenderCol2 {
public:
  MixBlender(const vector<string> &args);
  virtual string code() const;
};

class AlphaBlender : public BlenderCol2 {
public:
  AlphaBlender(const vector<string> &args);
  virtual string code() const;
};

class FrontToBackBlender : public BlenderCol2 {
public:
  FrontToBackBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using add operation.
 */
class AddBlender : public BlenderCol2 {
public:
  AddBlender(const vector<string> &args, bool smoothAdd, bool signedAdd);
  virtual string code() const;
  bool smoothAdd_;
  bool signedAdd_;
};
/**
 * Mixes together two colors avoiding overblending to white.
 */
class AddNormalizedBlender : public BlenderCol2 {
public:
  AddNormalizedBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using substract operation.
 */
class SubBlender : public BlenderCol2 {
public:
  SubBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using multiply operation.
 */
class MulBlender : public BlenderCol2 {
public:
  MulBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using divide operation.
 */
class DivBlender : public BlenderCol2 {
public:
  DivBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using diff operation.
 */
class DiffBlender : public BlenderCol2 {
public:
  DiffBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using darker color.
 */
class DarkBlender : public BlenderCol2 {
public:
  DarkBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using lighter color.
 */
class LightBlender : public BlenderCol2 {
public:
  LightBlender(const vector<string> &args);
  virtual string code() const;
};

class ScreenBlender : public BlenderCol2 {
public:
  ScreenBlender(const vector<string> &args);
  virtual string code() const;
};

class OverlayBlender : public BlenderCol2 {
public:
  OverlayBlender(const vector<string> &args);
  virtual string code() const;
};

class DodgeBlender : public BlenderCol2 {
public:
  DodgeBlender(const vector<string> &args);
  virtual string code() const;
};

class BurnBlender : public BlenderCol2 {
public:
  BurnBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using rgb to hsv convert.
 */
class HueBlender : public BlenderCol2 {
public:
  HueBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using rgb to hsv convert.
 */
class SatBlender : public BlenderCol2 {
public:
  SatBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using rgb to hsv convert.
 */
class ValBlender : public BlenderCol2 {
public:
  ValBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using rgb to hsv convert.
 */
class ColBlender : public BlenderCol2 {
public:
  ColBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using soft blending.
 */
class SoftBlender : public BlenderCol2 {
public:
  SoftBlender(const vector<string> &args) :
          BlenderCol2("softBlender", args) {};
  virtual string code() const;
};

/**
 * Mixes together two colors using linear blending.
 */
class LinearBlender : public BlenderCol2 {
public:
  LinearBlender(const vector<string> &args) :
          BlenderCol2("linearBlender", args) {};
  virtual string code() const;
};

BlenderCol2* newBlender(TextureBlendMode blendMode,
		const vector<string> &args);

#endif /* _BLENDING_H_ */
