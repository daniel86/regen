#include "screen.h"

using namespace regen;

Screen::Screen(const Vec2i &viewport) {
	// Note: should be in non-frame-locked mode, as readers might always need
	// to read the newest viewport data.
	sh_viewport_ = ref_ptr<ShaderInput2i>::alloc("viewport");
	sh_viewport_->setUniformData(viewport);
}

void Screen::setViewport(const Vec2i &viewport) {
	sh_viewport_->setVertex(0, viewport);
}
