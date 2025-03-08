/*
 * light-pass.h
 *
 *  Created on: 13.03.2013
 *      Author: daniel
 */

#ifndef __LIGHT_PASS_H_
#define __LIGHT_PASS_H_

#include <regen/states/state.h>
#include <regen/states/shader-state.h>
#include <regen/meshes/mesh-state.h>
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
		LightPass(Light::Type type, const std::string &shaderKey);

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
		GLboolean empty() const;

		/**
		 * @param l a light.
		 * @return true if the light was previously added.
		 */
		GLboolean hasLight(Light *l) const;

		/**
		 * @return the lights.
		 */
		auto &lights() { return lights_; }

		/**
		 * @param mode the shadow filtering mode.
		 */
		void setShadowFiltering(ShadowFilterMode mode);

		// override
		void enable(RenderState *rs) override;

	protected:
		struct LightPassLight {
			ref_ptr<Light> light;
			ref_ptr<LightCamera> camera;
			ref_ptr<Texture> shadow;
			ref_ptr<Texture> shadowColor;
			std::list<ref_ptr<ShaderInput> > inputs;
			std::list<ShaderInputLocation> inputLocations;
		};

		Light::Type lightType_;
		const std::string shaderKey_;

		ref_ptr<Mesh> mesh_;
		ref_ptr<ShaderState> shader_;

		std::list<LightPassLight> lights_;
		std::map<Light *, std::list<LightPassLight>::iterator> lightIterators_;

		GLint shadowMapLoc_ = -1;
		GLint shadowColorLoc_ = -1;
		ShadowFilterMode shadowFiltering_ = SHADOW_FILTERING_NONE;
		GLuint numShadowLayer_ = 1;

		void addInputLocation(LightPassLight &l,
							  const ref_ptr<ShaderInput> &in, const std::string &name);

		void addLightInput(LightPassLight &light);
	};
} // namespace

#endif /* __LIGHT_PASS_H_ */
