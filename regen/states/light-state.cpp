/*
 * light-state.cpp
 *
 *  Created on: 26.01.2011
 *      Author: daniel
 */

#include "light-state.h"

using namespace regen;

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

Light::Light(Light::Type lightType)
		: State(),
		  Animation(GL_TRUE, GL_FALSE, lightType == SPOT),
		  HasInput(VBO::USAGE_DYNAMIC),
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

	lightPosition_ = ref_ptr<ShaderInput3f>::alloc("lightPosition");
	lightPosition_->setUniformData(Vec3f(1.0, 1.0, 1.0));
	setInput(lightPosition_);

	lightDirection_ = ref_ptr<ShaderInput3f>::alloc("lightDirection");
	lightDirection_->setUniformData(Vec3f(1.0, 1.0, -1.0));
	setInput(lightDirection_);

	lightDiffuse_ = ref_ptr<ShaderInput3f>::alloc("lightDiffuse");
	lightDiffuse_->setUniformData(Vec3f(0.7f));
	setInput(lightDiffuse_);

	lightSpecular_ = ref_ptr<ShaderInput3f>::alloc("lightSpecular");
	lightSpecular_->setUniformData(Vec3f(1.0f));
	setInput(lightSpecular_);

	lightRadius_ = ref_ptr<ShaderInput2f>::alloc("lightRadius");
	lightRadius_->setUniformData(Vec2f(999999.9, 999999.9));
	setInput(lightRadius_);

	coneMatrix_ = ref_ptr<ModelTransformation>::alloc();
	lightConeAngles_ = ref_ptr<ShaderInput2f>::alloc("lightConeAngles");
	lightConeAngles_->setUniformData(Vec2f(0.0f));
	setInput(lightConeAngles_);
	set_innerConeAngle(50.0f);
	set_outerConeAngle(55.0f);
}

Light::Type Light::lightType() const { return lightType_; }

GLboolean Light::isAttenuated() const { return isAttenuated_; }

void Light::set_isAttenuated(GLboolean isAttenuated) { isAttenuated_ = isAttenuated; }

void Light::set_innerConeAngle(GLfloat deg) {
	Vec2f a = lightConeAngles_->getVertex(0);
	lightConeAngles_->setVertex(0,
								Vec2f(cos(2.0f * M_PI * deg / 360.0f), a.y));
}

void Light::set_outerConeAngle(GLfloat deg) {
	Vec2f a = lightConeAngles_->getVertex(0);
	lightConeAngles_->setVertex(0,
								Vec2f(a.x, cos(2.0f * M_PI * deg / 360.0f)));
}

void Light::updateConeMatrix() {
	const Vec3f &pos = lightPosition_->getVertex(0);
	// Note: cone opens in positive z direction.
	Vec3f dir = lightDirection_->getVertex(0);
	dir.normalize();
	GLfloat angleCos = dir.dot(Vec3f(0.0, 0.0, 1.0));

	if (math::isApprox(abs(angleCos), 1.0)) {
		coneMatrix_->get()->setVertex(0, Mat4f::identity());
	} else {
		const GLfloat &radius = lightRadius_->getVertex(0).y;
		const GLfloat &coneAngle = lightConeAngles_->getVertex(0).y;

		// Quaternion rotates view to light direction
		Quaternion q;
		Vec3f axis = dir.cross(Vec3f(0.0, 0.0, 1.0));
		axis.normalize();
		q.setAxisAngle(axis, acos(angleCos));

		// scale `unit`-cone, rotate to light direction and finally translate to light position
		GLfloat x = 2.0f * radius * tan(acos(coneAngle));

		Mat4f val = q.calculateMatrix();
		val.scale(Vec3f(x, x, radius));
		val.translate(pos);
		coneMatrix_->get()->setVertex(0, val);
	}
}

void Light::glAnimate(RenderState *rs, GLdouble dt) {
	GLuint stamp = std::max(lightRadius_->stamp(), std::max(lightDirection_->stamp(),
															std::max(lightConeAngles_->stamp(),
																	 lightPosition_->stamp())));
	if (coneMatrixStamp_ != stamp) {
		coneMatrixStamp_ = stamp;
		updateConeMatrix();
	}
}

const ref_ptr<ShaderInput2f> &Light::radius() const { return lightRadius_; }

const ref_ptr<ShaderInput2f> &Light::coneAngle() const { return lightConeAngles_; }

const ref_ptr<ShaderInput3f> &Light::position() const { return lightPosition_; }

const ref_ptr<ShaderInput3f> &Light::direction() const { return lightDirection_; }

const ref_ptr<ShaderInput3f> &Light::diffuse() const { return lightDiffuse_; }

const ref_ptr<ShaderInput3f> &Light::specular() const { return lightSpecular_; }

const ref_ptr<ShaderInputMat4> &Light::coneMatrix() { return coneMatrix_->get(); }

//////////
//////////
//////////

LightNode::LightNode(
		const ref_ptr<Light> &light,
		const ref_ptr<AnimationNode> &n)
		: State(), light_(light), animNode_(n) {}

void LightNode::update(GLdouble dt) {
	Vec3f v = animNode_->localTransform().transformVector(
			light_->position()->getVertex(0));
	light_->position()->setVertex(0, v);
}
