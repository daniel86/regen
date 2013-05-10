/*
 * texture-mapped-text.cpp
 *
 *  Created on: 23.03.2011
 *      Author: daniel
 */

#include <GL/glew.h>

#include <boost/algorithm/string.hpp>
#include <regen/states/texture-state.h>

#include "texture-mapped-text.h"
using namespace regen;

TextureMappedText::TextureMappedText(Font &font, GLfloat height)
: Mesh(GL_TRIANGLES, VBO::USAGE_DYNAMIC),
  HasShader("gui.text"),
  font_(font),
  value_(),
  height_(height),
  numCharacters_(0)
{
  textColor_ = ref_ptr<ShaderInput4f>::manage(new ShaderInput4f("textColor"));
  textColor_->setUniformData(Vec4f(1.0));
  joinShaderInput(textColor_);

  ref_ptr<Texture> tex = font.texture();
  ref_ptr<TextureState> texState = ref_ptr<TextureState>::manage(new TextureState(tex, "fontTexture"));
  texState->set_mapTo(TextureState::MAP_TO_COLOR);
  texState->set_blendMode(BLEND_MODE_SRC_ALPHA);
  joinStates(texState);

  joinStates(shaderState());

  posAttribute_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f(ATTRIBUTE_NAME_POS));
  norAttribute_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f(ATTRIBUTE_NAME_NOR));
  texcoAttribute_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("texco0"));
}

void TextureMappedText::createShader(const StateConfig &cfg)
{
  shaderState_->createShader(cfg,shaderKey_);
  initializeResources(RenderState::get(), cfg, shaderState_->shader());
}

void TextureMappedText::set_color(const Vec4f &color)
{
  textColor_->setVertex4f(0, color);
}

void TextureMappedText::set_height(GLfloat height)
{ height_ = height; }

const list<wstring>& TextureMappedText::value() const
{ return value_; }

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

  posAttribute_->setVertexData(numCharacters_*6);
  texcoAttribute_->setVertexData(numCharacters_*6);
  norAttribute_->setVertexData(numCharacters_*6);

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
      const Font::FaceData &data = font_.faceData(ch);

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

  begin(ShaderInputContainer::INTERLEAVED);
  setInput(posAttribute_);
  setInput(norAttribute_);
  setInput(texcoAttribute_);
  end();
}

void TextureMappedText::makeGlyphGeometry(
    const Font::FaceData &data,
    const Vec3f &translation,
    GLfloat layer,
    ShaderInput *posAttribute,
    ShaderInput *norAttribute,
    ShaderInput *texcoAttribute,
    GLuint *vertexCounter)
{
  GLuint &i = *vertexCounter;
  Vec3f p0 = translation + Vec3f(0.0,data.height*height_,0.0);
  Vec3f p1 = translation + Vec3f(0.0,0.0,0.0);
  Vec3f p2 = translation + Vec3f(data.width*height_,0.0,0.0);
  Vec3f p3 = translation + Vec3f(data.width*height_,data.height*height_,0.0);
  Vec3f n = Vec3f(0.0,0.0,1.0);
  Vec3f texco0(0.0,0.0,layer);
  Vec3f texco1(0.0,data.uvY,layer);
  Vec3f texco2(data.uvX,data.uvY,layer);
  Vec3f texco3(data.uvX,0.0,layer);

  posAttribute->setVertex3f(i,   p0);
  posAttribute->setVertex3f(i+1, p1);
  posAttribute->setVertex3f(i+2, p2);
  posAttribute->setVertex3f(i+3, p2);
  posAttribute->setVertex3f(i+4, p3);
  posAttribute->setVertex3f(i+5, p0);

  norAttribute->setVertex3f(i,   n);
  norAttribute->setVertex3f(i+1, n);
  norAttribute->setVertex3f(i+2, n);
  norAttribute->setVertex3f(i+3, n);
  norAttribute->setVertex3f(i+4, n);
  norAttribute->setVertex3f(i+5, n);

  texcoAttribute->setVertex3f(i,   texco0);
  texcoAttribute->setVertex3f(i+1, texco1);
  texcoAttribute->setVertex3f(i+2, texco2);
  texcoAttribute->setVertex3f(i+3, texco2);
  texcoAttribute->setVertex3f(i+4, texco3);
  texcoAttribute->setVertex3f(i+5, texco0);

  *vertexCounter += 6;
}
