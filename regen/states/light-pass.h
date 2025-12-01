#ifndef REGEN_LIGHT_PASS_H_
#define REGEN_LIGHT_PASS_H_

#include <regen/scene/state.h>
#include "regen/shader/shader-state.h"
#include <regen/objects/mesh.h>
#include <regen/camera/light-camera.h>

namespace regen {
	/**
	 * \brief Deferred shading pass.
	 */
	class LightPass : public State {
	public:
		/**
		 * @param type the light type.
		 * @param shaderKey the shader key to include.
		 */
		LightPass(Light::Type type, std::string_view shaderKey);

		static ref_ptr<LightPass> load(LoadingContext &ctx, scene::SceneInputNode &input);

		/**
		 * @param cfg the shader configuration.
		 */
		void createShader(const StateConfig &cfg);

		/**
		 * Adds a light to the rendering pass.
		 * @param light the light.
		 * @param lightCamera Light-perspective Camera or null reference.
		 * @param shadowTexture ShadowMap or null reference.
		 * @param shadowColorTexture Color-ShadowMap or null reference.
		 * @param inputs render pass inputs.
		 */
		void addLight(
				const ref_ptr<Light> &light,
				const ref_ptr<LightCamera> &lightCamera,
				const ref_ptr<Texture> &shadowTexture,
				const ref_ptr<Texture> &shadowColorTexture,
				const std::list<ref_ptr<ShaderInput> > &inputs);

		/**
		 * @param l a previously added light.
		 */
		void removeLight(Light *l);

		/**
		 * @return true if no light was added yet.
		 */
		bool empty() const;

		/**
		 * @param l a light.
		 * @return true if the light was previously added.
		 */
		bool hasLight(Light *l) const;

		/**
		 * @return the lights.
		 */
		auto &lights() const { return lights_; }

		/**
		 * @param mode the shadow filtering mode.
		 */
		void setShadowFiltering(ShadowFilterMode mode);

		/**
		 * Enable or disable ambient light computation in this pass.
		 * @param useAmbient true to use ambient light, false to disable it.
		 */
		void setUseAmbient(bool useAmbient);

		// override
		void enable(RenderState *rs) override;

	protected:
		struct LightPassLight {
			ref_ptr<Light> light;
			ref_ptr<LightCamera> camera;
			ref_ptr<Texture> shadow;
			ref_ptr<Texture> shadowColor;
			std::list<ref_ptr<ShaderInput> > inputs;
			std::list<InputLocation> inputLocations;
		};

		Light::Type lightType_;
		const std::string shaderKey_;

		ref_ptr<Mesh> mesh_;
		ref_ptr<ShaderState> shader_;

		std::list<LightPassLight> lights_;
		std::map<Light *, std::list<LightPassLight>::iterator> lightIterators_;

		int shadowMapLoc_ = -1;
		int shadowColorLoc_ = -1;
		ShadowFilterMode shadowFiltering_ = SHADOW_FILTERING_NONE;
		uint32_t numShadowLayer_ = 1;
		uint32_t numInstances_ = 1;

		void addInputLocation(LightPassLight &l,
							  const ref_ptr<ShaderInput> &in, const std::string &name);

		void addLightInput(LightPassLight &light);
	};
} // namespace

#endif /* REGEN_LIGHT_PASS_H_ */
