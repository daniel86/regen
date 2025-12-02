#include "light-state.h"
#include "regen/simulation/boids-cpu.h"
#include "regen/scene/shader-input-processor.h"

using namespace regen;

namespace regen {
	class SpotConeAnimation : public Animation {
	public:
		explicit SpotConeAnimation(Light *light)
				: Animation(false, true), light_(light) {}

		void cpuUpdate(double dt) override {
			light_->updateConeMatrix();
		}

		Light *light_;
	};
}

Light::Light(Light::Type lightType, const BufferUpdateFlags &updateFlags)
		: State(),
		  lightType_(lightType),
		  isAttenuated_(true) {
	set_isAttenuated(lightType_ != DIRECTIONAL);

	lightBuffer_ = ref_ptr<UBO>::alloc("Light", updateFlags);
	setInput(lightBuffer_);

	lightRadius_ = ref_ptr<ShaderInput2f>::alloc("lightRadius");
	lightRadius_->setUniformData(Vec2f(999999.9, 999999.9));
	lightBuffer_->addStagedInput(lightRadius_);

	lightConeAngles_ = ref_ptr<ShaderInput2f>::alloc("lightConeAngles");
	lightConeAngles_->setUniformData(Vec2f(
			cos(2.0f * M_PIf * 50.0f / 360.0f),
			cos(2.0f * M_PIf * 55.0f / 360.0f)));
	lightBuffer_->addStagedInput(lightConeAngles_);

	lightPosition_ = ref_ptr<ShaderInput4f>::alloc("lightPosition");
	lightPosition_->setUniformData(Vec4f(1.0f, 1.0f, 1.0f, 0.0f));
	lightPosition_->setSchema(InputSchema::position());
	lightBuffer_->addStagedInput(lightPosition_);

	lightDirection_ = ref_ptr<ShaderInput3f>::alloc("lightDirection");
	lightDirection_->setUniformData(Vec3f(1.0f, 1.0f, -1.0f));
	lightDirection_->setSchema(InputSchema::direction());
	lightBuffer_->addStagedInput(lightDirection_);

	lightDiffuse_ = ref_ptr<ShaderInput3f>::alloc("lightDiffuse");
	lightDiffuse_->setUniformData(Vec3f::create(0.7f));
	lightDiffuse_->setSchema(InputSchema::color());
	lightBuffer_->addStagedInput(lightDiffuse_);

	lightSpecular_ = ref_ptr<ShaderInput3f>::alloc("lightSpecular");
	lightSpecular_->setUniformData(Vec3f::one());
	lightSpecular_->setSchema(InputSchema::color());
	lightBuffer_->addStagedInput(lightSpecular_);

	if (lightType_ == SPOT) {
		coneMatrix_ = ref_ptr<ShaderInputMat4>::alloc("lightConeMatrix");
		coneMatrix_->setUniformData(Mat4f::identity());
		lightBuffer_->addStagedInput(coneMatrix_);

		coneAnimation_ = ref_ptr<SpotConeAnimation>::alloc(this);
		coneAnimation_->setAnimationName("SpotCone");
		coneAnimation_->startAnimation();
	}
}

void Light::setConeAngles(float inner, float outer) {
	lightConeAngles_->setVertex(0, Vec2f(
			cos(2.0f * M_PIf * inner / 360.0f),
			cos(2.0f * M_PIf * outer / 360.0f)));
}

bool Light::updateConeMatrix() {
	uint32_t stamp = std::max(lightRadius_->stampOfReadData(),
			std::max(lightDirection_->stampOfReadData(),
			std::max(lightConeAngles_->stampOfReadData(), lightPosition_->stampOfReadData())));
	if (lightConeStamp_ == stamp) return false; // no update needed

	// Note: cone opens in positive z direction.
	auto numInstances = static_cast<uint32_t>(this->numInstances());
	if (coneMatrix_->numInstances() != numInstances) {
		// ensure cone matrix has numInstances
		coneMatrix_->setInstanceData(numInstances, 1, nullptr);
	}
	auto m_coneMatrix = coneMatrix_->mapClientData<Mat4f>(BUFFER_GPU_WRITE);

	for (unsigned int i = 0; i < numInstances; ++i) {
		auto dir = lightDirection_->getVertexClamped(i).r;
		dir.normalize();
		auto angleCos = dir.dot(Vec3f(0.0, 0.0, 1.0));

		if (math::isApprox(abs(angleCos), 1.0)) {
			m_coneMatrix.w[i] = Mat4f::identity();
		} else {
			auto radius = lightRadius_->getVertexClamped(i).r.y;
			auto coneAngle = lightConeAngles_->getVertexClamped(i).r.y;

			// Quaternion rotates view to light direction
			Quaternion q;
			auto axis = dir.cross(Vec3f(0.0, 0.0, 1.0));
			axis.normalize();
			q.setAxisAngle(axis, acos(angleCos));

			// scale `unit`-cone, rotate to light direction and finally translate to light position
			auto x = 2.0f * radius * tan(acos(coneAngle));
			auto val = q.calculateMatrix();
			val.scale(Vec3f(x, x, radius));
			val.translate(lightPosition_->getVertexClamped(i).r.xyz());
			m_coneMatrix.w[i] = val;
		}
	}

	lightConeStamp_ = stamp;
	return true;
}

namespace regen {
	std::ostream &operator<<(std::ostream &out, const Light::Type &type) {
		switch (type) {
			case Light::DIRECTIONAL:
				return out << "DIRECTIONAL";
			case Light::SPOT:
				return out << "SPOT";
			case Light::POINT:
				return out << "POINT";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, Light::Type &type) {
		std::string val;
		in >> val;
		boost::to_upper(val);
		if (val == "DIRECTIONAL") type = Light::DIRECTIONAL;
		else if (val == "SPOT") type = Light::SPOT;
		else if (val == "POINT") type = Light::POINT;
		else {
			REGEN_WARN("Unknown light type '" << val << "'. Using SPOT light.");
			type = Light::SPOT;
		}
		return in;
	}

	std::ostream &operator<<(std::ostream &out, const ShadowFilterMode &mode) {
		switch (mode) {
			case SHADOW_FILTERING_NONE:
				return out << "NONE";
			case SHADOW_FILTERING_PCF_GAUSSIAN:
				return out << "PCF_GAUSSIAN";
			case SHADOW_FILTERING_VSM:
				return out << "VSM";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, ShadowFilterMode &mode) {
		std::string val;
		in >> val;
		boost::to_upper(val);
		if (val == "NONE") mode = SHADOW_FILTERING_NONE;
		else if (val == "PCF_GAUSSIAN") mode = SHADOW_FILTERING_PCF_GAUSSIAN;
		else if (val == "VSM") mode = SHADOW_FILTERING_VSM;
		else {
			REGEN_WARN("Unknown shadow filtering mode '" << val << "'. Using no filtering.");
			mode = SHADOW_FILTERING_NONE;
		}
		return in;
	}
}

ref_ptr<Light> Light::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	BufferUpdateFlags updateFlags;
	updateFlags.frequency = input.getValue<BufferUpdateFrequency>(
		"update-frequency", BUFFER_UPDATE_PER_FRAME);
	updateFlags.scope = input.getValue<BufferUpdateScope>(
		"update-scope", BUFFER_UPDATE_FULLY);

	uint32_t numInstances = input.getValue<uint32_t>("num-instances", 1u);

	auto lightType = input.getValue<Light::Type>("type", Light::SPOT);
	ref_ptr<Light> light = ref_ptr<Light>::alloc(lightType, updateFlags);
	light->set_isAttenuated(
			input.getValue<bool>("is-attenuated", lightType != Light::DIRECTIONAL));

	auto dir = input.getValue<Vec3f>("direction", Vec3f(0.0f, 0.0f, 1.0f));
	dir.normalize();
	light->setDirection(0, dir);
	light->setPosition(0, input.getValue<Vec3f>("position", Vec3f::zero()));
	light->setDiffuse(0, input.getValue<Vec3f>("diffuse", Vec3f::one()));
	light->setSpecular(0, input.getValue<Vec3f>("specular", Vec3f::one()));
	light->setRadius(0, input.getValue<Vec2f>("radius", Vec2f(50.0f, 50.0f)));

	auto angles = input.getValue<Vec2f>("cone-angles", Vec2f(50.0f, 55.0f));
	light->setConeAngles(angles.x, angles.y);
	ctx.scene()->putState(input.getName(), light);

	std::vector<ref_ptr<scene::SceneInputNode>> visited;
	// process light node children
	for (auto &child: input.getChildren()) {
		if (child->getCategory() == "set") {
			visited.push_back(child);
			// set a given light input. The input key is given by the "target" attribute.
			auto targetName = child->getValue("target");
			// find the shader input in the light state
			auto target_opt = light->findShaderInput(targetName);
			if (!target_opt) {
				REGEN_WARN("Cannot find light input for set in node " << child->getDescription());
				continue;
			}
			auto setTarget = target_opt.value().in;
			auto numInstances0 = std::max(
					child->getValue<uint32_t>("num-instances", 1u),
					setTarget->numInstances());
			numInstances = std::max(numInstances, numInstances0);
			// allocate memory for the shader input
			setTarget->setInstanceData(numInstances0, 1, nullptr);
			scene::ShaderInputProcessor::setInput(*child.get(), setTarget.get(), numInstances0);
		}
		if (child->getCategory() == "animation") {
			auto animationType = child->getValue("type");
			visited.push_back(child);
			if (animationType == "boids") {
				// let a boid simulation change the light positions
				LoadingContext boidsConfig(ctx.scene(), ctx.parent());
				// TODO: this will update position, without updating the light!
				//   Which is fine in most cases, but eg. in case of spot light,
				//   the cone matrix may need to be updated.
				//   - also attach orientation for spot cameras.
				auto boids = ref_ptr<BoidsCPU>::alloc(light->position());
				boids->loadSettings(ctx, *child.get());

				light->attach(boids);
				boids->startAnimation();
			} else {
				REGEN_WARN("Unknown animation type '" << animationType << "' in node " << child->getDescription());
			}
		}
	}
	light->set_numInstances(numInstances);

	// remove visited children
	for (auto &child : visited) {
		input.removeChild(child);
	}
	if (lightType == Light::SPOT) {
		light->updateConeMatrix();
	}

	auto parser = ctx.scene();
	for (auto &child: input.getChildren()) {
		auto processor = parser->getStateProcessor(child->getCategory());
		if (processor.get() == nullptr) {
			REGEN_WARN("No processor registered for '" << child->getDescription() << "'.");
			continue;
		}
		processor->processInput(parser, *child.get(), ctx.parent(), light);
	}

	return light;
}
