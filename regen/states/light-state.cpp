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
			if(light_->updateConeMatrix()) {
				light_->updateShaderData();
			}
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
		updateConeMatrix();

		sh_coneMatrix_ = ref_ptr<ShaderInputMat4>::alloc("lightConeMatrix");
		sh_coneMatrix_->setUniformData(coneMatrix_[0]);
		lightBuffer_->addBlockInput(sh_coneMatrix_);

		coneAnimation_ = ref_ptr<SpotConeAnimation>::alloc(this);
		coneAnimation_->setAnimationName("SpotCone");
		coneAnimation_->startAnimation();
	}
}

template<typename T>
static inline void resizeLocalData_(const ref_ptr<ShaderInput> &sh, std::vector<T> &local) {
	if(sh->numInstances() > local.size()) {
		local.resize(sh->numInstances());
		auto mapped = sh->mapClientDataRaw(BUFFER_GPU_READ);
		std::memcpy(
			(byte *) local.data(),
			(byte *) mapped.r, local.size() * sizeof(T));
	}
}

void Light::resizeLocalData() {
	resizeLocalData_(sh_lightRadius_, lightRadius_);
	resizeLocalData_(sh_lightConeAngles_, lightConeAngles_);
	resizeLocalData_(sh_lightPosition_, lightPosition_);
	resizeLocalData_(sh_lightDirection_, lightDirection_);
	resizeLocalData_(sh_lightDiffuse_, lightDiffuse_);
	resizeLocalData_(sh_lightSpecular_, lightSpecular_);
	if (lightType_ == SPOT) {
		resizeLocalData_(sh_coneMatrix_, coneMatrix_);
	}
}

template<typename T>
static inline void updateShaderData_(
		uint32_t stamp,
		uint32_t &lastStamp,
		const ref_ptr<ShaderInput> &sh,
		const std::vector<T> &data) {
	if (lastStamp != stamp) {
		lastStamp = stamp;
		auto m_data = sh->mapClientDataRaw(BUFFER_GPU_WRITE);
		std::memcpy(m_data.w, data.data(), data.size() * sizeof(T));
		m_data.unmap();
	}
}

void Light::updateShaderData() {
	updateShaderData_(
		lightPosStamp_, lastPosStamp_,
		sh_lightPosition_, lightPosition_);
	updateShaderData_(
		lightDirStamp_, lastDirStamp_,
		sh_lightDirection_, lightDirection_);
	updateShaderData_(
		lightDiffuseStamp_, lastDiffuseStamp_,
		sh_lightDiffuse_, lightDiffuse_);
	updateShaderData_(
		lightSpecularStamp_, lastSpecularStamp_,
		sh_lightSpecular_, lightSpecular_);
	updateShaderData_(
		lightConeAnglesStamp_, lastConeAnglesStamp_,
		sh_lightConeAngles_, lightConeAngles_);
	updateShaderData_(
		lightRadiusStamp_, lastRadiusStamp_,
		sh_lightRadius_, lightRadius_);
	if (lightType_ == SPOT) {
		updateShaderData_(
			lightConeStamp_, lastConeStamp_,
			sh_coneMatrix_, coneMatrix_);
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

bool Light::updateConeMatrix() {
	uint32_t stamp = std::max(lightRadiusStamp_, std::max(lightDirStamp_,
			std::max(lightConeAnglesStamp_, lightPosStamp_)));
	if (lightConeStamp_ == stamp) return false; // no update needed

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
	bool isBufferResized = false;
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
			isBufferResized = true;
			scene::ShaderInputProcessor::setInput(*child.get(), setTarget.get(), numInstances);
		}
		if (child->getCategory() == "animation") {
			auto animationType = child->getValue("type");
			if (animationType == "boids") {
				// let a boid simulation change the light positions
				LoadingContext boidsConfig(ctx.scene(), ctx.parent());
				// TODO: this will update position, without updating the light!
				//   Which is fine in most cases, but eg. in case of spot light,
				//   the cone matrix may need to be updated.
				// TODO: also attach orientation for spot cameras.
				auto boids = ref_ptr<BoidsCPU>::alloc(light->sh_position());
				boids->loadSettings(ctx, input);

				light->attach(boids);
				boids->startAnimation();
			} else {
				REGEN_WARN("Unknown animation type '" << animationType << "' in node " << child->getDescription());
			}
		}
	}
	if (isBufferResized) {
		light->resizeLocalData();
	}
	if (lightType == Light::SPOT) {
		light->updateConeMatrix();
	}
	light->updateShaderData();

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
