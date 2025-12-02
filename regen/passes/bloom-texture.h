#ifndef REGEN_BLOOM_TEXTURE_H
#define REGEN_BLOOM_TEXTURE_H

#include <regen/textures/texture.h>

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

		explicit BloomTexture(uint32_t numMips = 5);

		auto &mips() { return mips_; }

		void resize(uint32_t width, uint32_t height);

	protected:
		std::vector<Mip> mips_;
	};
}

#endif //REGEN_BLOOM_TEXTURE_H
