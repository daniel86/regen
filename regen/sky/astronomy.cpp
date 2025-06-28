// Code partly based on the original code from osgHimmel:
//   Copyright (c) 2011-2012, Daniel Müller <dm@g4t3.de>
//   Computer Graphics Systems Group at the Hasso-Plattner-Institute, Germany

#include "astronomy.h"
#include "moon.h"
#include "sun.h"
#include "earth.h"

#include "regen/external/osghimmel/siderealtime.h"

using namespace regen;
using namespace osgHimmel;

Astronomy::Astronomy()
		:
		m_latitude(0.f), m_longitude(0.f) {
}

float Astronomy::sunDistance(const t_julianDay &t) {
	return Sun::distance(t);
}

float Astronomy::angularSunRadius(const t_julianDay &t) {
	return Earth::apparentAngularSunDiameter(t) * 0.5f;
}

float Astronomy::moonRadius() {
	return Moon::meanRadius();
}

float Astronomy::moonDistance(const t_julianDay &t) {
	return Moon::distance(t);
}

float Astronomy::angularMoonRadius(const t_julianDay &t) {
	return Earth::apparentAngularMoonDiameter(t) * 0.5f;
}

Vec3f Astronomy::moonPosition(const t_aTime &aTime, float latitude,
							  float longitude, bool refractionCorrected) {
	t_horf moon = Moon::horizontalPosition(aTime, latitude, longitude);
	if (refractionCorrected)
		moon.altitude += Earth::atmosphericRefraction(moon.altitude);

	regen::Vec3f moon_v = moon.toEuclidean();
	moon_v.normalize();
	return moon_v;
}

Vec3f Astronomy::sunPosition(const t_aTime &aTime, float latitude,
							 float longitude, bool refractionCorrected) {
	t_horf sun = Sun::horizontalPosition(aTime, latitude, longitude);
	if (refractionCorrected)
		sun.altitude += Earth::atmosphericRefraction(sun.altitude);

	Vec3f sun_v = sun.toEuclidean();
	sun_v.normalize();
	return sun_v;
}

Mat4f Astronomy::moonOrientation(const t_aTime &aTime, float latitude, float longitude) {
	const t_julianDay t(jd(aTime));

	float l, b;
	Moon::opticalLibrations(t, l, b);

	const Mat4f libLat = Mat4f::rotationMatrix(-_rad(b), 0, 0);
	const Mat4f libLon = Mat4f::rotationMatrix(0, _rad(l), 0);

	const float a = _rad(Moon::positionAngleOfAxis(t));
	const float p = _rad(Moon::parallacticAngle(aTime, latitude, longitude));

	const Mat4f zenith = Mat4f::rotationMatrix(0, 0, p - a);
	const Mat4f R(libLat * libLon * zenith);
	return R;
}

float Astronomy::earthShineIntensity(const t_aTime &aTime, float latitude, float longitude) {
	auto m = moonPosition(aTime, latitude, longitude, false);
	auto s = sunPosition(aTime, latitude, longitude, false);

	// ("Multiple Light Scattering" - 1980 - Van de Hulst) and
	// ("A Physically-Based Night Sky Model" - 2001 - Wann Jensen et al.) -> the 0.19 is the earth full intensity
	const float ep = acos(s.dot(-m));
	const float ep2 = ep * ep;
	const float ep3 = ep * ep2;
	const float Eem = -0.0061f * ep3 + 0.0289f * ep2 - 0.0105f * sin(ep);

	return Eem;
}

Mat4f Astronomy::equToHorTransform(const t_aTime &aTime, float latitude, float longitude) {
	auto s = static_cast<float>(siderealTime(aTime));
	return Mat4f::scaleMatrix(Vec3f(-1, 1, 1))
		   * Mat4f::rotationMatrix(_rad(latitude) - _PI_2, 0, 0)
		   * Mat4f::rotationMatrix(0, 0, -_rad(s + longitude));
}

void Astronomy::update(const t_aTime &aTime) {
	m_aTime = aTime;
	m_t = jd(aTime);
}

float Astronomy::setLatitude(const float latitude) {
	if (latitude != m_latitude)
		m_latitude = _clamp(-90, +90, latitude);

	return getLatitude();
}

float Astronomy::getLatitude() const {
	return m_latitude;
}

float Astronomy::setLongitude(const float longitude) {
	if (longitude != m_longitude)
		m_longitude = _clamp(-180, +180, longitude);

	return getLongitude();
}

float Astronomy::getLongitude() const {
	return m_longitude;
}

regen::Mat4f Astronomy::getMoonOrientation() const {
	return moonOrientation(getATime(), getLatitude(), getLongitude());
}

regen::Mat4f Astronomy::getMoonOrientation(
		const t_aTime &aTime, float latitude, float longitude) {
	return moonOrientation(aTime, latitude, longitude);
}

regen::Vec3f Astronomy::getMoonPosition(bool refractionCorrected) const {
	return moonPosition(getATime(), getLatitude(), getLongitude(), refractionCorrected);
}

regen::Vec3f Astronomy::getMoonPosition(
		const t_aTime &aTime, float latitude, float longitude, bool refractionCorrected) {
	return moonPosition(aTime, latitude, longitude, refractionCorrected);
}


regen::Vec3f Astronomy::getSunPosition(bool refractionCorrected) const {
	return sunPosition(getATime(), getLatitude(), getLongitude(), refractionCorrected);
}

regen::Vec3f Astronomy::getSunPosition(
		const t_aTime &aTime, float latitude, float longitude, bool refractionCorrected) {
	return sunPosition(aTime, latitude, longitude, refractionCorrected);
}

float Astronomy::getEarthShineIntensity() const {
	return earthShineIntensity(getATime(), getLatitude(), getLongitude());
}

float Astronomy::getEarthShineIntensity(
		const t_aTime &aTime, float latitude, float longitude) {
	return earthShineIntensity(aTime, latitude, longitude);
}

float Astronomy::getSunDistance() const {
	return sunDistance(t());
}

float Astronomy::getSunDistance(const t_aTime &aTime) const {
	return sunDistance(jd(aTime));
}

float Astronomy::getAngularSunRadius() const {
	return angularSunRadius(t());
}

float Astronomy::getAngularSunRadius(const t_aTime &aTime) const {
	return angularSunRadius(jd(aTime));
}

float Astronomy::getMoonDistance() const {
	return moonDistance(t());
}

float Astronomy::getMoonDistance(const t_aTime &aTime) const {
	return moonDistance(jd(aTime));
}

float Astronomy::getMoonRadius() const {
	return moonRadius();
}

float Astronomy::getAngularMoonRadius() const {
	return angularMoonRadius(t());
}

float Astronomy::getAngularMoonRadius(const t_aTime &aTime) const {
	return angularMoonRadius(jd(aTime));
}

regen::Mat4f Astronomy::getEquToHorTransform() const {
	return equToHorTransform(getATime(), getLatitude(), getLongitude());
}

regen::Mat4f Astronomy::getEquToHorTransform(
		const t_aTime &aTime, float latitude, float longitude) {
	return equToHorTransform(aTime, latitude, longitude);
}

