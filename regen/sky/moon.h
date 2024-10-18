/*
 * moon.h
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#ifndef MOON_H_
#define MOON_H_

#include <regen/sky/sky-layer.h>
#include <regen/sky/sky.h>
#include <regen/gl-types/fbo.h>

namespace regen {
	class MoonLayer : public SkyLayer {
	public:
		MoonLayer(const ref_ptr<Sky> &sky, const std::string &moonMapFile);

		void set_scale(GLdouble scale);

		const ref_ptr<ShaderInput1f> &scale() const;

		void set_scattering(GLdouble scattering);

		const ref_ptr<ShaderInput1f> &scattering() const;

		void set_sunShineColor(const Vec3f &color);

		void set_sunShineIntensity(GLdouble sunShineIntensity);

		const ref_ptr<ShaderInput4f> &sunShine() const;

		void set_earthShineColor(const Vec3f &color);

		void set_earthShineIntensity(GLdouble sunShineIntensity);

		const ref_ptr<ShaderInput3f> &earthShine() const;

		GLdouble defaultScale();

		GLdouble defaultScattering();

		Vec3f defaultSunShineColor();

		GLdouble defaultSunShineIntensity();

		Vec3f defaultEarthShineColor();

		GLdouble defaultEarthShineIntensity();

		// Override
		virtual void updateSkyLayer(RenderState *rs, GLdouble dt);

		ref_ptr<Mesh> getMeshState();

		ref_ptr<HasShader> getShaderState();

	protected:
		ref_ptr<Mesh> meshState_;
		ref_ptr<HasShader> shaderState_;

		ref_ptr<ShaderInput1f> scale_;
		ref_ptr<ShaderInput1f> scattering_;
		ref_ptr<ShaderInput4f> sunShine_;
		ref_ptr<ShaderInput3f> earthShine_;
		Vec3f earthShineColor_;
		GLdouble earthShineIntensity_;

		ref_ptr<ShaderInputMat4> moonOrientation_;

		void setupMoonTextureCube(const std::string &cubeMapFilePath);
	};
}
#endif /* MOON_H_ */
