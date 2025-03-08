#ifndef REGEN_RENDER_BUFFER_H_
#define REGEN_RENDER_BUFFER_H_

#include <regen/gl-types/gl-rectangle.h>

namespace regen {
	/**
	 * \brief OpenGL Objects that contain images.
	 *
	 * RenderBuffer's are created and used specifically with Framebuffer Objects.
	 * RenderBuffer's are optimized for being used as render targets,
	 * while Textures may not be.
	 */
	class RenderBuffer : public GLRectangle {
	public:
		/**
		 * @param numObjects number of GL buffers.
		 */
		explicit RenderBuffer(GLuint numObjects = 1);

		/**
		 * Binds this RenderBuffer.
		 * Call end when you are done.
		 */
		void begin(RenderState *rs);

		/**
		 * Complete previous call to begin.
		 */
		void end(RenderState *rs);

		/**
		 * Specifies the internal format to use for the renderbuffer object's image.
		 * Accepted values are GL_R*, GL_RG*, GL_RGB* GL_RGBA*, GL_DEPTH_COMPONENT*,
		 * GL_SRGB*, GL_COMPRESSED_*.
		 */
		void set_format(GLenum format);

		/**
		 * Establish data storage, format and dimensions
		 * of a renderbuffer object's image using multisampling.
		 */
		void storageMS(GLuint numMultisamples) const;

		/**
		 * Establish data storage, format and dimensions of a
		 * renderbuffer object's image
		 */
		void storage() const;

	protected:
		GLenum format_;
	};
} // namespace

#endif /* REGEN_RENDER_BUFFER_H_ */
