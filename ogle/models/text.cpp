/*
 * text.cpp
 *
 *  Created on: 23.03.2011
 *      Author: daniel
 */

#include <GL/glew.h>
#include <GL/gl.h>

#include <boost/algorithm/string.hpp>

#include "text.h"

#include <ogle/states/texture-state.h>
#include <ogle/shader/shader-generator.h>

Text::Text(
    FreeTypeFont &font,
    GLfloat height,
    GLboolean isOrtho,
    GLboolean useBackground)
: MeshState(GL_QUADS),
  font_(font),
  value_(),
  numCharacters_(0),
  height_(height),
  useBackground_(useBackground)
{
  ref_ptr<Texture> tex = ref_ptr<Texture>::cast(font.texture());
  ref_ptr<State> texState = ref_ptr<State>::manage(new TextureState(tex));
  joinStates(texState);
}

void Text::set_height(GLfloat height)
{
  height_ = height;
  updateAttributes();
}

const list<wstring>& Text::value() const
{
  return value_;
}
void Text::set_value(
    const list<wstring> &value,
    Alignment alignment,
    GLfloat maxLineWidth)
{
  value_ = value;
  numCharacters_ = (useBackground_ ? 1 : 0);
  for(list<wstring>::const_iterator
      it = value.begin(); it != value.end(); ++it)
  {
    numCharacters_ += it->size();
  }
  updateAttributes(alignment, maxLineWidth);
}
void Text::set_value(
    const wstring &value,
    Alignment alignment,
    GLfloat maxLineWidth)
{
  list<wstring> v;
  boost::split(v, value, boost::is_any_of("\n"));
  set_value(v, alignment, maxLineWidth);
}

void Text::updateAttributes(Alignment alignment, GLfloat maxLineWidth)
{
  ref_ptr<PositionShaderInput> posAttribute =
      ref_ptr<PositionShaderInput>::manage(new PositionShaderInput);
  ref_ptr<NormalShaderInput> norAttribute =
      ref_ptr<NormalShaderInput>::manage(new NormalShaderInput);
  ref_ptr<TexcoShaderInput> texcoAttribute =
      ref_ptr<TexcoShaderInput>::manage(new TexcoShaderInput( 0, 3 ));


  Vec3f translation, glyphTranslation;
  GLuint vertexCounter = (useBackground_ ? 4u : 0u);

  GLfloat actualMaxLineWidth = 0.0;
  GLfloat actualHeight = 0.0;

  posAttribute->setVertexData(numCharacters_*4);
  texcoAttribute->setVertexData(numCharacters_*4);
  norAttribute->setVertexData(numCharacters_*4);

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
    for(GLint i=0; i<it->size(); ++i)
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
    for(GLint i=0; i<it->size(); ++i)
    {
      const wchar_t &ch = (*it)[i];
      const FaceData &data = font_.faceData(ch);

      glyphTranslation = Vec3f(
          data.left*height_, (data.top-data.height)*height_, 0.001*(i+1)
      );
      makeGlyphGeometry(data, translation+glyphTranslation, (float) ch,
              posAttribute.get(), norAttribute.get(),
              texcoAttribute.get(), &vertexCounter);

      // move cursor to next glyph
      translation.x += data.advanceX*height_;
    }

    translation.x = 0.0;
  }

  if(useBackground_)
  {
    // make background quad
    GLfloat bgOffset = 0.25*font_.lineHeight()*height_;
    actualHeight = abs(translation.y - bgOffset);
    actualMaxLineWidth += bgOffset;
    posAttribute->setVertex3f(0, Vec3f(-0.5*bgOffset, 0.5*bgOffset, -0.001) );
    posAttribute->setVertex3f(1, Vec3f(-0.5*bgOffset, -actualHeight, -0.001) );
    posAttribute->setVertex3f(2, Vec3f(actualMaxLineWidth, -actualHeight, -0.001) );
    posAttribute->setVertex3f(3, Vec3f(actualMaxLineWidth, 0.5*bgOffset, -0.001) );

    norAttribute->setVertex3f(0, Vec3f(0.0,0.0,1.0) );
    norAttribute->setVertex3f(1, Vec3f(0.0,0.0,1.0) );
    norAttribute->setVertex3f(2, Vec3f(0.0,0.0,1.0) );
    norAttribute->setVertex3f(3, Vec3f(0.0,0.0,1.0) );

    texcoAttribute->setVertex3f(0, Vec3f(0.0,0.0,font_.backgroundGlyph()) );
    texcoAttribute->setVertex3f(1, Vec3f(0.0,1.0,font_.backgroundGlyph()) );
    texcoAttribute->setVertex3f(2, Vec3f(1.0,1.0,font_.backgroundGlyph()) );
    texcoAttribute->setVertex3f(3, Vec3f(1.0,0.0,font_.backgroundGlyph()) );
  }

  setInput(ref_ptr<ShaderInput>::cast(posAttribute));
  setInput(ref_ptr<ShaderInput>::cast(norAttribute));
  setInput(ref_ptr<ShaderInput>::cast(texcoAttribute));
}

void Text::makeGlyphGeometry(
    const FaceData &data,
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
