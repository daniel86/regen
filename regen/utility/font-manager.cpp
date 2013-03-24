/*
 * font-manager.cpp
 *
 *  Created on: 15.03.2011
 *      Author: daniel
 */

using namespace std;

#include <sstream>
#include <stdexcept>

#include "font-manager.h"
using namespace regen;

FontManager& FontManager::get()
{
  static FontManager tm;
  return tm;
}

FontManager::FontManager()
{
  if(FT_Init_FreeType( &ftlib_ ))
  { throw FreeTypeFont::Error("FT_Init_FreeType failed."); }
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

FreeTypeFont& FontManager::getFont(string f, GLuint size, GLuint dpi)
{
  // unique font identifier
  stringstream fontKey;
  fontKey << f << size << "_" << dpi;
  // check for cached font
  map<string, FreeTypeFont*>::iterator result = fonts_.find(fontKey.str());
  if(result != fonts_.end()) {
    return *(result->second);
  }

  // create the font
  FreeTypeFont* font = new FreeTypeFont(ftlib_, f, size, dpi);
  fonts_[fontKey.str()] = font;

  return *font;
}
