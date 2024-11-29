#ifndef REGEN_RAMP_TEXTURE_H_
#define REGEN_RAMP_TEXTURE_H_

#include <regen/gl-types/texture.h>
#include <regen/utility/ref-ptr.h>

namespace regen {
	/**
	 * A ramp texture is a texture that is used to map a scalar value to a color.
	 */
	class RampTexture : public Texture1D {
	public:
		/**
		 * Creates a ramp texture with the given format, internal format and width.
		 * @param format the texture format.
		 * @param internalFormat the texture internal format.
		 * @param width the texture width.
		 */
		RampTexture(GLenum format, GLenum internalFormat, GLuint width);

		/**
		 * Creates a ramp texture with the given format and data.
		 * @param format the texture format.
		 * @param data the texture data.
		 */
		RampTexture(GLenum format, const std::vector<GLuint> &data);

		/**
		 * Creates a ramp texture with the given format, internal format and data.
		 * @param format the texture format.
		 * @param internalFormat the texture internal format.
		 * @param data the texture data.
		 */
		RampTexture(GLenum format, GLenum internalFormat, const std::vector<GLuint> &data);

		~RampTexture() override = default;

		/**
		 * A ramp with one dark and one white texel.
		 */
		static ref_ptr<RampTexture> darkWhite();

		/**
		 * A ramp with three dark and two white texels.
		 */
		static ref_ptr<RampTexture> darkWhiteSkewed();

		/**
		 * A ramp with one black and one white texel.
		 */
		static ref_ptr<RampTexture> normal();

		/**
		 * A ramp from dark to white in three steps.
		 */
		static ref_ptr<RampTexture> threeStep();

		/**
		 * A ramp from dark to white in four steps.
		 */
		static ref_ptr<RampTexture> fourStep();

		/**
		 * A ramp from dark to white in four steps, but skewed.
		 */
		static ref_ptr<RampTexture> fourStepSkewed();

		/**
		 * A ramp from black to white to black.
		 */
		static ref_ptr<RampTexture> blackWhiteBlack();

		/**
		 * A ramp with black and white stripes.
		 */
		static ref_ptr<RampTexture> stripes();

		/**
		 * A ramp with a single stripe.
		 */
		static ref_ptr<RampTexture> stripe();

		/**
		 * A ramp with red, green and blue texels.
		 */
		static ref_ptr<RampTexture> rgb();
	};
} // namespace

#endif /* REGEN_RAMP_TEXTURE_H_ */
