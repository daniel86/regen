#ifndef GL_RECTANGLE_H_
#define GL_RECTANGLE_H_

#include <regen/gl-types/gl-object.h>
#include <regen/gl-types/shader-input.h>

namespace regen {
	/**
	 * \brief A 2D rectangular buffer.
	 */
	class GLRectangle : public GLObject {
	public:
		/**
		 * @param createObjects allocate buffers.
		 * @param releaseObjects delete buffers.
		 * @param numObjects number of buffers to allocate.
		 */
		GLRectangle(
				CreateObjectFunc createObjects,
				ReleaseObjectFunc releaseObjects,
				GLuint numObjects = 1);

		/**
		 * Set the buffer size.
		 */
		void set_rectangleSize(GLuint width, GLuint height);

		/**
		 * Width of the buffer.
		 */
		GLuint width() const;

		/**
		 * Height of the buffer.
		 */
		GLuint height() const;

		/**
		 * @return The inverse rectangle size.
		 */
		const ref_ptr<ShaderInput2f> &sizeInverse() const;

		/**
		 * @return The rectangle size.
		 */
		const ref_ptr<ShaderInput2f> &size() const;

	protected:
		ref_ptr<ShaderInput2f> size_;
		ref_ptr<ShaderInput2f> sizeInverse_;
	};
} // namespace

#endif /* GL_RECTANGLE_H_ */
