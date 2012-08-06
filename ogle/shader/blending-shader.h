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
class TextureBlenderCol2 : public ShaderFunctions {
public:
  TextureBlenderCol2(
          const string &name,
          const vector<string> &args);
};

/**
 * Mixes together two colors.
 */
class MixBlender : public TextureBlenderCol2 {
public:
  MixBlender(const vector<string> &args);
  virtual string code() const;
};

class AlphaBlender : public TextureBlenderCol2 {
public:
  AlphaBlender(const vector<string> &args);
  virtual string code() const;
};

class FrontToBackBlender : public TextureBlenderCol2 {
public:
  FrontToBackBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using add operation.
 */
class AddBlender : public TextureBlenderCol2 {
public:
  AddBlender(const vector<string> &args, bool smoothAdd, bool signedAdd);
  virtual string code() const;
  bool smoothAdd_;
  bool signedAdd_;
};
/**
 * Mixes together two colors avoiding overblending to white.
 */
class AddNormalizedBlender : public TextureBlenderCol2 {
public:
  AddNormalizedBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using substract operation.
 */
class SubBlender : public TextureBlenderCol2 {
public:
  SubBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using multiply operation.
 */
class MulBlender : public TextureBlenderCol2 {
public:
  MulBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using divide operation.
 */
class DivBlender : public TextureBlenderCol2 {
public:
  DivBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using diff operation.
 */
class DiffBlender : public TextureBlenderCol2 {
public:
  DiffBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using darker color.
 */
class DarkBlender : public TextureBlenderCol2 {
public:
  DarkBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using lighter color.
 */
class LightBlender : public TextureBlenderCol2 {
public:
  LightBlender(const vector<string> &args);
  virtual string code() const;
};

class ScreenBlender : public TextureBlenderCol2 {
public:
  ScreenBlender(const vector<string> &args);
  virtual string code() const;
};

class OverlayBlender : public TextureBlenderCol2 {
public:
  OverlayBlender(const vector<string> &args);
  virtual string code() const;
};

class DodgeBlender : public TextureBlenderCol2 {
public:
  DodgeBlender(const vector<string> &args);
  virtual string code() const;
};

class BurnBlender : public TextureBlenderCol2 {
public:
  BurnBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using rgb to hsv convert.
 */
class HueBlender : public TextureBlenderCol2 {
public:
  HueBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using rgb to hsv convert.
 */
class SatBlender : public TextureBlenderCol2 {
public:
  SatBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using rgb to hsv convert.
 */
class ValBlender : public TextureBlenderCol2 {
public:
  ValBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using rgb to hsv convert.
 */
class ColBlender : public TextureBlenderCol2 {
public:
  ColBlender(const vector<string> &args);
  virtual string code() const;
};

/**
 * Mixes together two colors using soft blending.
 */
class SoftBlender : public TextureBlenderCol2 {
public:
  SoftBlender(const vector<string> &args) :
          TextureBlenderCol2("softBlender", args) {};
  virtual string code() const;
};

/**
 * Mixes together two colors using linear blending.
 */
class LinearBlender : public TextureBlenderCol2 {
public:
  LinearBlender(const vector<string> &args) :
          TextureBlenderCol2("linearBlender", args) {};
  virtual string code() const;
};

TextureBlenderCol2* newBlender(TextureBlendMode blendMode,
		const vector<string> &args);

#endif /* _BLENDING_H_ */
