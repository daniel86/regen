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
		 * @param createObjects allocate buffers.
		 * @param releaseObjects delete buffers.
		 * @param numObjects number of buffers to allocate.
		 */
		GLRectangle(
				CreateObjectFunc2 createObjects,
				ReleaseObjectFunc releaseObjects,
				GLenum objectTarget,
				GLuint numObjects = 1);

		/**
		 * Set the buffer size.
		 */
		void set_rectangleSize(uint32_t width, uint32_t height);

		/**
		 * Width of the buffer.
		 */
		inline uint32_t width() const { return sizeUI_.x; }

		/**
		 * Height of the buffer.
		 */
		inline uint32_t height() const { return sizeUI_.y; }

		/**
		 * @return The inverse rectangle size.
		 */
		inline const Vec2f &sizeInverse() const { return sizeInverse_; }

		/**
		 * @return The rectangle size.
		 */
		inline const Vec2f &size() const { return size_; }

	protected:
		Vec2ui sizeUI_;
		Vec2f size_;
		Vec2f sizeInverse_;
	};
} // namespace

#endif /* GL_RECTANGLE_H_ */
