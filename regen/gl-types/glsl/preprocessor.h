/*
 * preprocessor.h
 *
 *  Created on: 12.05.2013
 *      Author: daniel
 */

#ifndef GLSL_PREPROCESSOR_H_
#define GLSL_PREPROCESSOR_H_

#include <regen/gl-types/glsl/glsl-processor.h>

namespace regen {
  /**
   * \brief A sequence of GLSL pre-processors.
   */
  class PreProcessor {
  public:
    PreProcessor();

    /**
     * Add a pre-processor to the sequence.
     */
    void addProcessor(const ref_ptr<GLSLProcessor> &processor);
    /**
     * Removes previously added pre-processor.
     */
    void removeProcessor(GLSLProcessor *processor);

    /**
     * Pre-process the specified input.
     */
    map<GLenum,string> processStages(const PreProcessorInput &in);

  protected:
    ref_ptr<GLSLProcessor> lastProcessor_;
  };
}

#endif /* PREPROCESSOR_H_ */
