#ifndef REGEN_TEXTURE_ARRAY_H_
#define REGEN_TEXTURE_ARRAY_H_

#include <regen/textures/texture-3d.h>

namespace regen {
	/**
	 * \brief Array of two dimensional textures.
	 */
	class Texture2DArray : public Texture3D {
	public:
		/**
		 * @param numTextures number of texture images.
		 */
		explicit Texture2DArray(GLuint numTextures = 1);
	};

	class Texture2DArrayDepth : public Texture2DArray {
	public:
		/**
		 * @param numTextures number of texture images.
		 */
		explicit Texture2DArrayDepth(GLuint numTextures = 1);
	};
} // namespace

#endif /* REGEN_TEXTURE_ARRAY_H_ */
