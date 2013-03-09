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

namespace ogle {
/**
 * \brief Freetype2 Font class, using texture mapped glyphs.
 */
class FreeTypeFont {
public:
  /**
   * \brief A font related error occurred.
   */
  class Error : public runtime_error {
  public:
    Error(const string& msg) : runtime_error(msg) {}
  };
  /**
   * Defines a glyph face.
   */
  typedef struct {
    GLfloat width;
    GLfloat height;
    GLfloat uvX;
    GLfloat uvY;
    GLfloat left;
    GLfloat top;
    GLfloat advanceX;
  }FaceData;

  /**
   * Default constructor.
   */
  FreeTypeFont(FT_Library &library, const string &fontPath, GLuint size, GLuint dpi=96);
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

  void initGlyph(FT_Face face, GLushort ch, GLuint textureWidth, GLuint textureHeight);
};

} // end ogle namespace

#endif /* FREE_TYPE_H_ */
