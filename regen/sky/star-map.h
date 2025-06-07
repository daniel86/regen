#ifndef REGEN_STAR_MAP_LAYER_H_
#define REGEN_STAR_MAP_LAYER_H_

#include <regen/sky/sky-layer.h>
#include <regen/sky/sky.h>
#include <regen/meshes/sky-box.h>
#include <regen/gl-types/fbo.h>

namespace regen {
	/**
	 * \brief A sky layer of far away stars.
	 * A textured sky box is used to render the stars.
	 */
	class StarMap : public SkyLayer {
	public:
		explicit StarMap(const ref_ptr<Sky> &sky, GLint levelOfDetail = 4);

		void set_texture(const std::string &textureFile);

		void set_apparentMagnitude(float apparentMagnitude);

		void set_deltaMagnitude(float deltaMagnitude);

		void set_scattering(float scattering) { scattering_->setUniformData(scattering); }

		const ref_ptr<ShaderInput1f> &scattering() const { return scattering_; }

		static float defaultScattering();

		ref_ptr<Mesh> getMeshState() override { return meshState_; }

		ref_ptr<HasShader> getShaderState() override { return meshState_; }

	protected:
		ref_ptr<SkyBox> meshState_;

		ref_ptr<ShaderInput1f> scattering_;
		ref_ptr<ShaderInput1f> deltaM_;
	};
}
#endif /* REGEN_STAR_MAP_LAYER_H_ */
