/*
 * cloud-layer.h
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#ifndef REGEN_CLOUD_LAYER_H_
#define REGEN_CLOUD_LAYER_H_

#include <regen/sky/sky-layer.h>
#include <regen/sky/sky.h>
#include <regen/gl-types/fbo.h>
#include <regen/textures/texture-3d.h>
#include <regen/textures/texture-2d.h>

namespace regen {
	class CloudLayer : public SkyLayer {
	public:
		explicit CloudLayer(const ref_ptr<Sky> &sky, GLuint textureSize = 2048);

		void set_altitude(float altitude) { altitude_->setVertex(0, altitude); }

		const ref_ptr<ShaderInput1f> &altitude() const { return altitude_; }

		void set_sharpness(float sharpness) { sharpness_->setVertex(0, sharpness); }

		const ref_ptr<ShaderInput1f> &sharpness() const { return sharpness_; }

		void set_coverage(float coverage) { coverage_->setVertex(0, coverage); }

		const ref_ptr<ShaderInput1f> &coverage() const { return coverage_; }

		void set_scale(const Vec2f &scale) { scale_->setVertex(0, scale); }

		const ref_ptr<ShaderInput2f> &scale() const { return scale_; }

		void set_change(float change) { change_->setVertex(0, change); }

		const ref_ptr<ShaderInput1f> &change() const { return change_; }

		void set_wind(const Vec2f &wind) { wind_->setVertex(0, wind); }

		const ref_ptr<ShaderInput2f> &wind() const { return wind_; }

		void set_color(const Vec3f &color) { color_->setVertex(0, color); }

		void set_thickness(float thickness) { thickness_->setVertex(0, thickness); }

		const ref_ptr<ShaderInput1f> &thickness() const { return thickness_; }

		void set_offset(float offset) { offset_->setVertex(0, offset); }

		const ref_ptr<ShaderInput1f> &offset() const { return offset_; }

		const ref_ptr<ShaderInput3f> &color() const { return color_; }

		void set_bottomColor(const Vec3f &color) { bottomColor_->setVertex(0, color); }

		const ref_ptr<ShaderInput3f> &bottomColor() const { return bottomColor_; }

		void set_topColor(const Vec3f &color) { topColor_->setVertex(0, color); }

		const ref_ptr<ShaderInput3f> &topColor() const { return topColor_; }

		const ref_ptr<Texture2D> &cloudTexture() const { return cloudTexture_; }

		const ref_ptr<FBO> &cloudTextureFBO() const { return fbo_; }

		static float defaultAltitudeHigh();

		static float defaultAltitudeLow();

		static Vec2f defaultScaleHigh();

		static Vec2f defaultScaleLow();

		static float defaultChangeHigh();

		static float defaultChangeLow();

		// Override
		void updateSkyLayer(RenderState *rs, GLdouble dt) override;

		// Override SkyLayer
		void createUpdateShader() override;

		ref_ptr<Mesh> getMeshState() override { return meshState_; }

		ref_ptr<HasShader> getShaderState() override { return shaderState_; }

	protected:
		ref_ptr<Mesh> meshState_;
		ref_ptr<Mesh> updateMesh_;
		ref_ptr<HasShader> shaderState_;

		ref_ptr<ShaderState> updateShader_;

		ref_ptr<UniformBlock> cloudUniforms_;
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
#endif /* REGEN_CLOUD_LAYER_H_ */
