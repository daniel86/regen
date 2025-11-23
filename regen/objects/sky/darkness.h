#ifndef REGEN_SKY_DARKNESS_LAYER_H_
#define REGEN_SKY_DARKNESS_LAYER_H_

#include <regen/objects/sky/sky-layer.h>
#include <regen/objects/sky/sky.h>
#include <regen/objects/sky-box.h>

namespace regen {
	/**
	 * \brief Darkness layer of the sky.
	 * This layer is used to render plain dark color without blending.
	 */
	class Darkness : public SkyLayer {
	public:
		explicit Darkness(const ref_ptr<Sky> &sky, int levelOfDetail = 4);

		ref_ptr<Mesh> getMeshState() override { return meshState_; }

		ref_ptr<HasShader> getShaderState() override { return meshState_; }

	protected:
		ref_ptr<SkyBox> meshState_;
	};
}
#endif /* REGEN_SKY_DARKNESS_LAYER_H_ */
