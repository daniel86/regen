/*
 * vao.h
 *
 *  Created on: 07.08.2012
 *      Author: daniel
 */

#ifndef VAO_H_
#define VAO_H_

#include <regen/gl-types/gl-object.h>

namespace regen {
  /**
   * \brief Vertex Array Objects (VAO) are OpenGL Objects that store the
   * set of bindings between Vertex Attributes and the user's source vertex data.
   *
   * VBOs store the actual vertex and index arrays,
   * while VAOs store the settings for interpreting the data in those arrays.
   *
   * The currently bound vertex array object is used for all commands
   * which modify vertex array state, such as VertexAttribPointer and
   * EnableVertexAttribArray; all commands which draw from vertex arrays,
   * such as DrawArrays and DrawElements; and all queries of vertex
   * array state.
   */
  class VAO : public GLObject
  {
  public:
    VAO();
  };
} // namespace

#endif /* VAO_H_ */