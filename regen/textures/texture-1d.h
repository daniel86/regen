#ifndef REGEN_TEXTURE_1D_H_
#define REGEN_TEXTURE_1D_H_

#include <regen/textures/texture.h>

namespace regen {
	/**
	 * \brief Images in this texture are all 1-dimensional.
	 *
	 * They have width, but no height or depth.
	 */
	class Texture1D : public Texture {
	public:
		/**
		 * @param numTextures number of texture images.
		 */
		explicit Texture1D(GLuint numTextures = 1);

		// override
		void texImage() const override;

		// override
		unsigned int numTexel() const override { return width(); }
	};
} // namespace

#endif /* REGEN_TEXTURE_1D_H_ */
