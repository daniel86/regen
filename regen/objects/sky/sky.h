#ifndef REGEN_SKY_H_
#define REGEN_SKY_H_

#include <list>

#include <regen/camera/camera.h>
#include <regen/shading/light-state.h>
#include <regen/objects/sky/sky-layer.h>
#include <regen/objects/primitives/rectangle.h>

#include <regen/external/osghimmel/timef.h>
#include "regen/utility/time.h"
#include "astronomy.h"
#include "regen/gl/states/depth-state.h"

namespace regen {
	class Sky : public StateNode, public Animation, public Resource {
	public:
		static constexpr const char *TYPE_NAME = "Sky";

		Sky(const ref_ptr<Camera> &cam,
			const ref_ptr<Screen> &screen);

		static ref_ptr<Sky> load(LoadingContext &ctx, scene::SceneInputNode &input);

		void setWorldTime(const WorldTime *worldTime) { worldTime_ = worldTime; }

		auto &worldTime() { return worldTime_; }

		void set_altitude(float altitude);

		double altitude() const;

		void set_longitude(float longitude);

		double longitude() const;

		void set_latitude(float latitude);

		double latitude() const;

		ref_ptr<Light> &sun() { return sun_; }

		ref_ptr<Light> &moon() { return moon_; }

		ref_ptr<Camera> &camera() { return cam_; }

		ref_ptr<Screen> &screen() { return screen_; }

		const Vec3f &noonColor() const { return noonColor_; }

		void set_noonColor(const Vec3f &noonColor) { noonColor_ = noonColor; }

		const Vec3f &dawnColor() const { return dawnColor_; }

		void set_dawnColor(const Vec3f &dawnColor) { dawnColor_ = dawnColor; }

		float moonSunLightReflectance() const { return moonSunLightReflectance_; }

		void set_moonSunLightReflectance(float moonSunLightReflectance);

		static float computeHorizonExtinction(const Vec3f &position, const Vec3f &dir, float radius);

		float computeEyeExtinction(const Vec3f &eyedir, float planetRadius);

		const ref_ptr<Rectangle> &skyQuad() const { return skyQuad_; }

		Astronomy &astro();

		void set_astro(const ref_ptr<Astronomy> &astro) { astro_ = astro; }

		void addLayer(const ref_ptr<SkyLayer> &layer);

		auto &layer() const { return layer_; }

		void createShader();

		// override
		void cpuUpdate(double dt) override;

		void gpuUpdate(RenderState *rs, double dt) override;

		void startAnimation() override;

		void stopAnimation() override;

	protected:
		friend class SkyView;

		ref_ptr<Camera> cam_;
		ref_ptr<Screen> screen_;
		uint32_t camStamp_ = 0;
		uint32_t viewportStamp_ = 0;

		const WorldTime *worldTime_ = nullptr;
		osgHimmel::TimeF time_osg_;
		ref_ptr<Astronomy> astro_;

		std::list<ref_ptr<SkyLayer> > layer_;
		ref_ptr<DepthState> depth_;

		ref_ptr<Light> sun_;
		ref_ptr<Light> moon_;
		ref_ptr<ShaderInput4f> cmnUniform_;
		ref_ptr<ShaderInputMat4> R_;
		ref_ptr<ShaderInput1f> q_;
		ref_ptr<ShaderInput1f> sqrt_q_;
		ref_ptr<Rectangle> skyQuad_;

		Vec3f noonColor_;
		Vec3f dawnColor_;
		float moonSunLightReflectance_;

		void updateSeed();
	};

	class SkyView : public StateNode {
	public:
		explicit SkyView(const ref_ptr<Sky> &sky);

		static ref_ptr<SkyView> load(LoadingContext &ctx, scene::SceneInputNode &input);

		const ref_ptr<Sky> &sky() const { return sky_; }

		void addLayer(const ref_ptr<SkyLayer> &layer);

		void createShader(RenderState *rs, const StateConfig &stateCfg);

		void traverse(RenderState *rs) override;

	protected:
		ref_ptr<Sky> sky_;
		std::vector<ref_ptr<SkyLayerView> > layer_;
	};
}

#endif /* REGEN_SKY_H_ */
