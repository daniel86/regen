#include "light-state.h"
#include "regen/animations/boids-cpu.h"
#include "regen/scene/shader-input-processor.h"

using namespace regen;

namespace regen {
	class SpotConeAnimation : public Animation {
	public:
		explicit SpotConeAnimation(Light *light)
				: Animation(false, true), light_(light) {}

		void animate(GLdouble dt) override {
			light_->updateConeMatrix();
			light_->updateShaderData();
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

	lightRadius_.resize(1, Vec2f(999999.9, 999999.9));
	sh_lightRadius_ = ref_ptr<ShaderInput2f>::alloc("lightRadius");
	sh_lightRadius_->setUniformData(lightRadius_[0]);
	lightBuffer_->addBlockInput(sh_lightRadius_);

	lightConeAngles_.resize(1, Vec2f(0.0f, 0.0f));
	set_innerConeAngle(50.0f);
	set_outerConeAngle(55.0f);
	sh_lightConeAngles_ = ref_ptr<ShaderInput2f>::alloc("lightConeAngles");
	sh_lightConeAngles_->setUniformData(lightConeAngles_[0]);
	lightBuffer_->addBlockInput(sh_lightConeAngles_);

	lightPosition_.resize(1, Vec4f(1.0f, 1.0f, 1.0f, 0.0f));
	sh_lightPosition_ = ref_ptr<ShaderInput4f>::alloc("lightPosition");
	sh_lightPosition_->setUniformData(lightPosition_[0]);
	sh_lightPosition_->setSchema(InputSchema::position());
	lightBuffer_->addBlockInput(sh_lightPosition_);

	lightDirection_.resize(1, Vec3f(1.0f, 1.0f, -1.0f));
	sh_lightDirection_ = ref_ptr<ShaderInput3f>::alloc("lightDirection");
	sh_lightDirection_->setUniformData(lightDirection_[0]);
	sh_lightDirection_->setSchema(InputSchema::direction());
	lightBuffer_->addBlockInput(sh_lightDirection_);

	lightDiffuse_.resize(1, Vec3f(0.7f));
	sh_lightDiffuse_ = ref_ptr<ShaderInput3f>::alloc("lightDiffuse");
	sh_lightDiffuse_->setUniformData(lightDiffuse_[0]);
	sh_lightDiffuse_->setSchema(InputSchema::color());
	lightBuffer_->addBlockInput(sh_lightDiffuse_);

	lightSpecular_.resize(1, Vec3f(1.0f));
	sh_lightSpecular_ = ref_ptr<ShaderInput3f>::alloc("lightSpecular");
	sh_lightSpecular_->setUniformData(lightSpecular_[0]);
	sh_lightSpecular_->setSchema(InputSchema::color());
	lightBuffer_->addBlockInput(sh_lightSpecular_);

	if (lightType_ == SPOT) {
		coneMatrix_.resize(1, Mat4f::identity());
		sh_coneMatrix_ = ref_ptr<ShaderInputMat4>::alloc("lightConeMatrix");
		sh_coneMatrix_->setUniformData(coneMatrix_[0]);
		lightBuffer_->addBlockInput(sh_coneMatrix_);

		coneAnimation_ = ref_ptr<SpotConeAnimation>::alloc(this);
		coneAnimation_->setAnimationName("SpotCone");
		coneAnimation_->startAnimation();
	}
}

void Light::updateShaderData() {
	if (lastPosStamp_ != lightPosStamp_) {
		lastPosStamp_ = lightPosStamp_;
		auto m_dir = sh_lightPosition_->mapClientDataRaw(BUFFER_GPU_WRITE);
		std::memcpy(m_dir.w, lightPosition_.data(), lightPosition_.size() * sizeof(Vec4f));
		m_dir.unmap();
	}
	if (lastDirStamp_ != lightDirStamp_) {
		lastDirStamp_ = lightDirStamp_;
		auto m_dir = sh_lightDirection_->mapClientDataRaw(BUFFER_GPU_WRITE);
		std::memcpy(m_dir.w, lightDirection_.data(), lightDirection_.size() * sizeof(Vec3f));
		m_dir.unmap();
	}
	if (lastDiffuseStamp_ != lightDiffuseStamp_) {
		lastDiffuseStamp_ = lightDiffuseStamp_;
		auto m_diff = sh_lightDiffuse_->mapClientDataRaw(BUFFER_GPU_WRITE);
		std::memcpy(m_diff.w, lightDiffuse_.data(), lightDiffuse_.size() * sizeof(Vec3f));
		m_diff.unmap();
	}
	if (lastSpecularStamp_ != lightSpecularStamp_) {
		lastSpecularStamp_ = lightSpecularStamp_;
		auto m_spec = sh_lightSpecular_->mapClientDataRaw(BUFFER_GPU_WRITE);
		std::memcpy(m_spec.w, lightSpecular_.data(), lightSpecular_.size() * sizeof(Vec3f));
		m_spec.unmap();
	}
	if (lastConeAnglesStamp_ != lightConeAnglesStamp_) {
		lastConeAnglesStamp_ = lightConeAnglesStamp_;
		auto m_cone = sh_lightConeAngles_->mapClientDataRaw(BUFFER_GPU_WRITE);
		std::memcpy(m_cone.w, lightConeAngles_.data(), lightConeAngles_.size() * sizeof(Vec2f));
		m_cone.unmap();
	}
	if (lastRadiusStamp_ != lightRadiusStamp_) {
		lastRadiusStamp_ = lightRadiusStamp_;
		auto m_radius = sh_lightRadius_->mapClientDataRaw(BUFFER_GPU_WRITE);
		std::memcpy(m_radius.w, lightRadius_.data(), lightRadius_.size() * sizeof(Vec2f));
		m_radius.unmap();
	}
	if (lightType_ == SPOT && lastConeStamp_ != lightConeStamp_) {
		lastConeStamp_ = lightConeStamp_;
		auto m_cone = sh_coneMatrix_->mapClientDataRaw(BUFFER_GPU_WRITE);
		std::memcpy(m_cone.w, coneMatrix_.data(), coneMatrix_.size() * sizeof(Mat4f));
		m_cone.unmap();
	}
}

void Light::set_innerConeAngle(float deg) {
	lightConeAngles_[0].x = cos(2.0f * M_PIf * deg / 360.0f);
	lightConeAnglesStamp_ += 1;
}

void Light::set_outerConeAngle(float deg) {
	lightConeAngles_[0].y = cos(2.0f * M_PIf * deg / 360.0f);
	lightConeAnglesStamp_ += 1;
}

void Light::updateConeMatrix() {
	uint32_t stamp = std::max(lightRadiusStamp_, std::max(lightDirStamp_,
			std::max(lightConeAnglesStamp_, lightPosStamp_)));
	if (lightConeStamp_ == stamp) return; // no update needed
	lightConeStamp_ = stamp;

	// Note: cone opens in positive z direction.
	// FIXME: where are num instances set for light? probably best to hook resize there!
	//         here is too late!
	auto numInstances = std::max(lightPosition_.size(), lightDirection_.size());
	if (coneMatrix_.size() != numInstances) {
		// ensure cone matrix has numInstances
		coneMatrix_.resize(numInstances, Mat4f::identity());
		sh_coneMatrix_->setInstanceData(numInstances, 1, (byte *) coneMatrix_.data());
	}

	for (unsigned int i = 0; i < numInstances; ++i) {
		auto dir = getClamped(lightDirection_, i);
		dir.normalize();
		auto angleCos = dir.dot(Vec3f(0.0, 0.0, 1.0));

		if (math::isApprox(abs(angleCos), 1.0)) {
			coneMatrix_[i] = Mat4f::identity();
		} else {
			auto radius = getClamped(lightRadius_,i).y;
			auto coneAngle = getClamped(lightConeAngles_,i).y;

			// Quaternion rotates view to light direction
			Quaternion q;
			auto axis = dir.cross(Vec3f(0.0, 0.0, 1.0));
			axis.normalize();
			q.setAxisAngle(axis, acos(angleCos));

			// scale `unit`-cone, rotate to light direction and finally translate to light position
			auto x = 2.0f * radius * tan(acos(coneAngle));
			auto val = q.calculateMatrix();
			val.scale(Vec3f(x, x, radius));
			val.translate(getClamped(lightPosition_,i).xyz_());
			coneMatrix_[i] = val;
		}
	}
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
	light->set_innerConeAngle(angles.x);
	light->set_outerConeAngle(angles.y);
	ctx.scene()->putState(input.getName(), light);

	// process light node children
	for (auto &child: input.getChildren()) {
		if (child->getCategory() == "set") {
			// FIXME: this won't work as expected anymore!!
			//      - same in shader input widget!
			// set a given light input. The input key is given by the "target" attribute.
			auto targetName = child->getValue("target");
			// find the shader input in the light state
			auto target_opt = light->findShaderInput(targetName);
			if (!target_opt) {
				REGEN_WARN("Cannot find light input for set in node " << child->getDescription());
				continue;
			}
			auto setTarget = target_opt.value().in;
			auto numInstances = std::max(
					child->getValue<GLuint>("num-instances", 1u),
					setTarget->numInstances());
			// allocate memory for the shader input
			setTarget->setInstanceData(numInstances, 1, nullptr);
			scene::ShaderInputProcessor::setInput(*child.get(), setTarget.get(), numInstances);
		}
		if (child->getCategory() == "animation") {
			auto animationType = child->getValue("type");
			if (animationType == "boids") {
				// let a boid simulation change the light positions
				LoadingContext boidsConfig(ctx.scene(), ctx.parent());
				auto boidsAnimation = BoidsCPU::load(
							boidsConfig,
							*child.get(),
							ref_ptr<ModelTransformation>::alloc(light->sh_position()));
				light->attach(boidsAnimation);
				boidsAnimation->startAnimation();
			} else {
				REGEN_WARN("Unknown animation type '" << animationType << "' in node " << child->getDescription());
			}
		}
	}

	return light;
}

//////////
//////////
//////////

LightNode::LightNode(
		const ref_ptr<Light> &light,
		const ref_ptr<AnimationNode> &n)
		: State(), light_(light), animNode_(n) {}

void LightNode::update(GLdouble /*dt*/) {
	Vec3f v = animNode_->localTransform().transformVector(light_->position(0).xyz_());
	light_->setPosition(0, v);
	light_->updateShaderData();
}
