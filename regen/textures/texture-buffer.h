#ifndef REGEN_TEXTURE_BUFFER_H_
#define REGEN_TEXTURE_BUFFER_H_

#include "texture.h"
#include "regen/gl-types/vbo.h"

namespace regen {
	/**
	 * \brief One-dimensional arrays of texels whose storage
	 * comes from an attached buffer object.
	 *
	 * When a buffer object is bound to a buffer texture,
	 * a format is specified, and the data in the buffer object
	 * is treated as an array of texels of the specified format.
	 */
	class TextureBuffer : public Texture {
	public:
		/**
		 * Accepted values are GL_R*, GL_RG*, GL_RGB* GL_RGBA*, GL_DEPTH_COMPONENT*,
		 * GL_SRGB*, GL_COMPRESSED_*.
		 */
		explicit TextureBuffer(GLenum texelFormat);

		/**
		 * Attach VBO to TextureBuffer and keep a reference on the VBO.
		 */
		void attach(const ref_ptr<VBO> &vbo, VBOReference &ref);

		/**
		 * Attach the storage for a buffer object to the active buffer texture.
		 */
		void attach(GLuint storage);

		/**
		 * Attach the storage for a buffer object to the active buffer texture.
		 */
		void attach(GLuint storage, GLuint offset, GLuint size);

		// override
		unsigned int numTexel() const override;


	private:
		GLenum texelFormat_;
		ref_ptr<VBO> attachedVBO_;
		VBOReference attachedVBORef_;

		// override
		void texImage() const override;
	};
} // namespace

#endif /* REGEN_TEXTURE_BUFFER_H_ */
