// Code partly based on the original code from osgHimmel:
//   Copyright (c) 2011-2012, Daniel Müller <dm@g4t3.de>
//   Computer Graphics Systems Group at the Hasso-Plattner-Institute, Germany

#include "moon.h"
#include "sun.h"
#include "earth.h"

#include "regen/external/osghimmel/siderealtime.h"
#include <regen/external/osghimmel/noise.h>
#include <regen/textures/texture-loader.h>

using namespace regen;
using namespace osgHimmel;

Moon::Moon(const ref_ptr<Sky> &sky, std::string_view moonMapFile)
		: SkyLayer(sky) {
	state()->joinStates(ref_ptr<BlendFuncState>::alloc(
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
			GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

	setupMoonTextureCube(moonMapFile);

	moonOrientation_ = ref_ptr<ShaderInputMat4>::alloc("moonOrientationMatrix");
	moonOrientation_->setUniformData(Mat4f::identity());
	state()->setInput(moonOrientation_);

	sunShine_ = ref_ptr<ShaderInput4f>::alloc("sunShine");
	sunShine_->setUniformData(Vec4f::create(defaultSunShineColor(), defaultSunShineIntensity()));
	state()->setInput(sunShine_);

	earthShine_ = ref_ptr<ShaderInput3f>::alloc("earthShine");
	earthShine_->setUniformData(Vec3f::zero());
	state()->setInput(earthShine_);
	earthShineColor_ = defaultEarthShineColor();
	earthShineIntensity_ = defaultEarthShineIntensity();

	scale_ = ref_ptr<ShaderInput1f>::alloc("scale");
	scale_->setUniformData(defaultScale());
	state()->setInput(scale_);

	scattering_ = ref_ptr<ShaderInput1f>::alloc("scattering");
	scattering_->setUniformData(defaultScattering());
	state()->setInput(scattering_);

	shaderState_ = ref_ptr<HasShader>::alloc("regen.weather.moon");
	meshState_ = ref_ptr<Rectangle>::alloc(sky->skyQuad());
}

void Moon::setupMoonTextureCube(std::string_view moonMapFile) {
	ref_ptr<TextureCube> texture = textures::loadCube(moonMapFile, true);
	state()->joinStates(ref_ptr<TextureState>::alloc(texture, "moonmapCube"));
}

float Moon::defaultScale() {
	return 0.1;
}

float Moon::defaultScattering() {
	return 4.0;
}

Vec3f Moon::defaultSunShineColor() {
	return {0.923, 0.786, 0.636};
}

float Moon::defaultSunShineIntensity() {
	return 128.0;
}

Vec3f Moon::defaultEarthShineColor() {
	return {0.88, 0.96, 1.0};
}

float Moon::defaultEarthShineIntensity() {
	return 4.0;
}

float Moon::meanLongitude(const t_julianDay &t) {
	// Mean longitude, referred to the mean equinox of the date (AA.45.1).
	const t_julianDay T(jCenturiesSinceSE(t));
	const float L0 = _deg(3.8104 + 8399.7091 * T);
	return _revd(L0);
}

float Moon::meanElongation(const t_julianDay &t) {
	// Mean elongation (AA.45.2).
	const t_julianDay T(jCenturiesSinceSE(t));
	const float D = _deg(5.1985 + 7771.3772 * T);
	return _revd(D);
}

float Moon::meanAnomaly(const t_julianDay &t) {
	const t_julianDay T(jCenturiesSinceSE(t));
	const float M = _deg(2.3554 + 8328.6911 * T);
	return _revd(M);
}

float Moon::meanLatitude(const t_julianDay &t) {
	// Mean distance of the Moon from its ascending node (AA.45.5)
	const t_julianDay T(jCenturiesSinceSE(t));
	const float F = _deg(1.6280 + 8433.4663 * T);
	return _revd(F);
}

float Moon::meanOrbitLongitude(const t_julianDay &t) {
	const t_julianDay T(jCenturiesSinceSE(t));
	// (AA p152)
	const float O = 125.04f + T * (-1934.136f);
	return _revd(O);
}

t_eclf Moon::position(const t_julianDay &t) {
	const float sM = _rad(Sun::meanAnomaly(t));
	const float mL = _rad(meanLongitude(t));
	const float mM = _rad(meanAnomaly(t));
	const float mD = _rad(meanElongation(t));
	const float mF = _rad(meanLatitude(t));

	// ("A Physically-Based Night Sky Model" - 2001 - Wann Jensen et al.)

	float Sl = mL;

	Sl += 0.1098f * sinf(+1 * mM);
	Sl += 0.0222f * sinf(2 * mD - 1 * mM);
	Sl += 0.0115f * sinf(2 * mD);
	Sl += 0.0037f * sinf(+2 * mM);
	Sl -= 0.0032f * sinf(+1 * sM);
	Sl -= 0.0020f * sinf(+2 * mF);
	Sl += 0.0010f * sinf(2 * mD - 2 * mM);
	Sl += 0.0010f * sinf(2 * mD - 1 * sM - 1 * mM);
	Sl += 0.0009f * sinf(2 * mD + 1 * mM);
	Sl += 0.0008f * sinf(2 * mD - 1 * sM);
	Sl -= 0.0007f * sinf(+1 * sM - 1 * mM);
	Sl -= 0.0006f * sinf(1 * mD);
	Sl -= 0.0005f * sinf(+1 * sM + 1 * mM);

	float Sb = 0.0;

	Sb += 0.0895f * sinf(+1 * mF);
	Sb += 0.0049f * sinf(+1 * mM + 1 * mF);
	Sb += 0.0048f * sinf(+1 * mM - 1 * mF);
	Sb += 0.0030f * sinf(2 * mD - 1 * mF);
	Sb += 0.0010f * sinf(2 * mD - 1 * mM + 1 * mF);
	Sb += 0.0008f * sinf(2 * mD - 1 * mM - 1 * mF);
	Sb += 0.0006f * sinf(2 * mD + 1 * mF);

	t_eclf ecl;

	ecl.longitude = _deg(Sl);
	ecl.latitude = _deg(Sb);

	return ecl;
}

t_equf Moon::apparentPosition(const t_julianDay &t) {
	t_eclf ecl = position(t);
	return ecl.toEquatorial(Earth::trueObliquity(t));
}

t_horf Moon::horizontalPosition(const t_aTime &aTime, float latitude, float longitude) {
	t_julianDay t(jd(aTime));
	t_julianDay s(siderealTime(aTime));
	t_equf equ = apparentPosition(t);
	return equ.toHorizontal(s, latitude, longitude);
}

float Moon::distance(const t_julianDay &t) {
	// NOTE: This gives the distance from the center of the moon to the
	// center of the earth.
	const float sM = _rad(Sun::meanAnomaly(t));
	const float mM = _rad(meanAnomaly(t));
	const float mD = _rad(meanElongation(t));

	float Sr = 0.016593;
	Sr += 0.000904f * cosf(1 * mM);
	Sr += 0.000166f * cosf(2 * mD - 1 * mM);
	Sr += 0.000137f * cosf(2 * mD);
	Sr += 0.000049f * cosf(2 * mM);
	Sr += 0.000015f * cosf(2 * mD + 1 * mM);
	Sr += 0.000009f * cosf(2 * mD - 1 * sM);
	return Earth::meanRadius() / Sr;
}

void Moon::opticalLibrations(const t_julianDay &t, float &l, float &b) {
	// (AA.51.1)
	const float Dr = _rad(Earth::longitudeNutation(t));
	const float F = _rad(meanLatitude(t));
	const float O = _rad(meanOrbitLongitude(t));

	const t_eclf ecl = position(t);
	const float lo = _rad(ecl.longitude);
	const float la = _rad(ecl.latitude);

	static const float I = _rad(1.54242f);

	const float cos_la = cosf(la);
	const float sin_la = sinf(la);
	const float cos_I = cosf(I);
	const float sin_I = sinf(I);

	const float W = _rev(lo - Dr - O);
	const float sin_W = sinf(W);

	const float A = _rev(atan2(sin_W * cos_la * cos_I - sin_la * sin_I, cos(W) * cos_la));

	l = _deg(A - F);
	b = _deg(asinf(-sin_W * cos_la * sin_I - sin_la * cos_I));
}

float Moon::parallacticAngle(const t_aTime &aTime, float latitude, float longitude) {
	// (AA.13.1)
	const t_julianDay t(jd(aTime));

	const float la = _rad(latitude);
	const float lo = _rad(longitude);

	const t_equf pos = apparentPosition(t);
	const float ra = _rad(pos.right_ascension);
	const float de = _rad(pos.declination);

	const float s = _rad(siderealTime(aTime));

	// (AA.p88) - local hour angle

	const float H = s + lo - ra;

	const float cos_la = cos(la);
	const float P = atan2f(sin(H) * cos_la, sin(la) * cos(de) - sin(de) * cos_la * cos(H));

	return _deg(P);
}

float Moon::positionAngleOfAxis(const t_julianDay &t) {
	// (AA.p344)
	const t_equf pos = apparentPosition(t);

	const float a = _rad(pos.right_ascension);
	const float e = _rad(Earth::meanObliquity(t));

	const float Dr = _rad(Earth::longitudeNutation(t));
	const float O = _rad(meanOrbitLongitude(t));

	const float V = O + Dr;

	static const float I = _rad(1.54242);
	const float sin_I = sinf(I);

	const float X = sin_I * sinf(V);
	const float Y = sin_I * cosf(V) * cosf(e) - cosf(I) * sinf(e);

	// optical libration in latitude

	const t_eclf ecl = position(t);

	const float lo = _rad(ecl.longitude);
	const float la = _rad(ecl.latitude);

	const float W = _rev(lo - Dr - O);
	const float b = asinf(-sinf(W) * cosf(la) * sin_I - sinf(la) * cosf(I));

	// final angle

	const float w = _rev(atan2(X, Y));
	const float P = asinf(sqrt(X * X + Y * Y) * cosf(a - w) / cosf(b));

	return _deg(P);
}

float Moon::meanRadius() {
	// http://nssdc.gsfc.nasa.gov/planetary/factsheet/moonfact.html
	static const float r = 1737.1f; // in kilometers
	return r;
}

void Moon::set_sunShineColor(const Vec3f &color) {
	auto v_sunShine = sunShine_->mapClientVertex<Vec4f>(BUFFER_GPU_READ | BUFFER_GPU_WRITE, 0);
	v_sunShine.w = Vec4f::create(color, v_sunShine.r.w);
}

void Moon::set_sunShineIntensity(float intensity) {
	auto v_color = sunShine_->mapClientVertex<Vec4f>(BUFFER_GPU_READ | BUFFER_GPU_WRITE, 0);
	v_color.w = Vec4f::create(v_color.r.xyz(), intensity);
}

void Moon::updateSkyLayer(RenderState *rs, double dt) {
	moonOrientation_->setVertex(0, sky_->astro().getMoonOrientation());
	earthShine_->setVertex(0, earthShineColor_ *
							  (sky_->astro().getEarthShineIntensity() * earthShineIntensity_));
}
