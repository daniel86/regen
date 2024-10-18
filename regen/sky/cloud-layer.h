/*
 * cloud-layer.h
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#ifndef CLOUD_LAYER_H_
#define CLOUD_LAYER_H_

#include <regen/sky/sky-layer.h>
#include <regen/sky/sky.h>
#include <regen/gl-types/fbo.h>

namespace regen {
	class CloudLayer : public SkyLayer {
	public:
		CloudLayer(const ref_ptr<Sky> &sky, GLuint textureSize = 2048);

		void set_altitude(GLdouble altitude);

		const ref_ptr<ShaderInput1f> &altitude() const;

		void set_sharpness(GLdouble sharpness);

		const ref_ptr<ShaderInput1f> &sharpness() const;

		void set_coverage(GLdouble coverage);

		const ref_ptr<ShaderInput1f> &coverage() const;

		void set_scale(const Vec2f &scale);

		const ref_ptr<ShaderInput2f> &scale() const;

		void set_change(GLdouble change);

		const ref_ptr<ShaderInput1f> &change() const;

		void set_wind(const Vec2f &wind);

		const ref_ptr<ShaderInput2f> &wind() const;

		void set_color(const Vec3f &color);

		void set_thickness(GLdouble thickness);

		const ref_ptr<ShaderInput1f> &thickness() const;

		void set_offset(GLdouble offset);

		const ref_ptr<ShaderInput1f> &offset() const;

		const ref_ptr<ShaderInput3f> &color() const;

		void set_bottomColor(const Vec3f &color);

		const ref_ptr<ShaderInput3f> &bottomColor() const;

		void set_topColor(const Vec3f &color);

		const ref_ptr<ShaderInput3f> &topColor() const;

		const ref_ptr<Texture2D> &cloudTexture() const;

		const ref_ptr<FBO> &cloudTextureFBO() const;

		const float defaultAltitudeHigh();

		const float defaultAltitudeLow();

		const Vec2f defaultScaleHigh();

		const Vec2f defaultScaleLow();

		GLdouble defaultChangeHigh();

		GLdouble defaultChangeLow();

		// Override
		virtual void updateSkyLayer(RenderState *rs, GLdouble dt);

		ref_ptr<Mesh> getMeshState();

		ref_ptr<HasShader> getShaderState();

	protected:
		ref_ptr<Mesh> meshState_;
		ref_ptr<HasShader> shaderState_;

		ref_ptr<State> updateState_;
		ref_ptr<ShaderState> updateShader_;

		ref_ptr<ShaderInput1f> offset_;
		ref_ptr<ShaderInput1f> thickness_;
		ref_ptr<ShaderInput3f> topColor_;
		ref_ptr<ShaderInput3f> bottomColor_;
		ref_ptr<ShaderInput1f> coverage_;
		ref_ptr<ShaderInput1f> sharpness_;
		ref_ptr<ShaderInput1f> change_;
		ref_ptr<ShaderInput2f> wind_;
		ref_ptr<ShaderInput1f> altitude_;
		ref_ptr<ShaderInput2f> scale_;
		ref_ptr<ShaderInput3f> color_;

		ref_ptr<FBO> fbo_;
		ref_ptr<Texture2D> cloudTexture_;
		ref_ptr<Texture3D> noise0_;
		ref_ptr<Texture3D> noise1_;
		ref_ptr<Texture3D> noise2_;
		ref_ptr<Texture3D> noise3_;
	};
}
#endif /* CLOUD_LAYER_H_ */
