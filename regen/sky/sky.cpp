/*
 * sky.cpp
 *
 *  Created on: Oct 3, 2014
 *      Author: daniel
 */

#include "sky.h"

#include <ctime>

#include <regen/external/osghimmel/astronomy.h>
#include <regen/external/osghimmel/earth.h>
#include <regen/external/osghimmel/moon.h>
#include <regen/states/blend-state.h>
#include <regen/states/depth-state.h>
#include <regen/states/state-configurer.h>

using namespace regen;

Sky::Sky(const ref_ptr<Camera> &cam, const ref_ptr<ShaderInput2i> &viewport)
		: StateNode(),
		  Animation(GL_TRUE, GL_TRUE),
		  cam_(cam),
		  viewport_(viewport) {
	srand(time(nullptr));

	ref_ptr<DepthState> depth = ref_ptr<DepthState>::alloc();
	depth->set_depthFunc(GL_LEQUAL);
	depth->set_depthRange(1.0, 1.0);
	depth->set_useDepthWrite(GL_FALSE);
	depth->set_useDepthTest(GL_TRUE);
	state()->joinStates(depth);

	//state()->joinStates(ref_ptr<ToggleState>::alloc(RenderState::CULL_FACE, GL_FALSE));
	state()->joinStates(ref_ptr<BlendState>::alloc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

	noonColor_ = Vec3f(0.5, 0.5, 0.5);
	dawnColor_ = Vec3f(0.2, 0.15, 0.15);
	moonSunLightReflectance_ = 0.4;

	timef_ = ref_ptr<osgHimmel::TimeF>::alloc(
			time(nullptr),       // Current time
			-3600.0L * 2.0L,  // UTC offset
			3600.0L           // secondsPerCycle
	);
	timef_->start();

	astro_ = ref_ptr<osgHimmel::Astronomy>::alloc();
	astro_->setLatitude(52.5491);
	astro_->setLongitude(13.3611);

	timeUniform_ = ref_ptr<ShaderInput1f>::alloc("time");
	timeUniform_->setUniformData(0.0f);
	state()->joinShaderInput(timeUniform_);

	// 0: altitude in km
	// 1: apparent angular radius (not diameter!)
	// 2: radius up to "end of atm"
	// 3: seed (for randomness of stuff)
	cmnUniform_ = ref_ptr<ShaderInput4f>::alloc("cmn");
	cmnUniform_->setUniformData(Vec4f(
			0.043,
			osgHimmel::Earth::meanRadius(),
			osgHimmel::Earth::meanRadius() + osgHimmel::Earth::atmosphereThicknessNonUniform(),
			0));
	state()->joinShaderInput(cmnUniform_);

	R_ = ref_ptr<ShaderInputMat4>::alloc("equToHorMatrix");
	R_->setUniformData(Mat4f::identity());
	state()->joinShaderInput(R_);

	q_ = ref_ptr<ShaderInput1f>::alloc("q");
	q_->setUniformData(0.0f);
	state()->joinShaderInput(q_);

	// directional light that approximates the sun
	sun_ = ref_ptr<Light>::alloc(Light::DIRECTIONAL);
	sun_->set_isAttenuated(GL_FALSE);
	sun_->specular()->setVertex(0, Vec3f(0.0f));
	sun_->diffuse()->setVertex(0, Vec3f(0.0f));
	sun_->direction()->setVertex(0, Vec3f(1.0f));
	state()->joinShaderInput(sun_->direction(), "sunPosition");
	// directional light that approximates the moon
	moon_ = ref_ptr<Light>::alloc(Light::DIRECTIONAL);
	moon_->set_isAttenuated(GL_FALSE);
	moon_->specular()->setVertex(0, Vec3f(0.0f));
	moon_->diffuse()->setVertex(0, Vec3f(0.0f));
	moon_->direction()->setVertex(0, Vec3f(1.0f));
	state()->joinShaderInput(moon_->direction(), "moonPosition");

	Rectangle::Config cfg;
	cfg.centerAtOrigin = GL_FALSE;
	cfg.isNormalRequired = GL_FALSE;
	cfg.isTangentRequired = GL_FALSE;
	cfg.isTexcoRequired = GL_FALSE;
	cfg.levelOfDetails = {4}; // TODO: why so much?
	cfg.posScale = Vec3f(2.0f);
	cfg.rotation = Vec3f(0.5 * M_PI, 0.0f, 0.0f);
	cfg.texcoScale = Vec2f(1.0);
	cfg.translation = Vec3f(-1.0f, -1.0f, 0.0f);
	cfg.usage = VBO::USAGE_STATIC;
	skyQuad_ = ref_ptr<Rectangle>::alloc(cfg);

	// mae some parts of the sky configurable from the GUI.
	// TODO: need to add the whole node here I guess.
	setAnimationName("sky");
	joinAnimationState(state());
}

const ref_ptr<Rectangle> &Sky::skyQuad() const {
	return skyQuad_;
}

std::string Sky::date() const {
	const time_t t = timef_->gett();
	std::string dateStr(ctime(&t));
	return dateStr.substr(0, dateStr.size() - 1);
}

GLdouble Sky::timef() const { return timef_->getf(); }

const ref_ptr<ShaderInput1f> &Sky::timeUniform() const { return timeUniform_; }

void Sky::set_time(const time_t &time) {
	timef_->sett(time, true);
	REGEN_INFO("Set sky time to: " << date());
}

void Sky::set_timestamp(double seconds) { set_time(static_cast<time_t>(seconds)); }

void Sky::set_date(const std::string &date) {
	struct tm tm;
	if (strptime(date.c_str(), "%d-%m-%Y %H:%M:%S", &tm)) {
		set_time(mktime(&tm));
		animate(0.0);
	} else {
		REGEN_WARN("Invalid date string: " << date << ".");
	}
}

void Sky::set_utcOffset(const time_t &offset) { timef_->setUtcOffset(offset); }

void Sky::set_secondsPerCycle(GLdouble secondsPerCycle) { timef_->setSecondsPerCycle(secondsPerCycle); }

osgHimmel::AbstractAstronomy &Sky::astro() { return *astro_.get(); }

void Sky::set_astro(const ref_ptr<osgHimmel::AbstractAstronomy> &astro) { astro_ = astro; }

const Vec3f &Sky::noonColor() { return noonColor_; }

void Sky::set_noonColor(const Vec3f &noonColor) { noonColor_ = noonColor; }

const Vec3f &Sky::dawnColor() { return dawnColor_; }

void Sky::set_dawnColor(const Vec3f &dawnColor) { dawnColor_ = dawnColor; }

GLfloat Sky::moonSunLightReflectance() { return moonSunLightReflectance_; }

void Sky::set_moonSunLightReflectance(
		GLfloat moonSunLightReflectance) { moonSunLightReflectance_ = moonSunLightReflectance; }

GLdouble Sky::altitude() const { return cmnUniform_->getVertex(0).x; }

GLdouble Sky::longitude() const { return astro_->getLongitude(); }

GLdouble Sky::latitude() const { return astro_->getLatitude(); }

void Sky::set_altitude(const GLdouble altitude) {
	Vec4f &v = *((Vec4f *) cmnUniform_->clientDataPtr());
	// Clamp altitude into non uniform atmosphere. (min alt is 1m)
	v.x = math::clamp(altitude, 0.001f, osgHimmel::Earth::atmosphereThicknessNonUniform());
	cmnUniform_->nextStamp();
}

void Sky::set_longitude(const GLdouble longitude) { astro_->setLongitude(longitude); }

void Sky::set_latitude(const GLdouble latitude) { astro_->setLatitude(latitude); }

ref_ptr<Light> &Sky::sun() { return sun_; }

ref_ptr<Light> &Sky::moon() { return moon_; }

ref_ptr<Camera> &Sky::camera() { return cam_; }

ref_ptr<ShaderInput2i> &Sky::viewport() { return viewport_; }

void Sky::updateSeed() {
	Vec4f &v = *((Vec4f *) cmnUniform_->clientDataPtr());
	v.w = rand();
	cmnUniform_->nextStamp();
}

void Sky::addLayer(const ref_ptr<SkyLayer> &layer) {
	addChild(layer);
	this->layer_.push_back(layer);
}

void Sky::createShader(RenderState *rs, const StateConfig &stateCfg) {
	for (auto  it = layer_.begin(); it != layer_.end(); ++it) {
		ref_ptr<SkyLayer> layer = *it;

		StateConfigurer cfg(stateCfg);
		cfg.addNode(layer.get());

		layer->getShaderState()->createShader(cfg.cfg());
		layer->getMeshState()->updateVAO(RenderState::get(), cfg.cfg(),
										 layer->getShaderState()->shaderState()->shader());
	}
}

void Sky::startAnimation() {
	if (isRunning_) return;
	if (!timef_->isRunning())
		timef_->start(false);
	Animation::startAnimation();
}

void Sky::stopAnimation() {
	if (!isRunning_) return;
	if (timef_->isRunning())
		timef_->stop(false);
	Animation::stopAnimation();
}

void Sky::animate(GLdouble dt) {
	timef_->update();
	astro_->update(osgHimmel::t_aTime::fromTimeF(*timef_.get()));
}

GLfloat Sky::computeHorizonExtinction(const Vec3f& position, const Vec3f& dir, GLfloat radius) {
	GLfloat u = dir.dot(-position);
	if (u < 0.0) {
		return 1.0;
	}
	Vec3f near = position + dir * u;
	if (near.length() < radius) {
		return 0.0;
	} else {
		near.normalize();
		Vec3f v2 = near * radius - position;
		v2.normalize();
		GLfloat diff = acos(v2.dot(dir));
		return math::smoothstep(0.0, 1.0, pow(diff * 2.0, 3.0));
	}
}

GLfloat Sky::computeEyeExtinction(const Vec3f& eyedir) {
	GLfloat surfaceHeight = 0.99;
	Vec3f eyePosition(0.0, surfaceHeight, 0.0);
	return computeHorizonExtinction(eyePosition, eyedir, surfaceHeight - 0.15);
}

static Vec3f computeColor(const Vec3f& color, GLfloat ext) {
	if (ext >= 0.0) {
		return color;
	} else {
		return color * (-ext * ext + 1.0);
	}
}

void Sky::glAnimate(RenderState *rs, GLdouble dt) {
	timeUniform_->setVertex(0, timef_->getf());
	// Compute sun/moon directions
	Vec3f moon = astro_->getMoonPosition(false);
	Vec3f sun = astro_->getSunPosition(false);
	sun_->direction()->setVertex(0, sun);
	moon_->direction()->setVertex(0, moon);
	// Compute sun/moon diffuse color
	GLfloat sunExt = computeEyeExtinction(sun);
	Vec3f sunColor = math::mix(dawnColor_, noonColor_, abs(sunExt));
	sun_->diffuse()->setVertex(0, computeColor(
			sunColor,
			sunExt)
	);
	moon_->diffuse()->setVertex(0, computeColor(
			sunColor * moonSunLightReflectance_,
			computeEyeExtinction(moon))
	);

	const float fovHalf = camera()->fov()->getVertex(0) * 0.5 * DEGREE_TO_RAD;
	const float height = viewport()->getVertex(0).y;
	q_->setUniformData(sqrt(2.0) * 2.0 * tan(fovHalf) / height);
	R_->setVertex(0, astro().getEquToHorTransform());
	// Update random number in cmn uniform
	updateSeed();

	for (auto it = layer_.begin(); it != layer_.end(); ++it) {
		(*it)->updateSky(rs, dt);
	}
}

SkyView::SkyView(const ref_ptr<Sky> &sky)
		: StateNode(),
		  sky_(sky) {
	for (auto it = sky->layer_.begin(); it != sky->layer_.end(); ++it) {
		addLayer(*it);
	}
}

void SkyView::addLayer(const ref_ptr<SkyLayer> &layer) {
	ref_ptr<SkyLayerView> view = ref_ptr<SkyLayerView>::alloc(sky_, layer);
	addChild(view);
	this->layer_.push_back(view);
}

void SkyView::traverse(RenderState *rs) {
	if (!isHidden_ && !state_->isHidden()) {
		sky_->state()->enable(rs);
		StateNode::traverse(rs);
		sky_->state()->disable(rs);
	}
}

const ref_ptr<Sky> &SkyView::sky() { return sky_; }

void SkyView::createShader(RenderState *rs, const StateConfig &stateCfg) {
	for (auto it = layer_.begin(); it != layer_.end(); ++it) {
		ref_ptr<SkyLayerView> layer = *it;

		StateConfigurer cfg(stateCfg);
		cfg.addNode(layer.get());

		layer->getShaderState()->createShader(cfg.cfg());
		layer->getMeshState()->updateVAO(RenderState::get(), cfg.cfg(),
										 layer->getShaderState()->shaderState()->shader());
	}
}
