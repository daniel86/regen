/*
 * texture-mapped-text.h
 *
 *  Created on: 23.03.2011
 *      Author: daniel
 */

#ifndef TEXT_H_
#define TEXT_H_

#include <regen/meshes/mesh-state.h>
#include <regen/utility/free-type.h>

namespace regen {
/**
 * \brief Implements texture mapped text rendering.
 *
 * This is done using texture mapped fonts.
 * The Font is saved in a texture array, the glyphs are
 * accessed by the w texture coordinate.
 */
class TextureMappedText : public Mesh
{
public:
  /**
   * Defines how text is aligned.
   */
  enum Alignment {
    ALIGNMENT_LEFT,
    ALIGNMENT_RIGHT,
    ALIGNMENT_CENTER
  };

  /**
   * @param font font for the text.
   * @param height text height.
   */
  TextureMappedText(FreeTypeFont &font, GLfloat height);

  /**
   * @param color the text color.
   */
  void set_color(const Vec4f &color);

  /**
   * @return text as list of lines.
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
   * Sets the y value of the primitive-set topmost point.
   */
  void set_height(GLfloat height);

protected:
  const FreeTypeFont &font_;
  list<wstring> value_;
  GLfloat height_;
  GLuint numCharacters_;

  ref_ptr<ShaderInput4f> textColor_;

  ref_ptr<ShaderInput3f> posAttribute_;
  ref_ptr<ShaderInput3f> norAttribute_;
  ref_ptr<ShaderInput3f> texcoAttribute_;

  void updateAttributes(Alignment alignment, GLfloat maxLineWidth);
  void makeGlyphGeometry(
      const FreeTypeFont::FaceData &data,
      const Vec3f &translation,
      GLfloat layer,
      VertexAttribute *posAttribute,
      VertexAttribute *norAttribute,
      VertexAttribute *texcoAttribute,
      GLuint *vertexCounter);
};
} // namespace

#endif /* TEXT_H_ */
