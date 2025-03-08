#ifndef REGEN_TEXTURE_2D_H_
#define REGEN_TEXTURE_2D_H_

#include <regen/textures/texture.h>

namespace regen {
	/**
	 * \brief Images in this texture all are 2-dimensional.
	 *
	 * They have width and height, but no depth.
	 */
	class Texture2D : public Texture {
	public:
		static constexpr const char *TYPE_NAME = "Texture2D";

		/**
		 * @param numTextures number of texture images.
		 */
		explicit Texture2D(GLuint numTextures = 1);

		// override
		void texImage() const override;

		// override
		unsigned int numTexel() const override { return width() * height(); }
	};

	/**
	 * \brief A texture with multiple mipmap textures.
	 *
	 * Note this is not using the GL mipmapping feature,
	 * downscaling must be done manually.
	 */
	class TextureMips2D : public Texture2D {
	public:
		explicit TextureMips2D(GLuint numMips = 4);

		auto &mipTextures() { return mipTextures_; }

		auto &mipRefs() { return mipRefs_; }

		auto numMips() const { return numMips_; }

	protected:
		std::vector<Texture *> mipTextures_;
		std::vector<ref_ptr<Texture2D>> mipRefs_;
		GLuint numMips_;
	};

	/**
	 * \brief The image in this texture (only one image. No mipmapping)
	 * is 2-dimensional.
	 *
	 * Texture coordinates used for these
	 * textures are not normalized.
	 */
	class TextureRectangle : public Texture2D {
	public:
		/**
		 * @param numTextures number of texture images.
		 */
		explicit TextureRectangle(GLuint numTextures = 1);
	};

	/**
	 * \brief Texture with depth format.
	 */
	class Texture2DDepth : public Texture2D {
	public:
		/**
		 * @param numTextures number of texture images.
		 */
		explicit Texture2DDepth(GLuint numTextures = 1);
	};

	/**
	 * \brief The image in this texture (only one image. No mipmapping) is 2-dimensional.
	 *
	 * Each pixel in these images contains multiple samples instead
	 * of just one value.
	 */
	class Texture2DMultisample : public Texture2D {
	public:
		/**
		 * @param numSamples number of samples per texel.
		 * @param numTextures number of texture images.
		 * @param fixedLocations use fixed locations.
		 */
		explicit Texture2DMultisample(
				GLsizei numSamples,
				GLuint numTextures = 1,
				GLboolean fixedLocations = GL_FALSE);

		// override
		void texImage() const override;

	private:
		GLboolean fixedsamplelocations_;
	};

	/**
	 * \brief The image in this texture (only one image. No mipmapping) is 2-dimensional.
	 *
	 * Each pixel in these images contains multiple samples instead
	 * of just one value.
	 * Uses a depth format.
	 */
	class Texture2DMultisampleDepth : public Texture2DDepth {
	public:
		/**
		 * @param numSamples number of samples per texel.
		 * @param fixedLocations use fixed locations.
		 */
		explicit Texture2DMultisampleDepth(GLsizei numSamples, GLboolean fixedLocations = GL_FALSE);

		// override
		void texImage() const override;

	private:
		GLboolean fixedsamplelocations_;
	};
} // namespace

#endif /* REGEN_TEXTURE_2D_H_ */
