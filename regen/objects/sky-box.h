#ifndef SKY_BOX_H_
#define SKY_BOX_H_

#include <regen/objects/primitives/box.h>
#include <regen/camera/camera.h>
#include <regen/textures/texture-state.h>
#include <regen/shading/light-state.h>
#include "regen/shader/shader-state.h"
#include "regen/textures/fbo.h"

namespace regen {
	/**
	 * \brief A special Box that is not translated by camera movement.
	 */
	class SkyBox : public Box, public HasShader {
	public:
		/**
		 * @param levelOfDetail LoD for Box mesh.
		 */
		explicit SkyBox(uint32_t levelOfDetail = 0, const std::string &shaderKey = "regen.models.sky-box");

		/**
		 * @return the cube map texture.
		 */
		const ref_ptr<TextureCube> &cubeMap() const;

		/**
		 * @param cubeMap the cube map texture.
		 */
		void setCubeMap(const ref_ptr<TextureCube> &cubeMap);

	protected:
		ref_ptr<TextureState> texState_;
		ref_ptr<TextureCube> cubeMap_;
	};
} // namespace

#endif /* SKY_BOX_H_ */
