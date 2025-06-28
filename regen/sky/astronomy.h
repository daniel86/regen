// Code partly based on the original code from osgHimmel:
//   Copyright (c) 2011-2012, Daniel Müller <dm@g4t3.de>
//   Computer Graphics Systems Group at the Hasso-Plattner-Institute, Germany

#pragma once
#ifndef REGEN_ASTRONOMY_H_
#define REGEN_ASTRONOMY_H_

#include "regen/math/vector.h"
#include "regen/math/matrix.h"

#include "regen/external/osghimmel/atime.h"
#include "regen/external/osghimmel/julianday.h"

namespace regen {
	class Astronomy {
	public:
		Astronomy();

		void update(const osgHimmel::t_aTime &aTime);

		inline const osgHimmel::t_aTime &getATime() const {
			return m_aTime;
		}

		float setLatitude(float latitude);

		float getLatitude() const;

		float setLongitude(float longitude);

		float getLongitude() const;

		Mat4f getMoonOrientation() const;

		static Mat4f getMoonOrientation(
				const osgHimmel::t_aTime &aTime, float latitude, float longitude);

		Vec3f getMoonPosition(bool refractionCorrected) const;

		static Vec3f getMoonPosition(
				const osgHimmel::t_aTime &aTime, float latitude, float longitude, bool refractionCorrected);

		Vec3f getSunPosition(bool refractionCorrected) const;

		static Vec3f getSunPosition(
				const osgHimmel::t_aTime &aTime, float latitude, float longitude, bool refractionCorrected);

		float getEarthShineIntensity() const;

		static float getEarthShineIntensity(
				const osgHimmel::t_aTime &aTime, float latitude, float longitude);

		float getSunDistance() const;

		float getSunDistance(const osgHimmel::t_aTime &aTime) const;

		float getAngularSunRadius() const;

		float getAngularSunRadius(const osgHimmel::t_aTime &aTime) const;

		float getMoonDistance() const;

		float getMoonDistance(const osgHimmel::t_aTime &aTime) const;

		float getMoonRadius() const;

		float getAngularMoonRadius() const;

		float getAngularMoonRadius(const osgHimmel::t_aTime &aTime) const;

		Mat4f getEquToHorTransform() const;

		static Mat4f getEquToHorTransform(
				const osgHimmel::t_aTime &aTime, float latitude, float longitude);

	protected:
		osgHimmel::t_aTime m_aTime;
		osgHimmel::t_julianDay m_t;

		float m_latitude;
		float m_longitude;

		inline const osgHimmel::t_julianDay &t() const { return m_t; }

		static Vec3f moonPosition(
				const osgHimmel::t_aTime &aTime, float latitude, float longitude,
				bool refractionCorrected);

		static Vec3f sunPosition(
				const osgHimmel::t_aTime &aTime, float latitude, float longitude,
				bool refractionCorrected);

		static Mat4f moonOrientation(const osgHimmel::t_aTime &aTime, float latitude, float longitude);

		static float earthShineIntensity(const osgHimmel::t_aTime &aTime, float latitude, float longitude);

		static Mat4f equToHorTransform(const osgHimmel::t_aTime &aTime, float latitude, float longitude);

		static float sunDistance(const osgHimmel::t_julianDay &t);

		static float angularSunRadius(const osgHimmel::t_julianDay &t);

		static float moonRadius();

		static float moonDistance(const osgHimmel::t_julianDay &t);

		static float angularMoonRadius(const osgHimmel::t_julianDay &t);
	};

} // namespace regen

#endif // REGEN_ASTRONOMY_H_
