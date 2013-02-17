/*
 * texture-mapped-text.h
 *
 *  Created on: 23.03.2011
 *      Author: daniel
 */

#ifndef TEXT_H_
#define TEXT_H_

#include <ogle/meshes/mesh-state.h>
#include <ogle/utility/free-type.h>

/**
 * A mesh containing some text.
 * This is done using texture mapped fonts.
 * The Font is saved in a texture array, the glyphs are
 * accessed by the w texture coordinate.
 */
class TextureMappedText : public MeshState
{
public:
  /**
   * Define how text is aligned.
   */
  enum Alignment { ALIGNMENT_LEFT, ALIGNMENT_RIGHT, ALIGNMENT_CENTER };

  TextureMappedText(FreeTypeFont &font, GLfloat height);

  void set_bgColor(const Vec4f &color);
  void set_fgColor(const Vec4f &color);

  /**
   * Returns text as list of lines.
   */
  const list<wstring>& value() const;
  /**
   * Sets the text to be displayed.
   */
  void set_value(
      const wstring &value,
      Alignment alignment=ALIGNMENT_LEFT,
      GLfloat maxLineWidth=0.0f);
  /**
   * Sets the text to be displayed.
   */
  void set_value(
      const list<wstring> &lines,
      Alignment alignment=ALIGNMENT_LEFT,
      GLfloat maxLineWidth=0.0f);

  /**
   * Sets the y value of the primitive-set  topmoset point.
   */
  void set_height(GLfloat height);

protected:
  const FreeTypeFont &font_;
  list<wstring> value_;
  GLfloat height_;
  GLuint numCharacters_;

  ref_ptr<ShaderInput1i> bgToggle_;
  ref_ptr<ShaderInput4f> bgColor_;
  ref_ptr<ShaderInput4f> fgColor_;

  ref_ptr<PositionShaderInput> posAttribute_;
  ref_ptr<NormalShaderInput> norAttribute_;
  ref_ptr<TexcoShaderInput> texcoAttribute_;

  void updateAttributes(Alignment alignment, GLfloat maxLineWidth);
  void makeGlyphGeometry(
      const FaceData &data,
      const Vec3f &translation,
      GLfloat layer,
      VertexAttribute *posAttribute,
      VertexAttribute *norAttribute,
      VertexAttribute *texcoAttribute,
      GLuint *vertexCounter);
};

#endif /* TEXT_H_ */
