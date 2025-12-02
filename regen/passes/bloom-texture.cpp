#include "bloom-texture.h"

using namespace regen;

BloomTexture::BloomTexture(uint32_t numMips) : TextureMips2D(numMips) {
	mips_.resize(numMips);
	mips_[0].texture = this;
	for (auto i = 0u; i < numMips; ++i) {
		auto &mip = mips_[i];
		mip.texture = mipTextures_[i];
		mip.texture->set_internalFormat(GL_R11F_G11F_B10F);
		mip.texture->set_format(GL_RGB);
		mip.texture->set_pixelType(GL_FLOAT);
	}
}

void BloomTexture::resize(uint32_t width, uint32_t height) {
	auto i_mipSize = Vec2ui(width, height);
	auto f_mipSize = Vec2f(
		static_cast<float>(i_mipSize.x),
		static_cast<float>(i_mipSize.y));

	for (auto &mip : mips_) {
        mip.sizeInverse = Vec2f(1.0f / f_mipSize.x, 1.0f / f_mipSize.y);
        mip.glViewport = Viewport(0, 0, i_mipSize.x, i_mipSize.y);

		mip.texture->set_rectangleSize(i_mipSize.x, i_mipSize.y);
		mip.texture->allocTexture();
		mip.texture->set_filter(TextureFilter::create(GL_LINEAR));
		mip.texture->set_wrapping(TextureWrapping::create(GL_CLAMP_TO_EDGE));
		// next mip is half the size of the previous mip
        f_mipSize *= 0.5f;
        i_mipSize /= 2;
	}
}
