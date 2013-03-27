/*
 * texture-mapped-text.cpp
 *
 *  Created on: 23.03.2011
 *      Author: daniel
 */

#include <GL/glew.h>
#include <GL/gl.h>

#include <boost/algorithm/string.hpp>
#include <regen/states/texture-state.h>
#include <regen/gl-types/vbo-manager.h>

#include "texture-mapped-text.h"
using namespace regen;

TextureMappedText::TextureMappedText(FreeTypeFont &font, GLfloat height)
: Mesh(GL_QUADS),
  HasShader("gui.text"),
  font_(font),
  value_(),
  height_(height),
  numCharacters_(0)
{
  textColor_ = ref_ptr<ShaderInput4f>::manage(new ShaderInput4f("textColor"));
  textColor_->setUniformData(Vec4f(1.0));
  joinShaderInput(ref_ptr<ShaderInput>::cast(textColor_));

  ref_ptr<Texture> tex = ref_ptr<Texture>::cast(font.texture());
  ref_ptr<TextureState> texState = ref_ptr<TextureState>::manage(new TextureState(tex, "fontTexture"));
  texState->set_mapTo(TextureState::MAP_TO_COLOR);
  texState->set_blendMode(BLEND_MODE_MULTIPLY);
  joinStates(ref_ptr<State>::cast(texState));

  joinStates(ref_ptr<State>::cast(shaderState()));

  posAttribute_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f(ATTRIBUTE_NAME_POS));
  norAttribute_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f(ATTRIBUTE_NAME_NOR));
  texcoAttribute_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("texco0"));
}

void TextureMappedText::set_color(const Vec4f &color)
{
  textColor_->setVertex4f(0, color);
}

void TextureMappedText::set_height(GLfloat height)
{
  height_ = height;
}

const list<wstring>& TextureMappedText::value() const
{
  return value_;
}
void TextureMappedText::set_value(
    const list<wstring> &value,
    Alignment alignment,
    GLfloat maxLineWidth)
{
  value_ = value;
  numCharacters_ = 0;
  for(list<wstring>::const_iterator
      it = value.begin(); it != value.end(); ++it)
  {
    numCharacters_ += it->size();
  }
  updateAttributes(alignment, maxLineWidth);
}
void TextureMappedText::set_value(
    const wstring &value,
    Alignment alignment,
    GLfloat maxLineWidth)
{
  list<wstring> v;
  boost::split(v, value, boost::is_any_of("\n"));
  set_value(v, alignment, maxLineWidth);
}

void TextureMappedText::updateAttributes(Alignment alignment, GLfloat maxLineWidth)
{
  Vec3f translation, glyphTranslation;
  GLuint vertexCounter = 0u;

  GLfloat actualMaxLineWidth = 0.0;

  posAttribute_->setVertexData(numCharacters_*4);
  texcoAttribute_->setVertexData(numCharacters_*4);
  norAttribute_->setVertexData(numCharacters_*4);

  translation = Vec3f(0.0,0.0,0.0);
  glyphTranslation = Vec3f(0.0,0.0,0.0);

  for(list<wstring>::iterator
      it = value_.begin(); it != value_.end(); ++it)
  {
    translation.y -= font_.lineHeight()*height_;

    GLfloat buf;
    // actual width for this line
    GLfloat lineWidth = 0.0;
    // remember space for splitting string at words
    GLint lastSpaceIndex=0;
    GLfloat lastSpaceWidth = 0.0;

    // get line width and split the line
    // where it exceeds the width limit
    for(GLuint i=0; i<it->size(); ++i)
    {
      const wchar_t &ch = (*it)[i];
      buf = lineWidth + font_.faceData(ch).advanceX*height_;
      if(maxLineWidth>0.0 && buf > maxLineWidth && lastSpaceIndex!=0) {
        // maximal line length reached
        // split string at remembered space

        // insert the rest after current iterator
        // it cannot be placed in one line with
        // the words before lastSpaceIndex
        list<wstring>::iterator nextIt = it;
        nextIt++;
        value_.insert(nextIt, it->substr(lastSpaceIndex+1,
                        it->size()-lastSpaceIndex-1));
        // set the string width validated width
        *it = it->substr(0, lastSpaceIndex);

        lineWidth = lastSpaceWidth;
        break;
      }
      else if(ch == ' ' || ch == '\t')
      {
        // remember spaces
        lastSpaceIndex = i;
        lastSpaceWidth = lineWidth;
      }
      lineWidth = buf;
    }
    if(lineWidth > actualMaxLineWidth) {
      actualMaxLineWidth = lineWidth;
    }

    // calculate line advance!
    switch(alignment) {
    case ALIGNMENT_CENTER:
      translation.x = 0.5f*(maxLineWidth - lineWidth);
      break;
    case ALIGNMENT_RIGHT:
      translation.x = maxLineWidth - lineWidth;
      break;
    case ALIGNMENT_LEFT:
      translation.x = 0.0f;
      break;
    }

    // create the geometry with appropriate
    // translation and size for each glyph
    for(GLuint i=0; i<it->size(); ++i)
    {
      const wchar_t &ch = (*it)[i];
      const FreeTypeFont::FaceData &data = font_.faceData(ch);

      glyphTranslation = Vec3f(
          data.left*height_, (data.top-data.height)*height_, 0.001*(i+1)
      );
      makeGlyphGeometry(data, translation+glyphTranslation, (float) ch,
              posAttribute_.get(), norAttribute_.get(),
              texcoAttribute_.get(), &vertexCounter);

      // move cursor to next glyph
      translation.x += data.advanceX*height_;
    }

    translation.x = 0.0;
  }

  VBOManager::remove(*posAttribute_.get());
  VBOManager::remove(*norAttribute_.get());
  VBOManager::remove(*texcoAttribute_.get());
  setInput(ref_ptr<ShaderInput>::cast(posAttribute_));
  setInput(ref_ptr<ShaderInput>::cast(norAttribute_));
  setInput(ref_ptr<ShaderInput>::cast(texcoAttribute_));
}

void TextureMappedText::makeGlyphGeometry(
    const FreeTypeFont::FaceData &data,
    const Vec3f &translation,
    GLfloat layer,
    VertexAttribute *posAttribute,
    VertexAttribute *norAttribute,
    VertexAttribute *texcoAttribute,
    GLuint *vertexCounter)
{
  posAttribute->setVertex3f(*vertexCounter,
              translation + Vec3f(0.0,data.height*height_,0.0) );
  posAttribute->setVertex3f(*vertexCounter+1,
              translation + Vec3f(0.0,0.0,0.0) );
  posAttribute->setVertex3f(*vertexCounter+2,
              translation + Vec3f(data.width*height_,0.0,0.0) );
  posAttribute->setVertex3f(*vertexCounter+3,
              translation + Vec3f(data.width*height_,data.height*height_,0.0) );

  norAttribute->setVertex3f(*vertexCounter, Vec3f(0.0,0.0,1.0));
  norAttribute->setVertex3f(*vertexCounter+1, Vec3f(0.0,0.0,1.0) );
  norAttribute->setVertex3f(*vertexCounter+2, Vec3f(0.0,0.0,1.0) );
  norAttribute->setVertex3f(*vertexCounter+3, Vec3f(0.0,0.0,1.0) );

  texcoAttribute->setVertex3f(*vertexCounter, Vec3f(0.0,0.0,layer) );
  texcoAttribute->setVertex3f(*vertexCounter+1, Vec3f(0.0,data.uvY,layer) );
  texcoAttribute->setVertex3f(*vertexCounter+2, Vec3f(data.uvX,data.uvY,layer) );
  texcoAttribute->setVertex3f(*vertexCounter+3, Vec3f(data.uvX,0.0,layer) );

  *vertexCounter += 4;
}
