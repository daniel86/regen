/*
 * star-map-layer.h
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#ifndef STAR_MAP_LAYER_H_
#define STAR_MAP_LAYER_H_

#include <regen/sky/sky-layer.h>
#include <regen/sky/sky.h>
#include <regen/meshes/sky-box.h>
#include <regen/gl-types/fbo.h>

namespace regen {
	class StarMap : public SkyLayer {
	public:
		explicit StarMap(const ref_ptr<Sky> &sky, GLint levelOfDetail = 4);

		void set_texture(const std::string &textureFile);

		void set_apparentMagnitude(GLdouble apparentMagnitude);

		void set_deltaMagnitude(GLdouble deltaMagnitude);

		void set_scattering(GLdouble scattering);

		const ref_ptr<ShaderInput1f> &scattering() const;

		static GLdouble defaultScattering();

		// Override
		void updateSkyLayer(RenderState *rs, GLdouble dt) override;

		ref_ptr<Mesh> getMeshState() override;

		ref_ptr<HasShader> getShaderState() override;

	protected:
		ref_ptr<SkyBox> meshState_;

		ref_ptr<ShaderInput1f> scattering_;
		ref_ptr<ShaderInput1f> deltaM_;
	};
}
#endif /* STAR_MAP_LAYER_H_ */
