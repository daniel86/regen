/*
 * stars.h
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#ifndef REGEN_STARS_LAYER_H_
#define REGEN_STARS_LAYER_H_

#include <regen/sky/sky-layer.h>
#include <regen/sky/sky.h>
#include <regen/gl-types/fbo.h>

namespace regen {
	/**
	 * \brief A sky layer of bright stars.
	 */
	class Stars : public SkyLayer {
	public:
		explicit Stars(const ref_ptr<Sky> &sky);

		void set_brightStarsFile(const std::string &brightStars);

		void set_apparentMagnitude(float vMag) { apparentMagnitude_->setVertex(0, vMag); }

		const ref_ptr<ShaderInput1f> &apparentMagnitude() const { return apparentMagnitude_; }

		void set_color(const Vec3f& color) { color_->setVertex(0, color); }

		const ref_ptr<ShaderInput3f> &color() const { return color_; }

		void set_colorRatio(float ratio) { colorRatio_->setVertex(0, ratio); }

		const ref_ptr<ShaderInput1f> &colorRatio() const { return colorRatio_; }

		void set_glareIntensity(float intensity) { glareIntensity_->setVertex(0, intensity); }

		const ref_ptr<ShaderInput1f> &glareIntensity() const { return glareIntensity_; }

		void set_glareScale(float scale) { glareScale_->setVertex(0, scale); }

		const ref_ptr<ShaderInput1f> &glareScale() const { return glareScale_; }

		void set_scintillation(float scintillation) { scintillation_->setVertex(0, scintillation); }

		const ref_ptr<ShaderInput1f> &scintillation() const { return scintillation_; }

		void set_scattering(float scattering) { scattering_->setVertex(0, scattering); }

		const ref_ptr<ShaderInput1f> &scattering() const { return scattering_; }

		void set_scale(float scale) { scale_->setVertex(0, scale); }

		const ref_ptr<ShaderInput1f> &scale() const { return scale_; }

		static float defaultApparentMagnitude();

		static Vec3f defaultColor();

		static float defaultColorRatio();

		static float defaultGlareScale();

		static float defaultScintillation();

		static float defaultScattering();

		ref_ptr<Mesh> getMeshState() override { return meshState_; }

		ref_ptr<HasShader> getShaderState() override { return shaderState_; }

	protected:
		ref_ptr<Mesh> meshState_;
		ref_ptr<ShaderInput4f> pos_;
		ref_ptr<ShaderInput4f> col_;

		ref_ptr<HasShader> shaderState_;

		ref_ptr<ShaderInput1f> apparentMagnitude_;
		ref_ptr<ShaderInput3f> color_;
		ref_ptr<ShaderInput1f> colorRatio_;
		ref_ptr<ShaderInput1f> glareIntensity_;
		ref_ptr<ShaderInput1f> glareScale_;
		ref_ptr<ShaderInput1f> scintillation_;
		ref_ptr<ShaderInput1f> scattering_;
		ref_ptr<ShaderInput1f> scale_;
		ref_ptr<TextureState> noiseTexState_;
		ref_ptr<Texture1D> noiseTex_;

		ref_ptr<Texture> noise1_;

		void updateNoiseTexture();
	};
}
#endif /* REGEN_STARS_LAYER_H_ */
