#include "texture-binder.h"
#include <regen/gl-types/gl-param.h>

using namespace regen;

TextureBinder::TextureBinder() {
	auto maxImageUnits = glParam<int32_t>(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
	REGEN_DEBUG("Texture binder initialized with " << maxImageUnits << " image units.");
	activeBindings_.resize(maxImageUnits, nullptr);
}

void TextureBinder::reset() {
	auto &self = TextureBinder::instance();
	for (auto& tex : self.activeBindings_) {
		if (tex) {
			tex->clearTextureChannel();
			tex = nullptr;
		}
	}
	self.nextUnit_ = 0;
}

void TextureBinder::bind(Texture *tex, int32_t unit) {
	if (activeBindings_[unit]) {
		activeBindings_[unit]->clearTextureChannel();
	}
	activeBindings_[unit] = tex;
	tex->setTextureChannel(unit);

	auto rs = RenderState::get();
	rs->textures().apply(unit, tex->textureBind());
}

int32_t TextureBinder::bind(Texture *tex) {
	auto &self = TextureBinder::instance();
	auto lastBinding = tex->textureChannel();

	if (lastBinding != -1) {
		auto actualTex = self.activeBindings_[lastBinding];
		if (actualTex == tex) {
			return lastBinding; // already bound to this unit
		} else {
			self.bind(tex, lastBinding);
			return lastBinding;
		}
	}

	int32_t nextUnit = self.nextUnit_++;
	self.bind(tex, nextUnit);
	if (self.nextUnit_ >= static_cast<int32_t>(self.activeBindings_.size())) {
		// start from the beginning after reaching the end
		self.nextUnit_ = 0;
	}
	return nextUnit;
}
