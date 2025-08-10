#include "screen.h"

using namespace regen;

Screen::Screen(const Vec2i &viewport)
		: v_viewport_(viewport) {
	sh_viewport_ = ref_ptr<ShaderInput2i>::alloc("viewport");
	sh_viewport_->setUniformData(v_viewport_);
}

void Screen::setViewport(const Vec2i &viewport) {
	v_viewport_ = viewport;
	sh_viewport_->setVertex(0, v_viewport_);
}
