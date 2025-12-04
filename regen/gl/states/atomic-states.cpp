#include "atomic-states.h"

using namespace regen;

ref_ptr<State> PolygonState::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	ref_ptr<State> offsetState, modeState;
	if (input.hasAttribute("offset-fill")) {
		auto offset = input.getValue<Vec2f>("offset-fill", Vec2f(1.1, 4.0));
		offsetState = ref_ptr<PolygonOffsetState>::alloc(offset.x, offset.y);
	}
	if (input.hasAttribute("mode")) {
		auto mode = input.getValue("mode");
		if (mode == "fill") {
			modeState = ref_ptr<FillModeState>::alloc(GL_FILL);
		} else if (mode == "line") {
			modeState = ref_ptr<FillModeState>::alloc(GL_LINE);
		} else if (mode == "point") {
			modeState = ref_ptr<FillModeState>::alloc(GL_POINT);
		}
	}
	if (modeState.get()) {
		if (offsetState.get()) {
			modeState->joinStates(offsetState);
		}
		return modeState;
	} else {
		return offsetState;
	}
}

ref_ptr<State> CullState::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	ref_ptr<State> orderState, modeState;
	if (input.hasAttribute("mode")) {
		modeState = ref_ptr<CullFaceState>::alloc(glenum::cullFace(input.getValue("mode")));
	}
	if (input.hasAttribute("winding-order")) {
		orderState = ref_ptr<FrontFaceState>::alloc(glenum::frontFace(input.getValue("winding-order")));
	}
	if (modeState.get()) {
		if (orderState.get()) {
			modeState->joinStates(orderState);
		}
		return modeState;
	} else {
		return orderState;
	}
}

ref_ptr<State> SampleShadingState::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	return ref_ptr<SampleShadingState>::alloc(input.getValue<float>("min-samples", 4.0f));
}

ref_ptr<State> ColorMaskState::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	return ref_ptr<ColorMaskState>::alloc(ColorMask(
			input.getValue<ColorMask>("mask", ColorMask::create(true))));
}

ref_ptr<State> BarrierState::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	if (input.hasAttribute("bit")) {
		auto barrierBit = glenum::barrierBit(input.getValue("bit"));
		return ref_ptr<BarrierState>::alloc(barrierBit);
	} else if (input.hasAttribute("bits")) {
		auto bits = input.getArray<std::string>("bits");
		GLbitfield barriers = 0;
		for (const auto &b : bits) {
			barriers |= glenum::barrierBit(b);
		}
		return ref_ptr<BarrierState>::alloc(barriers);
	}
	REGEN_WARN("Ignoring " << input.getDescription() << " without bit(s) attribute.");
	return {};
}
