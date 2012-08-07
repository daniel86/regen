/*
 * font-manager.h
 *
 *  Created on: 15.03.2011
 *      Author: daniel
 */

#ifndef FONT_MANAGER_H_
#define FONT_MANAGER_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <map>
#include <string>

#include <ogle/font/free-type.h>

/**
 * Singleton font manager.
 * Fonts are cached, so you will get the
 * same pointer if you call getFont() twice with the same arguments.
 */
class FontManager
{
public:
  /**
   * Get the font manager instance.
   */
  static FontManager& get();

  /**
   * Get a font.
   * @filename path to font
   * @size font size, as usual
   * @dpi dots per inch for font
   * @glyphRotationDegrees rotate each glyph by this angle
   * @filterMode filter mode for glyph textures
   * @color text color
   */
  FreeTypeFont& getFont(
      string filename,
      GLuint size,
      const Vec4f &color=(Vec4f){1,1,1,1},
      const Vec4f &backgroundColor=(Vec4f){0,0,0,1},
      GLuint glyphRotationDegrees=0,
      GLenum filterMode=GL_LINEAR,
      bool useMipmap=true,
      GLuint dpi=96) throw (FreeTypeError, FontError, FileNotFoundException);

private:
  // Hide these 'cause this is a singleton.
  FontManager();
  ~FontManager();
  FontManager(const FontManager&){};
  FontManager& operator = (const FontManager&){ return *this; };

  FT_Library ftlib_;
  // container for fonts
  std::map<string, FreeTypeFont*> fonts_;
};

#endif /* FONT_MANAGER_H_ */
