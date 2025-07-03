#include "bloom-texture.h"

using namespace regen;

BloomTexture::BloomTexture(GLuint numMips) : TextureMips2D(numMips) {
	// TODO: rather use one texture and set its mipmap levels manually?
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

void BloomTexture::resize(GLuint width, GLuint height) {
	auto i_mipSize = Vec2ui(width, height);
	auto f_mipSize = Vec2f(
		static_cast<float>(i_mipSize.x),
		static_cast<float>(i_mipSize.y));

	for (auto &mip : mips_) {
		// next mip is half the size of the previous mip
        f_mipSize *= 0.5f;
        i_mipSize /= 2;
        mip.sizeInverse = Vec2f(1.0f / f_mipSize.x, 1.0f / f_mipSize.y);
        mip.glViewport = Viewport(0, 0, i_mipSize.x, i_mipSize.y);

		mip.texture->set_rectangleSize(i_mipSize.x, i_mipSize.y);
		mip.texture->allocTexture();
		mip.texture->set_filter(GL_LINEAR);
		mip.texture->set_wrapping(GL_CLAMP_TO_EDGE);
	}
}
