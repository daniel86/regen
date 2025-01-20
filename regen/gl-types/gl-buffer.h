#ifndef REGEN_GL_BUFFER_H_
#define REGEN_GL_BUFFER_H_

#include <regen/gl-types/gl-object.h>

namespace regen {
	/**
	 * \brief Base class for OpenGL buffer objects.
	 */
	class GLBuffer : public GLObject {
	public:
		explicit GLBuffer(GLenum bufferTarget);

		~GLBuffer() override = default;

		GLBuffer(const GLBuffer &) = delete;

		void bindBufferBase(GLuint bindingPoint) const;

	protected:
		GLenum bufferTarget_;
	};
} // namespace

#endif /* REGEN_UBO_H_ */
