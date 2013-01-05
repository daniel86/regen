/*
 * free-type.h
 *
 *  Created on: 05.04.2011
 *      Author: daniel
 */

#ifndef FREE_TYPE_H_
#define FREE_TYPE_H_

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>

#include <stdexcept>
#include <vector>
#include <string>
using namespace std;

#include <ogle/gl-types/texture.h>
#include <ogle/algebra/vector.h>
#include <ogle/utility/ref-ptr.h>
#include <ogle/exceptions/io-exceptions.h>

typedef struct {
  float width, height, uvX, uvY, left, top, advanceX;
}FaceData;

/**
 * Something wrong processing the font.
 */
class FreeTypeError : public runtime_error {
public:
  FreeTypeError(const string& msg)
  : runtime_error(msg)
  {
  }
};
/**
 * Might be a problem with the font file.
 */
class FontError : public runtime_error {
public:
  FontError(const string& msg)
  : runtime_error(msg)
  {
  }
};

/**
 * Freetype2 Font class, using texture mapped glyphs.
 */
class FreeTypeFont {
public:
  /**
   * Default constructor.
   */
  FreeTypeFont(FT_Library &library, const string &fontPath, GLuint size, GLuint dpi=96)
  throw (FreeTypeError, FontError, FileNotFoundException);
  ~FreeTypeFont();

  /**
   * Height of a line of text. In unit space (maps font size to 1.0).
   */
  GLfloat lineHeight() const;
  /**
   * The font size. In pixels.
   */
  GLuint size() const;
  /**
   * The texture used by this font.
   * You can access the glyphs with the char as index.
   */
  const ref_ptr<Texture2DArray>& texture() const;
  /**
   * Character to face data.
   */
  const FaceData& faceData(GLushort ch) const;

protected:
  GLuint size_;
  ref_ptr<Texture2DArray> arrayTexture_;
  FaceData *faceData_;
  GLfloat lineHeight_;

  GLubyte* invertPixmapWithAlpha(const FT_Bitmap& bitmap, GLuint width, GLuint height) const;

  void initGlyph(FT_Face face, GLushort ch, GLuint textureWidth, GLuint textureHeight)
  throw (FreeTypeError);
};

#endif /* FREE_TYPE_H_ */
