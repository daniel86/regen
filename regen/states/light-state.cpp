/*
 * light-state.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

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
		}

		Light *light_;
	};
}

Light::Light(Light::Type lightType)
		: State(),
		  HasInput(ARRAY_BUFFER),
		  lightType_(lightType),
		  isAttenuated_(GL_TRUE),
		  coneMatrixStamp_(0) {
	switch (lightType_) {
		case DIRECTIONAL:
			set_isAttenuated(GL_FALSE);
			break;
		default:
			set_isAttenuated(GL_TRUE);
			break;
	}

	lightUniforms_ = ref_ptr<UBO>::alloc("Light");
	setInput(lightUniforms_);

	lightRadius_ = ref_ptr<ShaderInput2f>::alloc("lightRadius");
	lightRadius_->setUniformData(Vec2f(999999.9, 999999.9));
	lightUniforms_->addBlockInput(lightRadius_);

	lightConeAngles_ = ref_ptr<ShaderInput2f>::alloc("lightConeAngles");
	lightConeAngles_->setUniformData(Vec2f(0.0f));
	lightUniforms_->addBlockInput(lightConeAngles_);

	lightPosition_ = ref_ptr<ShaderInput4f>::alloc("lightPosition");
	lightPosition_->setUniformData(Vec4f(1.0f, 1.0f, 1.0f, 0.0f));
	lightPosition_->setSchema(InputSchema::position());
	lightUniforms_->addBlockInput(lightPosition_);

	lightDirection_ = ref_ptr<ShaderInput3f>::alloc("lightDirection");
	lightDirection_->setUniformData(Vec3f(1.0, 1.0, -1.0));
	lightDirection_->setSchema(InputSchema::direction());
	lightUniforms_->addBlockInput(lightDirection_);

	lightDiffuse_ = ref_ptr<ShaderInput3f>::alloc("lightDiffuse");
	lightDiffuse_->setUniformData(Vec3f(0.7f));
	lightDiffuse_->setSchema(InputSchema::color());
	lightUniforms_->addBlockInput(lightDiffuse_);

	lightSpecular_ = ref_ptr<ShaderInput3f>::alloc("lightSpecular");
	lightSpecular_->setUniformData(Vec3f(1.0f));
	lightSpecular_->setSchema(InputSchema::color());
	lightUniforms_->addBlockInput(lightSpecular_);

	set_innerConeAngle(50.0f);
	set_outerConeAngle(55.0f);

	coneMatrix_ = ref_ptr<ModelTransformation>::alloc();
	if (lightType_ == SPOT) {
		coneAnimation_ = ref_ptr<SpotConeAnimation>::alloc(this);
		coneAnimation_->setAnimationName("SpotCone");
		coneAnimation_->startAnimation();
	}
}

void Light::set_innerConeAngle(GLfloat deg) {
	auto data = lightConeAngles_->mapClientVertex<Vec2f>(
			ShaderData::READ | ShaderData::WRITE, 0);
	data.w = Vec2f(cos(2.0f * M_PIf * deg / 360.0f), data.r.y);
}

void Light::set_outerConeAngle(GLfloat deg) {
	auto data = lightConeAngles_->mapClientVertex<Vec2f>(
			ShaderData::READ | ShaderData::WRITE, 0);
	data.w = Vec2f(data.r.x, cos(2.0f * M_PIf * deg / 360.0f));
}

void Light::updateConeMatrix() {
	GLuint stamp = std::max(lightRadius_->stamp(), std::max(lightDirection_->stamp(),
															std::max(lightConeAngles_->stamp(),
																	 lightPosition_->stamp())));
	if (coneMatrixStamp_ != stamp) {
		coneMatrixStamp_ = stamp;
		updateConeMatrix_();
	}
}

void Light::updateConeMatrix_() {
	// Note: cone opens in positive z direction.
	auto numInstances = std::max(lightPosition_->numInstances(), lightDirection_->numInstances());
	if (coneMatrix_->modelMat()->numInstances() != numInstances) {
		// ensure cone matrix has numInstances
		coneMatrix_->modelMat()->setInstanceData(numInstances, 1, nullptr);
	}

	for (unsigned int i = 0; i < numInstances; ++i) {
		auto dir = lightDirection_->getVertexClamped(i).r;
		dir.normalize();
		auto angleCos = dir.dot(Vec3f(0.0, 0.0, 1.0));

		if (math::isApprox(abs(angleCos), 1.0)) {
			coneMatrix_->modelMat()->setVertex(i, Mat4f::identity());
		} else {
			const auto radius = lightRadius_->getVertexClamped(i).r.y;
			const auto coneAngle = lightConeAngles_->getVertexClamped(i).r.y;

			// Quaternion rotates view to light direction
			Quaternion q;
			auto axis = dir.cross(Vec3f(0.0, 0.0, 1.0));
			axis.normalize();
			q.setAxisAngle(axis, acos(angleCos));

			// scale `unit`-cone, rotate to light direction and finally translate to light position
			auto x = 2.0f * radius * tan(acos(coneAngle));
			auto val = q.calculateMatrix();
			val.scale(Vec3f(x, x, radius));
			val.translate(lightPosition_->getVertexClamped(i).r.xyz_());
			coneMatrix_->modelMat()->setVertex(i, val);
		}
	}
}

const ref_ptr<ShaderInputMat4> &Light::coneMatrix() {
	return coneMatrix_->modelMat();
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
	auto lightType = input.getValue<Light::Type>("type", Light::SPOT);
	ref_ptr<Light> light = ref_ptr<Light>::alloc(lightType);
	light->set_isAttenuated(
			input.getValue<bool>("is-attenuated", lightType != Light::DIRECTIONAL));

	auto dir = input.getValue<Vec3f>("direction", Vec3f(0.0f, 0.0f, 1.0f));
	dir.normalize();
	light->direction()->setVertex(0, dir);
	light->position()->setVertex3(0,
								 input.getValue<Vec3f>("position", Vec3f(0.0f)));
	light->direction()->setVertex(0,
								  input.getValue<Vec3f>("direction", Vec3f(0.0f, 0.0f, 1.0f)));
	light->diffuse()->setVertex(0,
								input.getValue<Vec3f>("diffuse", Vec3f(1.0f)));
	light->specular()->setVertex(0,
								 input.getValue<Vec3f>("specular", Vec3f(1.0f)));
	light->radius()->setVertex(0,
							   input.getValue<Vec2f>("radius", Vec2f(50.0f)));

	auto angles = input.getValue<Vec2f>("cone-angles", Vec2f(50.0f, 55.0f));
	light->set_innerConeAngle(angles.x);
	light->set_outerConeAngle(angles.y);
	ctx.scene()->putState(input.getName(), light);

	// process light node children
	for (auto &child: input.getChildren()) {
		if (child->getCategory() == "set") {
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
							ref_ptr<ModelTransformation>::alloc(light->position()));
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

void LightNode::update(GLdouble dt) {
	Vec3f v = animNode_->localTransform().transformVector(
			light_->position()->getVertex(0).r.xyz_());
	light_->position()->setVertex3(0, v);
}
