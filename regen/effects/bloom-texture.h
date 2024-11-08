#ifndef REGEN_BLOOM_TEXTURE_H
#define REGEN_BLOOM_TEXTURE_H

#include <regen/gl-types/texture.h>

namespace regen {
	/**
	 * \brief A mip mapped texture used for the bloom effect.
	 */
	class BloomTexture : public TextureMips2D {
	public:
		struct Mip {
			Texture *texture;
			Viewport glViewport;
			Vec2f sizeInverse;
		};

		explicit BloomTexture(GLuint numMips = 5);

		void resize(GLuint width, GLuint height);

		auto &mips() { return mips_; }

		auto numMips() const { return mips_.size(); }

	protected:
		std::vector<Mip> mips_;
	};
}

#endif //REGEN_BLOOM_TEXTURE_H
