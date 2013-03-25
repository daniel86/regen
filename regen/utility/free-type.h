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

#include <regen/gl-types/texture.h>
#include <regen/algebra/vector.h>
#include <regen/utility/ref-ptr.h>

namespace regen {
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
    /**
     * @param msg the error message.
     */
    Error(const string& msg) : runtime_error(msg) {}
  };
  /**
   * Defines a glyph face.
   */
  typedef struct {
    /** face width */
    GLfloat width;
    /** face height */
    GLfloat height;
    /** max uv.x in array texture (min is 0). */
    GLfloat uvX;
    /** max uv.y in array texture (min is 0). */
    GLfloat uvY;
    /** left margin */
    GLfloat left;
    /** top margin */
    GLfloat top;
    /** distance to net glyph */
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

} // namespace

#endif /* FREE_TYPE_H_ */
