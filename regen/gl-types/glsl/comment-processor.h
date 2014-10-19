/*
 * comment-processor.h
 *
 *  Created on: 19.05.2013
 *      Author: daniel
 */

#ifndef COMMENT_PROCESSOR_H_
#define COMMENT_PROCESSOR_H_

#include <GL/glew.h>
#include <regen/gl-types/glsl/glsl-processor.h>

#include <iostream>

namespace regen {
  /**
   * \brief Removes comments from GLSL code.
   */
  class CommentProcessor : public GLSLProcessor {
  public:
    CommentProcessor();

    // override
    bool process(PreProcessorState &state, std::string &line);
    void clear();
  protected:
    bool commentActive_;
  };
} // namespace

#endif /* COMMENT_PROCESSOR_H_ */
