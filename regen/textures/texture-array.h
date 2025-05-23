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

	/**
	 * \brief Array of two dimensional textures with multiple samples.
	 */
	class Texture2DArrayMultisample : public Texture2DArray {
	public:
		/**
		 * @param numSamples number of samples per texel.
		 * @param numTextures number of texture images.
		 * @param fixedLocations use fixed locations.
		 */
		explicit Texture2DArrayMultisample(
				GLsizei numSamples,
				GLuint numTextures = 1,
				GLboolean fixedLocations = GL_FALSE);

	protected:
		uint32_t numSamples_;
		bool fixedSampleLocations_;
	};

	/**
	 * \brief Array of two dimensional textures with multiple samples.
	 */
	class Texture2DArrayMultisampleDepth : public Texture2DArray {
	public:
		/**
		 * @param numSamples number of samples per texel.
		 * @param numTextures number of texture images.
		 * @param fixedLocations use fixed locations.
		 */
		explicit Texture2DArrayMultisampleDepth(
				GLsizei numSamples,
				GLuint numTextures = 1,
				GLboolean fixedLocations = GL_FALSE);

	protected:
		uint32_t numSamples_;
		bool fixedSampleLocations_;
	};
} // namespace

#endif /* REGEN_TEXTURE_ARRAY_H_ */
