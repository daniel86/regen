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
: AttributeState(GL_QUADS),
  font_(font),
  value_(),
  numCharacters_(0),
  height_(height),
  useBackground_(useBackground)
{
  ref_ptr<Texture> tex = font.texture();
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
  ref_ptr<VertexAttributefv> posAttribute = ref_ptr<VertexAttributefv>::manage(
      new VertexAttributefv( ATTRIBUTE_NAME_POS ));
  ref_ptr<VertexAttributefv> norAttribute = ref_ptr<VertexAttributefv>::manage(
      new VertexAttributefv( ATTRIBUTE_NAME_NOR ));
  ref_ptr<VertexAttributefv> texcoAttribute = ref_ptr<VertexAttributefv>::manage(
      new TexcoAttribute( 0, 3 ));

  ref_ptr< vector<GLuint> > indexes;
  vector<MeshFace> faces;
  Vec3f translation, glyphTranslation;
  GLuint nextIndex = (useBackground_ ? 4u : 0u); // background quad is first
  GLuint vertexCounter = (useBackground_ ? 4u : 0u);

  GLfloat actualMaxLineWidth = 0.0;
  GLfloat actualHeight = 0.0;

  indexes = ref_ptr< vector<GLuint> >::manage(new vector<GLuint>(4));
  posAttribute->setVertexData(numCharacters_*4);
  texcoAttribute->setVertexData(numCharacters_*4);
  norAttribute->setVertexData(numCharacters_*4);

  translation = Vec3f(0.0,0.0,0.0);
  glyphTranslation = Vec3f(0.0,0.0,0.0);

  if(useBackground_) {
    indexes->push_back(0);
    indexes->push_back(1);
    indexes->push_back(2);
    indexes->push_back(3);
  }

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
              indexes, posAttribute.get(), norAttribute.get(),
              texcoAttribute.get(), &nextIndex, &vertexCounter);

      faces.push_back( (MeshFace){indexes} );
      indexes = ref_ptr< vector<GLuint> >::manage(new vector<GLuint>(4));

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
    setAttributeVertex3f(posAttribute.get(), 0, Vec3f(-0.5*bgOffset, 0.5*bgOffset, -0.001) );
    setAttributeVertex3f(posAttribute.get(), 1, Vec3f(-0.5*bgOffset, -actualHeight, -0.001) );
    setAttributeVertex3f(posAttribute.get(), 2, Vec3f(actualMaxLineWidth, -actualHeight, -0.001) );
    setAttributeVertex3f(posAttribute.get(), 3, Vec3f(actualMaxLineWidth, 0.5*bgOffset, -0.001) );

    setAttributeVertex3f(norAttribute.get(), 0, Vec3f(0.0,0.0,1.0) );
    setAttributeVertex3f(norAttribute.get(), 1, Vec3f(0.0,0.0,1.0) );
    setAttributeVertex3f(norAttribute.get(), 2, Vec3f(0.0,0.0,1.0) );
    setAttributeVertex3f(norAttribute.get(), 3, Vec3f(0.0,0.0,1.0) );

    setAttributeVertex3f(texcoAttribute.get(), 0, Vec3f(0.0,0.0,font_.backgroundGlyph()) );
    setAttributeVertex3f(texcoAttribute.get(), 1, Vec3f(0.0,1.0,font_.backgroundGlyph()) );
    setAttributeVertex3f(texcoAttribute.get(), 2, Vec3f(1.0,1.0,font_.backgroundGlyph()) );
    setAttributeVertex3f(texcoAttribute.get(), 3, Vec3f(1.0,0.0,font_.backgroundGlyph()) );
  }

  setFaces(faces, 4);
  setAttribute(posAttribute);
  setAttribute(norAttribute);
  setAttribute(texcoAttribute);
}

void Text::makeGlyphGeometry(
    const FaceData &data,
    const Vec3f &translation,
    GLfloat layer,
    ref_ptr< vector<GLuint> > &indexes,
    VertexAttributefv *posAttribute,
    VertexAttributefv *norAttribute,
    VertexAttributefv *uvAttribute,
    GLuint *nextIndex,
    GLuint *vertexCounter)
{
  indexes->push_back(*nextIndex);
  indexes->push_back(*nextIndex + 1);
  indexes->push_back(*nextIndex + 2);
  indexes->push_back(*nextIndex + 3);
  *nextIndex += 4;

  setAttributeVertex3f(posAttribute, *vertexCounter,
              translation + Vec3f(0.0,data.height*height_,0.0) );
  setAttributeVertex3f(posAttribute, *vertexCounter+1,
              translation + Vec3f(0.0,0.0,0.0) );
  setAttributeVertex3f(posAttribute, *vertexCounter+2,
              translation + Vec3f(data.width*height_,0.0,0.0) );
  setAttributeVertex3f(posAttribute, *vertexCounter+3,
              translation + Vec3f(data.width*height_,data.height*height_,0.0) );

  setAttributeVertex3f(norAttribute, *vertexCounter, Vec3f(0.0,0.0,1.0));
  setAttributeVertex3f(norAttribute, *vertexCounter+1, Vec3f(0.0,0.0,1.0) );
  setAttributeVertex3f(norAttribute, *vertexCounter+2, Vec3f(0.0,0.0,1.0) );
  setAttributeVertex3f(norAttribute, *vertexCounter+3, Vec3f(0.0,0.0,1.0) );

  setAttributeVertex3f(uvAttribute, *vertexCounter, Vec3f(0.0,0.0,layer) );
  setAttributeVertex3f(uvAttribute, *vertexCounter+1, Vec3f(0.0,data.uvY,layer) );
  setAttributeVertex3f(uvAttribute, *vertexCounter+2, Vec3f(data.uvX,data.uvY,layer) );
  setAttributeVertex3f(uvAttribute, *vertexCounter+3, Vec3f(data.uvX,0.0,layer) );

  *vertexCounter += 4;
}
