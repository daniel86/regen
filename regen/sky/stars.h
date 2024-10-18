/*
 * stars.h
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#ifndef STARS_LAYER_H_
#define STARS_LAYER_H_

#include <regen/sky/sky-layer.h>
#include <regen/sky/sky.h>
#include <regen/gl-types/fbo.h>

namespace regen {
	class Stars : public SkyLayer {
	public:
		explicit Stars(const ref_ptr<Sky> &sky);

		void set_brightStarsFile(const std::string &brightStars);

		void set_apparentMagnitude(GLfloat vMag);

		const ref_ptr<ShaderInput1f> &apparentMagnitude() const;

		void set_color(const Vec3f& color);

		const ref_ptr<ShaderInput3f> &color() const;

		void set_colorRatio(GLfloat ratio);

		const ref_ptr<ShaderInput1f> &colorRatio() const;

		void set_glareIntensity(GLfloat intensity);

		const ref_ptr<ShaderInput1f> &glareIntensity() const;

		void set_glareScale(GLfloat scale);

		const ref_ptr<ShaderInput1f> &glareScale() const;

		void set_scintillation(GLfloat scintillation);

		const ref_ptr<ShaderInput1f> &scintillation() const;

		void set_scattering(GLfloat scattering);

		const ref_ptr<ShaderInput1f> &scattering() const;

		void set_scale(GLfloat scale);

		const ref_ptr<ShaderInput1f> &scale() const;


		static GLfloat defaultApparentMagnitude();

		static Vec3f defaultColor();

		static GLfloat defaultColorRatio();

		static GLfloat defaultGlareScale();

		static GLfloat defaultScintillation();

		static GLfloat defaultScattering();

		// Override
		void updateSkyLayer(RenderState *rs, GLdouble dt) override;

		ref_ptr<Mesh> getMeshState() override;

		ref_ptr<HasShader> getShaderState() override;

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
#endif /* STARS_LAYER_H_ */
