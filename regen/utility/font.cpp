/*
 * font-manager.cpp
 *
 *  Created on: 15.03.2011
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include <regen/utility/string-util.h>
#include <regen/gl-types/gl-util.h>
#include <regen/gl-types/render-state.h>

#include "font.h"
using namespace regen;

// number of glyphs that will be loaded for each face
#define NUMBER_OF_GLYPHS 256

FT_Library Font::ftlib_ = FT_Library();
Font::FontMap Font::fonts_ = FontMap();

Font& Font::get(string f, GLuint size, GLuint dpi)
{
  if(fonts_.empty()) {
    if(FT_Init_FreeType( &ftlib_ ))
    { throw Font::Error("FT_Init_FreeType failed."); }
  }
  // unique font identifier
  stringstream fontKey;
  fontKey << f << size << "_" << dpi;
  // check for cached font
  FontMap::iterator result = fonts_.find(fontKey.str());
  if(result != fonts_.end()) {
    return *result->second.get();
  }

  // create the font
  ref_ptr<Font> font = ref_ptr<Font>::alloc(f, size, dpi);
  fonts_[fontKey.str()] = font;

  return *font.get();
}

Font::Font(const string &fontPath, GLuint size, GLuint dpi)
: fontPath_(fontPath),
  size_(size),
  dpi_(dpi),
  lineHeight_(1.0f/.83f)
{
  // The object in which Freetype holds information on a given
  // font is called a "face".
  FT_Face face;

  if (!boost::filesystem::exists(fontPath)) {
    throw Error(REGEN_STRING(
        "Unable to font file at '" << fontPath << "'."));
  }

  // This is where we load in the font information from the file.
  // Of all the places where the code might die, this is the most likely,
  // as FT_New_Face will die if the font file does not exist or is somehow broken.
  if (FT_New_Face(ftlib_, fontPath.c_str(), 0, &face )) {
    throw Error(REGEN_STRING(
        "FT_New_Face failed. " <<
        "There is probably a problem with the font file at " << fontPath << "."));
  }
  // Freetype measures font size in terms of 1/64ths of pixels.
  // Thus, to make a font h pixels high, we need to request a size of h*64.
  // (h << 6 is just a prettier way of writing h*64)
  FT_Set_Char_Size(face, size << 6, size << 6, dpi, dpi);

  // find the bounding box that can hold any glyph of this font,
  // we need to find this box because all glyph textures must have the same size
  // for Texture2DArray.
  int pixels_x = ::FT_MulFix((face->bbox.xMax - face->bbox.xMin), face->size->metrics.x_scale );
  int pixels_y = ::FT_MulFix((face->bbox.yMax - face->bbox.yMin), face->size->metrics.y_scale );
  GLuint textureWidth = (GLuint) ( pixels_x / 64 );
  GLuint textureHeight = (GLuint)  ( pixels_y / 64 );
  // i do not know why but this is needed,
  // else the glyph is corrupted
  if(textureWidth%2 != 0) textureWidth += 1;
  if(textureHeight%2 != 0) textureHeight += 1;

  // remembers some geometry infos about glyphs
  faceData_ = new FaceData[NUMBER_OF_GLYPHS];

  // create a array texture for the glyphs
  arrayTexture_ = ref_ptr<Texture2DArray>::alloc(1);
  arrayTexture_->begin(RenderState::get());
  arrayTexture_->set_format(GL_RED);
  arrayTexture_->set_internalFormat(GL_R8);
  arrayTexture_->set_pixelType(GL_UNSIGNED_BYTE);
  arrayTexture_->set_rectangleSize(textureWidth, textureHeight);
  arrayTexture_->set_depth(NUMBER_OF_GLYPHS);
  arrayTexture_->set_wrapping(GL_CLAMP_TO_BORDER);
  arrayTexture_->set_filter(GL_LINEAR,GL_LINEAR);
  arrayTexture_->set_swizzleG(GL_RED);
  arrayTexture_->set_swizzleB(GL_RED);
  arrayTexture_->set_swizzleA(GL_RED);
  // GL expects 4byte aligned rows
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  arrayTexture_->texImage();
  for(unsigned short i=0;i<NUMBER_OF_GLYPHS;i++)
  {
    initGlyph(face, i, textureWidth, textureHeight);
  }
  // reset to default
  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  arrayTexture_->end(RenderState::get());

  FT_Done_Face(face);
}
Font::~Font()
{
  stringstream fontKey;
  fontKey << fontPath_ << size_ << "_" << dpi_;
  FontMap::iterator needle = fonts_.find(fontKey.str());
  if(needle != fonts_.end()) {
    fonts_.erase(needle);
  }
  if(fonts_.empty()) {
    FT_Done_FreeType(ftlib_);
  }
  delete [] faceData_;
}

GLfloat Font::lineHeight() const
{
  return lineHeight_;
}
GLuint Font::size() const
{
  return size_;
}
const ref_ptr<Texture2DArray>& Font::texture() const
{
  return arrayTexture_;
}
const Font::FaceData& Font::faceData(GLushort ch) const
{
  return faceData_[ch];
}

GLubyte* Font::invertPixmapWithAlpha (
    const FT_Bitmap& bitmap, GLuint width, GLuint height) const
{
  const unsigned int arraySize = width * height;
  GLubyte* inverse = new GLubyte[arraySize];
  GLubyte* inverse_ptr = inverse;
  int r,p;

  memset(inverse, 0x0, sizeof(GLubyte)*arraySize);

  for(r = 0; r < bitmap.rows; ++r)
  {
    GLubyte* bitmap_ptr = &bitmap.buffer[bitmap.pitch * r];
    for(p = 0; p < bitmap.width; ++p)
    {
      *inverse_ptr++ = *bitmap_ptr++;
    }
    inverse_ptr += ( width - bitmap.width );
  }

  return inverse;
}

void Font::initGlyph(FT_Face face, GLushort ch, GLuint textureWidth, GLuint textureHeight)
{
  FaceData &glyphData = faceData_[ch];
  FT_Glyph glyph;
  GLubyte* inverted;

  // Load the Glyph for our character.
  if(FT_Load_Glyph( face, FT_Get_Char_Index( face, ch ), FT_LOAD_DEFAULT )) {
    throw Error("FT_Load_Glyph failed");
  }
  // Move the face's glyph into a Glyph object.
  if(FT_Get_Glyph( face->glyph, &glyph )) {
    throw Error("FT_Get_Glyph failed");
  }
  // Convert the glyph to a bitmap.
  FT_Glyph_To_Bitmap( &glyph, ft_render_mode_normal, 0, 1 );

  FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)glyph;
  // This reference will make accessing the bitmap easier
  FT_Bitmap& bitmap = bitmap_glyph->bitmap;

  {
    GLfloat bitmapWith = (GLfloat) bitmap.width;
    GLfloat bitmapHeight = (GLfloat) bitmap.rows;
    GLfloat sizeFactor = 1.0f / (GLfloat) size();

    glyphData.uvX = bitmapWith/((GLfloat)textureWidth);
    glyphData.uvY = bitmapHeight/((GLfloat)textureHeight);
    glyphData.height = bitmapHeight*sizeFactor;
    glyphData.width = bitmapWith*sizeFactor;
    glyphData.advanceX = (face->glyph->advance.x >> 6)*sizeFactor;
    glyphData.left = bitmap_glyph->left*sizeFactor;
    glyphData.top = bitmap_glyph->top*sizeFactor;
  }

  {
    inverted = invertPixmapWithAlpha(bitmap, textureWidth, textureHeight);
    arrayTexture_->texSubImage((int) ch, inverted);
    delete []inverted;
  }

  FT_Done_Glyph(glyph);
}
