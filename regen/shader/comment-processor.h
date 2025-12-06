#ifndef REGEN_COMMENT_PROCESSOR_H_
#define REGEN_COMMENT_PROCESSOR_H_

#include "glsl-processor.h"

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

#endif /* REGEN_COMMENT_PROCESSOR_H_ */
