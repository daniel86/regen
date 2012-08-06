/*
 * text.h
 *
 *  Created on: 23.03.2011
 *      Author: daniel
 */

#ifndef TEXT_H_
#define TEXT_H_

#include <ogle/states/attribute-state.h>
#include <ogle/font/free-type.h>

/**
 * A mesh containing some text.
 */
class Text : public AttributeState
{
public:
  /**
   * Define how text is aligned.
   */
  enum Alignment { ALIGNMENT_LEFT, ALIGNMENT_RIGHT, ALIGNMENT_CENTER };

  /**
   * Default constructor.
   */
  Text(FreeTypeFont &font,
      float height=1.0,
      bool isOrtho=true,
      bool useBackground=false);

  /**
   * Returns text as list of lines.
   */
  const list<wstring>& value() const;
  /**
   * Sets the text to be displayed.
   */
  void set_value(const wstring &value,
      Alignment alignment=ALIGNMENT_LEFT,
      float maxLineWidth=0.0f);
  /**
   * Sets the text to be displayed.
   */
  void set_value(const list<wstring> &lines,
      Alignment alignment=ALIGNMENT_LEFT,
      float maxLineWidth=0.0f);

  /**
   * Sets the y value of the primitive-set  topmoset point.
   */
  void set_height(float height);

protected:
  static unsigned int textCounter;

  FreeTypeFont &font_;
  list<wstring> value_;
  float height_;
  GLuint numCharacters_;
  bool useBackground_;

  void makeGeometry(
      Alignment alignment=ALIGNMENT_LEFT,
      float maxLineWidth=0.0f);
  void makeGlyphGeometry(
      const FaceData &data,
      const Vec3f &translation,
      float layer,
      ref_ptr< vector<GLuint> > &indexes,
      VertexAttributefv *posAttribute,
      VertexAttributefv *norAttribute,
      VertexAttributefv *uvAttribute,
      unsigned int *nextIndex,
      GLuint *vertexCounter);
};

#endif /* TEXT_H_ */
