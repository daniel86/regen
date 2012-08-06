/*
 * font-manager.cpp
 *
 *  Created on: 15.03.2011
 *      Author: daniel
 */

#include <sstream>
#include <stdexcept>

#include "font-manager.h"

FontManager& FontManager::get()
{
  static FontManager tm;
  return tm;
}

FontManager::FontManager()
{
  if(FT_Init_FreeType( &ftlib_ ))
  {
    throw FreeTypeError("FT_Init_FreeType failed.");
  }
}
FontManager::~FontManager()
{
  for(map<string, FreeTypeFont*>::iterator
      font = fonts_.begin(); font != fonts_.end(); ++font)
  {
    delete font->second;
  }
  fonts_.clear();

  FT_Done_FreeType(ftlib_);
}

FreeTypeFont& FontManager::getFont(
    string f,
    GLuint size,
    const Vec4f &color,
    const Vec4f &backgroundColor,
    GLuint glyphRotationDegrees,
    GLenum filterMode,
    bool useMipmap,
    GLuint dpi) throw (FreeTypeError, FontError, FileNotFoundException)
{
  // unique font identifier
  stringstream fontKey;
  fontKey << f << size << "_" <<
      color << "_" <<
      backgroundColor << "_" <<
      glyphRotationDegrees << "_" <<
      filterMode << "_" <<
      dpi;

  // check for cached font
  map<string, FreeTypeFont*>::iterator result = fonts_.find(fontKey.str());
  if(result != fonts_.end()) {
    return *(result->second);
  }

  // create the font
  FreeTypeFont* font = new FreeTypeFont(ftlib_, f, size,
              dpi, glyphRotationDegrees, filterMode, useMipmap, color, backgroundColor);
  fonts_[fontKey.str()] = font;

  return *font;
}
