#include "deformation.h"
#include "regen/animations/mesh-animation.h"

using namespace regen::scene;
using namespace regen;

class AnimationRepeater : public EventHandler {
public:
	AnimationRepeater(const ref_ptr<MeshAnimation> &anim, const Vec2d &tickRange)
			: EventHandler(), anim_(anim), tickRange_(tickRange) {}

	void call(EventObject *ev, EventData *data) override {
		anim_->setTickRange(tickRange_);
		anim_->startAnimation();
	}

protected:
	ref_ptr<MeshAnimation> anim_;
	Vec2d tickRange_;
};

void DeformationNodeProvider::processInput(
		SceneParser *parser,
		SceneInputNode &input,
		const ref_ptr<State> &parent) {
	auto meshID = input.getValue("mesh-id");
	auto repeat = input.getValue<std::string>("repeat", "true") == "true";
	auto friction = input.getValue<float>("friction", 8.0f);
	auto frequency = input.getValue<float>("frequency", 5.0f);
	auto meshVector = parser->getResources()->getMesh(parser, meshID);

	// configure per-attribute interpolation functions, default is linear
	auto interpolationsCfg = input.getFirstChild("interpolations");
	std::list<MeshAnimation::Interpolation> interpolations;
	if (interpolationsCfg.get() != nullptr) {
		for (auto it = interpolationsCfg->getChildren().begin();
			 it != interpolationsCfg->getChildren().end(); ++it) {
			auto &interpolationCfg = *it;
			auto attributeName = interpolationCfg->getValue("target");
			auto functionName = interpolationCfg->getValue("function");
			interpolations.emplace_back(attributeName, "interpolate_" + functionName);
		}
	}

	for (const auto &mesh: *meshVector.get()) {
		// create the animation
		auto animation = ref_ptr<MeshAnimation>::alloc(mesh, interpolations);
		animation->setFriction(friction);
		animation->setFrequency(frequency);

		// configure key frames
		double animationTime = 0.0;
		auto framesCfg = input.getFirstChild("frames");
		for (const auto &frameCfg: framesCfg->getChildren()) {
			auto timeInTicks = frameCfg->getValue<double>("duration", 0.0);
			auto shape = frameCfg->getValue("shape");
			if (shape.empty()) {
				REGEN_WARN("Skipping deformation frame without \"shape\".");
				continue;
			}
			if (shape == "original") {
				animation->addMeshFrame(timeInTicks);
			} else if (shape == "sphere") {
				auto h_radius = frameCfg->getValue<float>("radius", 0.5f);
				auto v_radius = frameCfg->getValue<float>("radius", 0.5f);
				auto offset = frameCfg->getValue<Vec3f>("offset", Vec3f(0.0f, 0.0f, 0.0f));
				animation->addSphereAttributes(h_radius, v_radius, timeInTicks, offset);
			} else if (shape == "box") {
				auto width = frameCfg->getValue<float>("width", 1.0f);
				auto height = frameCfg->getValue<float>("height", 1.0f);
				auto depth = frameCfg->getValue<float>("depth", 1.0f);
				auto offset = frameCfg->getValue<Vec3f>("offset", Vec3f(0.0f, 0.0f, 0.0f));
				animation->addBoxAttributes(width, height, depth, timeInTicks, offset);
			} else {
				REGEN_WARN("Ignoring frame with unknown shape '" << shape << "'.");
				continue;
			}
			animationTime += timeInTicks;
		}
		Vec2d tickRange = {0.0, animationTime};

		mesh->addAnimation(animation);
		animation->setTickRange(tickRange);
		animation->startAnimation();

		if (repeat) {
			auto animRepeater = ref_ptr<AnimationRepeater>::alloc(animation, tickRange);
			animation->connect(Animation::ANIMATION_STOPPED, animRepeater);
		}
	}
}
