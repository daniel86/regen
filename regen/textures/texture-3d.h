#ifndef REGEN_TEXTURE_3D_H_
#define REGEN_TEXTURE_3D_H_

#include <regen/textures/texture.h>

namespace regen {
	/**
	 * \brief A 3 dimensional texture.
	 */
	class Texture3D : public Texture {
	public:
		/**
		 * @param numTextures number of texture images.
		 */
		explicit Texture3D(GLuint numTextures = 1);

		/**
		 * @param depth the texture depth.
		 */
		void set_depth(GLuint depth);

		/**
		 * @return the texture depth.
		 */
		GLuint depth() const { return numTextures_; }

		/**
		 * Specify a single layer of the 3D texture.
		 * @param layer the texture layer.
		 * @param subData data for the layer.
		 */
		virtual void texSubImage(GLint layer, GLubyte *subData) const;

		// override
		void texImage() const override;

		// override
		unsigned int numTexel() const override { return width() * height() * depth(); }

	protected:
		GLuint numTextures_;
	};

	/**
	 * \brief A 3 dimensional depth texture.
	 */
	class Texture3DDepth : public Texture3D {
	public:
		/**
		 * @param numTextures number of texture images.
		 */
		explicit Texture3DDepth(GLuint numTextures = 1);
	};
} // namespace

#endif /* REGEN_TEXTURE_3D_H_ */
