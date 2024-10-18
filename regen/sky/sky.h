/*
 * sky.h
 *
 *  Created on: Oct 3, 2014
 *      Author: daniel
 */

#ifndef SKY_H_
#define SKY_H_

#include <list>

#include <regen/camera/camera.h>
#include <regen/states/light-state.h>
#include <regen/sky/sky-layer.h>
#include <regen/states/blend-state.h>
#include <regen/meshes/rectangle.h>

#include <regen/external/osghimmel/timef.h>
#include <regen/external/osghimmel/astronomy.h>

namespace regen {

	class Sky : public StateNode, public Animation {
	public:
		Sky(const ref_ptr<Camera> &cam,
			const ref_ptr<ShaderInput2i> &viewport);

		void set_time(const time_t &time);

		void set_timestamp(GLdouble timestamp);

		void set_date(const std::string &date);

		void set_utcOffset(const time_t &offset);

		void set_secondsPerCycle(GLdouble secondsPerCycle);

		std::string date() const;

		/**
		 * Float time in the interval [0;1]
		 */
		GLdouble timef() const;

		const ref_ptr<ShaderInput1f> &timeUniform() const;


		void set_altitude(GLdouble altitude);

		GLdouble altitude() const;

		void set_longitude(GLdouble longitude);

		GLdouble longitude() const;

		void set_latitude(GLdouble latitude);

		GLdouble latitude() const;


		ref_ptr<Light> &sun();

		ref_ptr<Light> &moon();

		ref_ptr<Camera> &camera();

		ref_ptr<ShaderInput2i> &viewport();


		const Vec3f &noonColor();

		void set_noonColor(const Vec3f &noonColor);

		const Vec3f &dawnColor();

		void set_dawnColor(const Vec3f &dawnColor);

		GLfloat moonSunLightReflectance();

		void set_moonSunLightReflectance(GLfloat moonSunLightReflectance);


		GLfloat computeHorizonExtinction(const Vec3f& position, const Vec3f& dir, GLfloat radius);

		GLfloat computeEyeExtinction(const Vec3f& eyedir);

		const ref_ptr<Rectangle> &skyQuad() const;


		osgHimmel::AbstractAstronomy &astro();

		void set_astro(const ref_ptr<osgHimmel::AbstractAstronomy> &astro);

		void addLayer(const ref_ptr<SkyLayer> &layer);

		void createShader(RenderState *rs, const StateConfig &stateCfg);

		// override
		void animate(GLdouble dt) override;

		void glAnimate(RenderState *rs, GLdouble dt) override;

		void startAnimation() override;

		void stopAnimation() override;

	protected:
		friend class SkyView;

		ref_ptr<Camera> cam_;
		ref_ptr<ShaderInput2i> viewport_;

		ref_ptr<osgHimmel::TimeF> timef_;
		ref_ptr<osgHimmel::AbstractAstronomy> astro_;

		std::list<ref_ptr<SkyLayer> > layer_;

		ref_ptr<Light> sun_;
		ref_ptr<Light> moon_;
		ref_ptr<ShaderInput1f> timeUniform_;
		ref_ptr<ShaderInput4f> cmnUniform_;
		ref_ptr<ShaderInputMat4> R_;
		ref_ptr<ShaderInput1f> q_;
		ref_ptr<Rectangle> skyQuad_;

		Vec3f noonColor_;
		Vec3f dawnColor_;
		GLfloat moonSunLightReflectance_;


		void updateSeed();
	};

	class SkyView : public StateNode {
	public:
		explicit SkyView(const ref_ptr<Sky> &sky);

		const ref_ptr<Sky> &sky();

		void addLayer(const ref_ptr<SkyLayer> &layer);

		void createShader(RenderState *rs, const StateConfig &stateCfg);

		void traverse(RenderState *rs) override;

	protected:
		ref_ptr<Sky> sky_;
		std::list<ref_ptr<SkyLayerView> > layer_;
	};
}

#endif /* SKY_H_ */
