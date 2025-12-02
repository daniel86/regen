#ifndef REGEN_RENDER_BUFFER_H_
#define REGEN_RENDER_BUFFER_H_

#include "regen/gl/gl-rectangle.h"

namespace regen {
	/**
	 * \brief OpenGL Objects that contain images.
	 *
	 * RenderBuffer's are created and used specifically with Framebuffer Objects, and
	 * are optimized for being used as render targets, while Textures may not be.
	 * RBOs cannot be directly accessed from shaders, and they do not support mipmapping
	 * or texture filtering.
	 */
	class RenderBuffer : public GLRectangle {
	public:
		/**
		 * @param numObjects number of GL buffers.
		 */
		explicit RenderBuffer(uint32_t numObjects = 1);

		/**
		 * @param format internal format of the renderbuffer.
		 * @param width width of the renderbuffer.
		 * @param height height of the renderbuffer.
		 * @param numObjects number of GL buffers.
		 */
		RenderBuffer(GLenum format,
					 uint32_t width,
					 uint32_t height,
					 uint32_t numObjects = 1);

		/**
		 * Specifies the internal format to use for the renderbuffer object's image.
		 * Accepted values are GL_R*, GL_RG*, GL_RGB* GL_RGBA*, GL_DEPTH_COMPONENT*,
		 * GL_SRGB*, GL_COMPRESSED_*.
		 */
		void set_format(GLenum format) { format_ = format; }

		/**
		 * Establish data storage, format and dimensions
		 * of a renderbuffer object's image using multisampling.
		 */
		void storageMS(uint32_t numMultisamples) const;

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
