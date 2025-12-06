#ifndef REGEN_GLSL_PREPROCESSOR_H_
#define REGEN_GLSL_PREPROCESSOR_H_

#include "glsl-processor.h"

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
		std::map<GLenum, std::string> processStages(const PreProcessorInput &in);

	protected:
		ref_ptr<GLSLProcessor> lastProcessor_;
	};
}

#endif /* REGEN_GLSL_PREPROCESSOR_H_ */
