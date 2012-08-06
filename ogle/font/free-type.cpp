/*
 * free-type.cpp
 *
 *  Created on: 05.04.2011
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>

#include "free-type.h"
#include <ogle/utility/string-util.h>

// number of glyphs that will be loaded for each face
#define NUMBER_OF_GLYPHS 256

FreeTypeFont::FreeTypeFont(
    FT_Library &library,
    const string &fontPath,
    unsigned int size,
    unsigned int dpi,
    unsigned int glyphRotationDegrees,
    GLenum filterMode,
    bool useMipmap,
    Vec4f color,
    Vec4f backgroundColor)
throw (FreeTypeError, FontError, FileNotFoundException)
: size_(size),
  lineHeight_(1.0f/.83f)
{
  // The object in which Freetype holds information on a given
  // font is called a "face".
  FT_Face face;

  if(access(fontPath.c_str(), F_OK) != 0) {
    throw FileNotFoundException(FORMAT_STRING(
        "Unable to font file at '" << fontPath << "'."));
  }

  // This is where we load in the font information from the file.
  // Of all the places where the code might die, this is the most likely,
  // as FT_New_Face will die if the font file does not exist or is somehow broken.
  if (FT_New_Face( library, fontPath.c_str(), 0, &face )) {
    throw FontError(FORMAT_STRING(
        "FT_New_Face failed. " <<
        "There is probably a problem with the font file at " << fontPath << "."));
  }
  // Freetype measures font size in terms of 1/64ths of pixels.
  // Thus, to make a font h pixels high, we need to request a size of h*64.
  // (h << 6 is just a prettier way of writing h*64)
  FT_Set_Char_Size(face, size << 6, size << 6, dpi, dpi);

  if(glyphRotationDegrees % 360 != 0) {
    FT_Matrix rotation_matrix;
    FT_Vector sinus;
    FT_Vector_Unit( &sinus, (FT_Angle)(glyphRotationDegrees * 0x10000L) );
    rotation_matrix.xx = sinus.x;
    rotation_matrix.xy = -sinus.y;
    rotation_matrix.yx = sinus.y;
    rotation_matrix.yy = sinus.x;
    FT_Set_Transform( face, &rotation_matrix, 0 );
  }

  // find the bounding box that can hold any glyph of this font,
  // we need to find this box because all glyph textures must have the same size
  // for Texture2DArray.
  int pixels_x = ::FT_MulFix((face->bbox.xMax - face->bbox.xMin), face->size->metrics.x_scale );
  int pixels_y = ::FT_MulFix((face->bbox.yMax - face->bbox.yMin), face->size->metrics.y_scale );
  unsigned int textureWidth = (unsigned int) ( pixels_x / 64 );
  unsigned int textureHeight = (unsigned int)  ( pixels_y / 64 );
  // i do not know why but this is needed,
  // else the glyph is corrupted
  if(textureWidth%2 != 0) textureWidth += 1;
  if(textureHeight%2 != 0) textureHeight += 1;

  // remembers some geometry infos about glyphs
  faceData_ = new FaceData[NUMBER_OF_GLYPHS];

  // create a array texture for the glyphs
  glPushAttrib( GL_PIXEL_MODE_BIT ); {
    glPixelTransferf(GL_RED_SCALE, color.x-1);
    glPixelTransferf(GL_GREEN_SCALE, color.y-1);
    glPixelTransferf(GL_BLUE_SCALE, color.z-1);
    glPixelTransferf(GL_ALPHA_SCALE, color.w);
    glPixelTransferf(GL_RED_BIAS, 1);
    glPixelTransferf(GL_GREEN_BIAS, 1);
    glPixelTransferf(GL_BLUE_BIAS, 1);
    glPixelTransferf( GL_ALPHA_BIAS, 0.0f);

    arrayTexture_ = ref_ptr< Texture2DArray >::manage(
        new Texture2DArray(1, GL_LUMINANCE_ALPHA, GL_RGBA,
            GL_UNSIGNED_BYTE, 0, textureWidth, textureHeight));
    arrayTexture_->addMapTo(MAP_TO_COLOR);
    arrayTexture_->set_numTextures(NUMBER_OF_GLYPHS+1);
    arrayTexture_->bind();
    arrayTexture_->set_wrapping( GL_CLAMP );
    arrayTexture_->texImage();

    for(unsigned short i=0;i<NUMBER_OF_GLYPHS;i++)
    {
      initGlyph(face, i, textureWidth, textureHeight);
    }

    // create a texture holding the background color
    glPixelTransferf(GL_RED_SCALE, backgroundColor.x-1);
    glPixelTransferf(GL_GREEN_SCALE, backgroundColor.y-1);
    glPixelTransferf(GL_BLUE_SCALE, backgroundColor.z-1);
    glPixelTransferf(GL_ALPHA_SCALE, backgroundColor.w);
    GLuint texSize = 2*textureWidth*textureHeight;
    GLubyte* background = new GLubyte[texSize];
    memset(background, 0xff, sizeof(GLubyte)*(texSize));
    arrayTexture_->texSubImage(NUMBER_OF_GLYPHS, background);
  } glPopAttrib();

  if(useMipmap) {
    if(filterMode == GL_LINEAR) {
      arrayTexture_->set_filter( GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR );
    } else {
      arrayTexture_->set_filter( GL_NEAREST, GL_NEAREST_MIPMAP_LINEAR );
    }
    arrayTexture_->setupMipmaps( GL_DONT_CARE );
  } else {
    arrayTexture_->set_filter( filterMode, filterMode );
  }

  FT_Done_Face(face);
}
FreeTypeFont::~FreeTypeFont()
{
  delete [] faceData_;
}

GLuint FreeTypeFont::backgroundGlyph() const {
  return NUMBER_OF_GLYPHS;
}

float FreeTypeFont::lineHeight() const
{
  return lineHeight_;
}
GLuint FreeTypeFont::size() const
{
  return size_;
}
ref_ptr<Texture2DArray>& FreeTypeFont::texture()
{
  return arrayTexture_;
}
const FaceData& FreeTypeFont::faceData(unsigned short ch) const
{
  return faceData_[ch];
}

GLubyte* FreeTypeFont::invertPixmapWithAlpha (
    const FT_Bitmap& bitmap,
    unsigned int width,
    unsigned int height) const
{
  const unsigned int arraySize = 2 * width * height;
  GLubyte* inverse = new GLubyte[arraySize];
  GLubyte* inverse_ptr = inverse;
  int r,p;

  memset(inverse, 0, sizeof(GLubyte)*(arraySize));

  for(r = 0; r < bitmap.rows; ++r)
  {
    GLubyte* bitmap_ptr = &bitmap.buffer[bitmap.pitch * r];
    for(p = 0; p < bitmap.width; ++p)
    {
      *inverse_ptr++ = 0xff;
      *inverse_ptr++ = *bitmap_ptr++;
    }
    inverse_ptr += 2 * ( width - bitmap.pitch );
  }

  return inverse;
}

void FreeTypeFont::initGlyph(
    FT_Face face,
    unsigned short ch,
    unsigned int textureWidth,
    unsigned int textureHeight) throw (FreeTypeError)
{
  FaceData &glyphData = faceData_[ch];
  FT_Glyph glyph;
  FT_Render_Mode renderFlag;
  GLubyte* inverted;

  // Load the Glyph for our character.
  if(FT_Load_Glyph( face, FT_Get_Char_Index( face, ch ), FT_LOAD_DEFAULT )) {
    throw FreeTypeError("FT_Load_Glyph failed");
  }
  // Move the face's glyph into a Glyph object.
  if(FT_Get_Glyph( face->glyph, &glyph )) {
    throw FreeTypeError("FT_Get_Glyph failed");
  }
  // Convert the glyph to a bitmap.
  FT_Glyph_To_Bitmap( &glyph, ft_render_mode_normal, 0, 1 );

  FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)glyph;
  // This reference will make accessing the bitmap easier
  FT_Bitmap& bitmap = bitmap_glyph->bitmap;

  {
    float bitmapWith = (float) bitmap.width;
    float bitmapHeight = (float) bitmap.rows;
    float sizeFactor = 1.0f / (float) size();

    glyphData.uvX = bitmapWith/((float)textureWidth);
    glyphData.uvY = bitmapHeight/((float)textureHeight);
    glyphData.height = bitmapHeight*sizeFactor;
    glyphData.width = bitmapWith*sizeFactor;
    glyphData.advanceX = (face->glyph->advance.x >> 6)*sizeFactor;
    glyphData.left = bitmap_glyph->left*sizeFactor;
    glyphData.top = bitmap_glyph->top*sizeFactor;
  }

  {
    inverted = invertPixmapWithAlpha(bitmap, textureWidth, textureHeight);
    arrayTexture_->texSubImage((int) ch, inverted);
  }

  FT_Done_Glyph(glyph);
}
