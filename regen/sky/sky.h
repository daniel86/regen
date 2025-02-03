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
#include <regen/meshes/primitives/rectangle.h>

#include <regen/external/osghimmel/timef.h>
#include <regen/external/osghimmel/astronomy.h>

namespace regen {

	class Sky : public StateNode, public Animation {
	public:
		Sky(const ref_ptr<Camera> &cam,
			const ref_ptr<ShaderInput2i> &viewport);

		void setWorldTime(const boost::posix_time::ptime *worldTime) { worldTime_ = worldTime; }

		void set_altitude(GLdouble altitude);

		GLdouble altitude() const;

		void set_longitude(GLdouble longitude);

		GLdouble longitude() const;

		void set_latitude(GLdouble latitude);

		GLdouble latitude() const;

		ref_ptr<Light> &sun() { return sun_; }

		ref_ptr<Light> &moon() { return moon_; }

		ref_ptr<Camera> &camera() { return cam_; }

		ref_ptr<ShaderInput2i> &viewport() { return viewport_; }

		const Vec3f &noonColor() const { return noonColor_; }

		void set_noonColor(const Vec3f &noonColor) { noonColor_ = noonColor; }

		const Vec3f &dawnColor() const { return dawnColor_; }

		void set_dawnColor(const Vec3f &dawnColor) { dawnColor_ = dawnColor; }

		GLfloat moonSunLightReflectance() const { return moonSunLightReflectance_; }

		void set_moonSunLightReflectance(GLfloat moonSunLightReflectance);

		GLfloat computeHorizonExtinction(const Vec3f& position, const Vec3f& dir, GLfloat radius);

		GLfloat computeEyeExtinction(const Vec3f& eyedir);

		const ref_ptr<Rectangle> &skyQuad() const { return skyQuad_; }

		osgHimmel::AbstractAstronomy &astro();

		void set_astro(const ref_ptr<osgHimmel::AbstractAstronomy> &astro) { astro_ = astro; }

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

		const boost::posix_time::ptime *worldTime_ = nullptr;
		ref_ptr<osgHimmel::AbstractAstronomy> astro_;

		std::list<ref_ptr<SkyLayer> > layer_;

		ref_ptr<Light> sun_;
		ref_ptr<Light> moon_;
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

		const ref_ptr<Sky> &sky() const { return sky_; }

		void addLayer(const ref_ptr<SkyLayer> &layer);

		void createShader(RenderState *rs, const StateConfig &stateCfg);

		void traverse(RenderState *rs) override;

	protected:
		ref_ptr<Sky> sky_;
		std::list<ref_ptr<SkyLayerView> > layer_;
	};
}

#endif /* SKY_H_ */
