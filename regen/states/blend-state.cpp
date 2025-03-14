/*
 * blend-state.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include <regen/states/atomic-states.h>

#include "blend-state.h"

namespace regen {

	std::ostream &operator<<(std::ostream &out, const BlendMode &mode) {
		switch (mode) {
			case BLEND_MODE_FRONT_TO_BACK:
			case BLEND_MODE_SRC_ALPHA:
			case BLEND_MODE_ALPHA:
				return out << "src_alpha";
			case BLEND_MODE_BACK_TO_FRONT:
			case BLEND_MODE_DST_ALPHA:
				return out << "dst_alpha";
			case BLEND_MODE_MULTIPLY:
				return out << "mul";
			case BLEND_MODE_DIVIDE:
				return out << "div";
			case BLEND_MODE_DIFFERENCE:
				return out << "diff";
			case BLEND_MODE_SMOOTH_ADD:
				return out << "smooth_add";
			case BLEND_MODE_ADD:
				return out << "add";
			case BLEND_MODE_SUBTRACT:
				return out << "sub";
			case BLEND_MODE_REVERSE_SUBTRACT:
				return out << "reverse_sub";
			case BLEND_MODE_LIGHTEN:
				return out << "lighten";
			case BLEND_MODE_DARKEN:
				return out << "darken";
			case BLEND_MODE_SCREEN:
				return out << "screen";
			case BLEND_MODE_OVERLAY:
				return out << "overlay";
			case BLEND_MODE_DODGE:
				return out << "dodge";
			case BLEND_MODE_BURN:
				return out << "burn";
			case BLEND_MODE_SOFT:
				return out << "soft";
			case BLEND_MODE_LINEAR:
				return out << "linear";
			case BLEND_MODE_HUE:
				return out << "hue";
			case BLEND_MODE_SATURATION:
				return out << "sat";
			case BLEND_MODE_VALUE:
				return out << "val";
			case BLEND_MODE_COLOR:
				return out << "col";
			case BLEND_MODE_MIX:
				return out << "mix";
			case BLEND_MODE_SRC:
				return out << "src";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, BlendMode &mode) {
		std::string val;
		in >> val;
		boost::to_lower(val);
		if (val == "src") mode = BLEND_MODE_SRC;
		else if (val == "src_alpha") mode = BLEND_MODE_SRC_ALPHA;
		else if (val == "dst_alpha") mode = BLEND_MODE_SRC_ALPHA;
		else if (val == "alpha") mode = BLEND_MODE_ALPHA;
		else if (val == "mul") mode = BLEND_MODE_MULTIPLY;
		else if (val == "div") mode = BLEND_MODE_DIVIDE;
		else if (val == "diff") mode = BLEND_MODE_DIFFERENCE;
		else if (val == "smooth_add") mode = BLEND_MODE_SMOOTH_ADD;
		else if (val == "average") mode = BLEND_MODE_SMOOTH_ADD;
		else if (val == "add") mode = BLEND_MODE_ADD;
		else if (val == "sub") mode = BLEND_MODE_SUBTRACT;
		else if (val == "reverse_sub") mode = BLEND_MODE_REVERSE_SUBTRACT;
		else if (val == "lighten") mode = BLEND_MODE_LIGHTEN;
		else if (val == "darken") mode = BLEND_MODE_DARKEN;
		else if (val == "screen") mode = BLEND_MODE_SCREEN;
		else if (val == "overlay") mode = BLEND_MODE_OVERLAY;
		else if (val == "dodge") mode = BLEND_MODE_DODGE;
		else if (val == "burn") mode = BLEND_MODE_BURN;
		else if (val == "soft") mode = BLEND_MODE_SOFT;
		else if (val == "linear") mode = BLEND_MODE_LINEAR;
		else if (val == "hue") mode = BLEND_MODE_HUE;
		else if (val == "sat") mode = BLEND_MODE_SATURATION;
		else if (val == "val") mode = BLEND_MODE_VALUE;
		else if (val == "col") mode = BLEND_MODE_COLOR;
		else if (val == "mix") mode = BLEND_MODE_MIX;
		else {
			REGEN_WARN("Unknown blend mode '" << val << "'. Using default SRC blending.");
			mode = BLEND_MODE_SRC;
		}
		return in;
	}

	BlendState::BlendState(BlendMode blendMode) : ServerSideState() {
		joinStates(ref_ptr<ToggleState>::alloc(RenderState::BLEND, GL_TRUE));

		switch (blendMode) {
			case BLEND_MODE_ALPHA:
			case BLEND_MODE_FRONT_TO_BACK:
				// c = c0*(1-c1.a) + c1*c1.a
				setBlendEquation(GL_FUNC_ADD);
				setBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case BLEND_MODE_DST_ALPHA:
			case BLEND_MODE_BACK_TO_FRONT:
				// c = c0*(1-c1.a) + c1*c1.a
				setBlendEquation(GL_FUNC_ADD);
				setBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);
				break;
			case BLEND_MODE_MULTIPLY:
				// c = c0*c1, a=a0*a1
				setBlendEquation(GL_FUNC_ADD);
				setBlendFunc(GL_DST_COLOR, GL_ZERO);
				break;
			case BLEND_MODE_ADD:
				// c = c0+c1, a=a0+a1
				setBlendEquation(GL_FUNC_ADD);
				setBlendFunc(GL_ONE, GL_ONE);
				break;
			case BLEND_MODE_SMOOTH_ADD: // aka average
				// c = 0.5*c0 + 0.5*c1
				// a = a0 + a1
				setBlendEquation(GL_FUNC_ADD);
				setBlendFuncSeparate(
						GL_CONSTANT_ALPHA, GL_CONSTANT_ALPHA,
						GL_ONE, GL_ONE);
				setBlendColor(Vec4f(0.5f));
				break;
			case BLEND_MODE_SUBTRACT:
				// c = c0-c1, a=a0-a1
				setBlendEquation(GL_FUNC_SUBTRACT);
				setBlendFunc(GL_ONE, GL_ONE);
				break;
			case BLEND_MODE_REVERSE_SUBTRACT:
				// c = c1-c0, a=c1-c0
				setBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
				setBlendFunc(GL_ONE, GL_ONE);
				break;
			case BLEND_MODE_LIGHTEN:
				// c = max(c0,c1), a = max(a0,a1)
				setBlendEquation(GL_MAX);
				setBlendFunc(GL_ONE, GL_ONE);
				break;
			case BLEND_MODE_DARKEN:
				// c = min(c0,c1), a = min(a0,a1)
				setBlendEquation(GL_MIN);
				setBlendFunc(GL_ONE, GL_ONE);
				break;
			case BLEND_MODE_SCREEN:
				// c = c0 - c1*(1-c0), a = a0 - a1*(1-a0)
				setBlendEquation(GL_FUNC_SUBTRACT);
				setBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE);
				break;
			case BLEND_MODE_SRC_ALPHA:
				// c = c1*c1.a
				setBlendEquation(GL_FUNC_ADD);
				setBlendFunc(GL_SRC_ALPHA, GL_ZERO);
				break;
			case BLEND_MODE_SRC:
				// c = c1
				setBlendEquation(GL_FUNC_ADD);
				setBlendFunc(GL_ONE, GL_ZERO);
				break;
			case BLEND_MODE_DIVIDE:
			case BLEND_MODE_DIFFERENCE:
			case BLEND_MODE_MIX:
			case BLEND_MODE_OVERLAY:
			case BLEND_MODE_HUE:
			case BLEND_MODE_SATURATION:
			case BLEND_MODE_VALUE:
			case BLEND_MODE_COLOR:
			case BLEND_MODE_DODGE:
			case BLEND_MODE_BURN:
			case BLEND_MODE_SOFT:
			case BLEND_MODE_LINEAR:
				break;
		}
	}

	BlendState::BlendState(GLenum sfactor, GLenum dfactor) : ServerSideState() {
		joinStates(ref_ptr<ToggleState>::alloc(RenderState::BLEND, GL_TRUE));
		setBlendFunc(sfactor, dfactor);
	}

	void BlendState::setBlendFunc(GLenum sfactor, GLenum dfactor) {
		if (blendFunc_.get()) {
			disjoinStates(blendFunc_);
		}
		blendFunc_ = ref_ptr<BlendFuncState>::alloc(sfactor, dfactor, sfactor, dfactor);
		joinStates(blendFunc_);
	}

	void BlendState::setBlendFuncSeparate(
			GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
		if (blendFunc_.get()) {
			disjoinStates(blendFunc_);
		}
		blendFunc_ = ref_ptr<BlendFuncState>::alloc(srcRGB, dstRGB, srcAlpha, dstAlpha);
		joinStates(blendFunc_);
	}

	void BlendState::setBlendEquation(GLenum equation) {
		if (blendEquation_.get()) {
			disjoinStates(blendEquation_);
		}
		blendEquation_ = ref_ptr<BlendEquationState>::alloc(equation);
		joinStates(blendEquation_);
	}

	void BlendState::setBlendColor(const Vec4f &col) {
		if (blendColor_.get()) {
			disjoinStates(blendColor_);
		}
		blendColor_ = ref_ptr<BlendColorState>::alloc(col);
		joinStates(blendColor_);
	}

	ref_ptr<BlendState> BlendState::load(LoadingContext &ctx, scene::SceneInputNode &input) {
		ref_ptr<BlendState> blend = ref_ptr<BlendState>::alloc(
				input.getValue<BlendMode>("mode", BLEND_MODE_SRC));
		if (input.hasAttribute("color")) {
			blend->setBlendColor(input.getValue<Vec4f>("color", Vec4f(0.0f)));
		}
		if (input.hasAttribute("equation")) {
			blend->setBlendEquation(glenum::blendFunction(
					input.getValue<std::string>("equation", "ADD")));
		}
		return blend;
	}
}
