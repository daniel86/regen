/*
 * font-manager.h
 *
 *  Created on: 15.03.2011
 *      Author: daniel
 */

#ifndef FONT_MANAGER_H_
#define FONT_MANAGER_H_

#include <GL/glew.h>

extern "C" {
#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>
}

#include <stdexcept>
#include <vector>
#include <string>
#include <map>
using namespace std;

#include <regen/gl-types/texture.h>
#include <regen/math/vector.h>
#include <regen/utility/ref-ptr.h>

namespace regen {
  /**
   * \brief Freetype2 Font class, using texture mapped glyphs.
   */
  class Font
  {
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
     * Get a font.
     * @param filename path to font
     * @param size font size, as usual
     * @param dpi dots per inch for font
     */
    static Font& get(string filename, GLuint size, GLuint dpi=96);

    /**
     * Default constructor.
     * @param filename path to font
     * @param size font size, as usual
     * @param dpi dots per inch for font
     */
    Font(const string &filename, GLuint size, GLuint dpi=96);
    virtual ~Font();

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

  private:
    typedef map<string, ref_ptr<Font> > FontMap;
    static FT_Library ftlib_;
    static FontMap fonts_;

    const string fontPath_;
    const GLuint size_;
    const GLuint dpi_;
    ref_ptr<Texture2DArray> arrayTexture_;
    FaceData *faceData_;
    GLfloat lineHeight_;

    Font(const Font&) : fontPath_(""), size_(0), dpi_(0) {}
    Font& operator=(const Font&) { return *this; }

    GLubyte* invertPixmapWithAlpha(const FT_Bitmap& bitmap, GLuint width, GLuint height) const;

    void initGlyph(FT_Face face, GLushort ch, GLuint textureWidth, GLuint textureHeight);
  };
} // namespace

#endif /* FONT_MANAGER_H_ */
