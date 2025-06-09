
// Copyright (c) 2011-2012, Daniel Müller <dm@g4t3.de>
// Computer Graphics Systems Group at the Hasso-Plattner-Institute, Germany
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright 
//     notice, this list of conditions and the following disclaimer in the 
//     documentation and/or other materials provided with the distribution.
//   * Neither the name of the Computer Graphics Systems Group at the 
//     Hasso-Plattner-Institute (HPI), Germany nor the names of its 
//     contributors may be used to endorse or promote products derived from 
//     this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
// POSSIBILITY OF SUCH DAMAGE.

#include "abstractastronomy.h"

#include "mathmacros.h"


using namespace osgHimmel;

AbstractAstronomy::AbstractAstronomy()
		:
		m_latitude(0.f), m_longitude(0.f) {
}


AbstractAstronomy::~AbstractAstronomy() {
}

void AbstractAstronomy::update(const t_aTime &aTime) {
	m_aTime = aTime;
	m_t = jd(aTime);
}

float AbstractAstronomy::setLatitude(const float latitude) {
	if (latitude != m_latitude)
		m_latitude = _clamp(-90, +90, latitude);

	return getLatitude();
}

float AbstractAstronomy::getLatitude() const {
	return m_latitude;
}

float AbstractAstronomy::setLongitude(const float longitude) {
	if (longitude != m_longitude)
		m_longitude = _clamp(-180, +180, longitude);

	return getLongitude();
}

float AbstractAstronomy::getLongitude() const {
	return m_longitude;
}

regen::Mat4f AbstractAstronomy::getMoonOrientation() const {
	return moonOrientation(getATime(), getLatitude(), getLongitude());
}

regen::Mat4f AbstractAstronomy::getMoonOrientation(
		const t_aTime &aTime, float latitude, float longitude) const {
	return moonOrientation(aTime, latitude, longitude);
}

regen::Vec3f AbstractAstronomy::getMoonPosition(bool refractionCorrected) const {
	return moonPosition(getATime(), getLatitude(), getLongitude(), refractionCorrected);
}

regen::Vec3f AbstractAstronomy::getMoonPosition(
		const t_aTime &aTime, float latitude, float longitude, bool refractionCorrected) const {
	return moonPosition(aTime, latitude, longitude, refractionCorrected);
}


regen::Vec3f AbstractAstronomy::getSunPosition(bool refractionCorrected) const {
	return sunPosition(getATime(), getLatitude(), getLongitude(), refractionCorrected);
}

regen::Vec3f AbstractAstronomy::getSunPosition(
		const t_aTime &aTime, float latitude, float longitude, bool refractionCorrected) const {
	return sunPosition(aTime, latitude, longitude, refractionCorrected);
}

float AbstractAstronomy::getEarthShineIntensity() const {
	return earthShineIntensity(getATime(), getLatitude(), getLongitude());
}

float AbstractAstronomy::getEarthShineIntensity(
		const t_aTime &aTime, float latitude, float longitude) const {
	return earthShineIntensity(aTime, latitude, longitude);
}

float AbstractAstronomy::getSunDistance() const {
	return sunDistance(t());
}

float AbstractAstronomy::getSunDistance(const t_aTime &aTime) const {
	return sunDistance(jd(aTime));
}

float AbstractAstronomy::getAngularSunRadius() const {
	return angularSunRadius(t());
}

float AbstractAstronomy::getAngularSunRadius(const t_aTime &aTime) const {
	return angularSunRadius(jd(aTime));
}

float AbstractAstronomy::getMoonDistance() const {
	return moonDistance(t());
}

float AbstractAstronomy::getMoonDistance(const t_aTime &aTime) const {
	return moonDistance(jd(aTime));
}

float AbstractAstronomy::getMoonRadius() const {
	return moonRadius();
}

float AbstractAstronomy::getAngularMoonRadius() const {
	return angularMoonRadius(t());
}

float AbstractAstronomy::getAngularMoonRadius(const t_aTime &aTime) const {
	return angularMoonRadius(jd(aTime));
}

regen::Mat4f AbstractAstronomy::getEquToHorTransform() const {
	return equToHorTransform(getATime(), getLatitude(), getLongitude());
}

regen::Mat4f AbstractAstronomy::getEquToHorTransform(
		const t_aTime &aTime, float latitude, float longitude) const {
	return equToHorTransform(aTime, latitude, longitude);
}
