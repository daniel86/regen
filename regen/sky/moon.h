/*
 * moon.h
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#ifndef REGEN_MOON_H_
#define REGEN_MOON_H_

#include <regen/sky/sky-layer.h>
#include <regen/sky/sky.h>
#include <regen/gl-types/fbo.h>

namespace regen {
	/**
	 * \brief Moon layer of the sky.
	 * The moon is rendered using a textured quad with alpha blending.
	 * As moon texture, a cube map is used.
	 */
	class MoonLayer : public SkyLayer {
	public:
		MoonLayer(const ref_ptr<Sky> &sky, const std::string &moonMapFile);

		void set_scale(float scale) { scale_->setVertex(0, scale); }

		const ref_ptr<ShaderInput1f> &scale() const { return scale_; }

		void set_scattering(float scattering) { scattering_->setVertex(0, scattering); }

		const ref_ptr<ShaderInput1f> &scattering() const { return scattering_; }

		void set_sunShineColor(const Vec3f &color);

		void set_sunShineIntensity(float sunShineIntensity);

		const ref_ptr<ShaderInput4f> &sunShine() const { return sunShine_; }

		void set_earthShineColor(const Vec3f &color) { earthShineColor_ = color; }

		void set_earthShineIntensity(float sunShineIntensity) { earthShineIntensity_ = sunShineIntensity; }

		const ref_ptr<ShaderInput3f> &earthShine() const { return earthShine_; }

		static float defaultScale();

		static float defaultScattering();

		static Vec3f defaultSunShineColor();

		static float defaultSunShineIntensity();

		static Vec3f defaultEarthShineColor();

		static float defaultEarthShineIntensity();

		// Override
		void updateSkyLayer(RenderState *rs, GLdouble dt) override;

		ref_ptr<Mesh> getMeshState() override { return meshState_; }

		ref_ptr<HasShader> getShaderState() override { return shaderState_; }

	protected:
		ref_ptr<Mesh> meshState_;
		ref_ptr<HasShader> shaderState_;

		ref_ptr<UniformBlock> moonUniforms_;
		ref_ptr<ShaderInput1f> scale_;
		ref_ptr<ShaderInput1f> scattering_;
		ref_ptr<ShaderInput4f> sunShine_;
		ref_ptr<ShaderInput3f> earthShine_;
		Vec3f earthShineColor_;
		float earthShineIntensity_;

		ref_ptr<ShaderInputMat4> moonOrientation_;

		void setupMoonTextureCube(const std::string &cubeMapFilePath);
	};
}
#endif /* REGEN_MOON_H_ */
