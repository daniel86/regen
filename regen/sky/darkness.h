#ifndef REGEN_SKY_DARKNESS_LAYER_H_
#define REGEN_SKY_DARKNESS_LAYER_H_

#include <regen/sky/sky-layer.h>
#include <regen/sky/sky.h>
#include <regen/meshes/sky-box.h>

namespace regen {
	/**
	 * \brief Darkness layer of the sky.
	 * This layer is used to render plain dark color without blending.
	 */
	class Darkness : public SkyLayer {
	public:
		explicit Darkness(const ref_ptr<Sky> &sky, GLint levelOfDetail = 4);

		ref_ptr<Mesh> getMeshState() override { return meshState_; }

		ref_ptr<HasShader> getShaderState() override { return meshState_; }

	protected:
		ref_ptr<SkyBox> meshState_;
	};
}
#endif /* REGEN_SKY_DARKNESS_LAYER_H_ */
