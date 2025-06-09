
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

#pragma once
#ifndef OSG_HIMMEL_ABSTRACT_ASTRONOMY_H_
#define OSG_HIMMEL_ABSTRACT_ASTRONOMY_H_

#include "declspec.h"
#include "atime.h"
#include "julianday.h"
#include <regen/math/vector.h>
#include <regen/math/matrix.h>

namespace osgHimmel {

	class AbstractAstronomy {
	public:

		AbstractAstronomy();

		virtual ~AbstractAstronomy();

		void update(const t_aTime &aTime);

		inline const t_aTime &getATime() const {
			return m_aTime;
		}

		float setLatitude(float latitude);

		float getLatitude() const;

		float setLongitude(float longitude);

		float getLongitude() const;

		regen::Mat4f getMoonOrientation() const;

		regen::Mat4f getMoonOrientation(
				const t_aTime &aTime, float latitude, float longitude) const;

		regen::Vec3f getMoonPosition(bool refractionCorrected) const;

		regen::Vec3f getMoonPosition(
				const t_aTime &aTime, float latitude, float longitude, bool refractionCorrected) const;

		regen::Vec3f getSunPosition(bool refractionCorrected) const;

		regen::Vec3f getSunPosition(
				const t_aTime &aTime, float latitude, float longitude, bool refractionCorrected) const;

		float getEarthShineIntensity() const;

		float getEarthShineIntensity(
				const t_aTime &aTime, float latitude, float longitude) const;

		float getSunDistance() const;

		float getSunDistance(const t_aTime &aTime) const;

		float getAngularSunRadius() const;

		float getAngularSunRadius(const t_aTime &aTime) const;

		float getMoonDistance() const;

		float getMoonDistance(const t_aTime &aTime) const;

		float getMoonRadius() const;

		float getAngularMoonRadius() const;

		float getAngularMoonRadius(const t_aTime &aTime) const;

		regen::Mat4f getEquToHorTransform() const;

		regen::Mat4f getEquToHorTransform(
				const t_aTime &aTime, float latitude, float longitude) const;


	protected:

		virtual regen::Vec3f moonPosition(
				const t_aTime &aTime, float latitude, float longitude, bool refractionCorrected) const = 0;

		virtual regen::Vec3f sunPosition(
				const t_aTime &aTime, float latitude, float longitude, bool refractionCorrected) const = 0;

		virtual regen::Mat4f moonOrientation(
				const t_aTime &aTime, float latitude, float longitude) const = 0;

		virtual float earthShineIntensity(
				const t_aTime &aTime, float latitude, float longitude) const = 0;

		virtual float sunDistance(const t_julianDay &t) const = 0;

		virtual float angularSunRadius(const t_julianDay &t) const = 0;

		virtual float moonRadius() const = 0;

		virtual float moonDistance(const t_julianDay &t) const = 0;

		virtual float angularMoonRadius(const t_julianDay &t) const = 0;

		virtual regen::Mat4f equToHorTransform(
				const t_aTime &aTime, float latitude, float longitude) const = 0;

		inline const t_julianDay &t() const {
			return m_t;
		}

	protected:

		t_aTime m_aTime;
		t_julianDay m_t;

		float m_latitude;
		float m_longitude;
	};

} // namespace osgHimmel

#endif // OSG_HIMMEL_ABSTRACT_ASTRONOMY_H_
