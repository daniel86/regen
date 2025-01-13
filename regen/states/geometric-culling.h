/*
 * geometric-culling.h
 *
 *  Created on: Oct 17, 2014
 *      Author: daniel
 */

#ifndef GEOMETRIC_CULLING_H_
#define GEOMETRIC_CULLING_H_

#include <regen/states/state-node.h>
#include <regen/shapes/spatial-index.h>

namespace regen {
	class GeometricCulling : public StateNode {
	public:
		GeometricCulling(
				const ref_ptr<Camera> &camera,
				const ref_ptr<SpatialIndex> &spatialIndex,
				std::string_view shapeName);

		~GeometricCulling() override = default;

		void traverse(RenderState *rs) override;

	protected:
		ref_ptr<Camera> camera_;
		ref_ptr<SpatialIndex> spatialIndex_;
		std::string shapeName_;

		GLuint numInstances_;
		ref_ptr<ShaderInput1ui> instanceIDMap_;
		ref_ptr<Mesh> mesh_;

		void updateMeshLOD();
	};
}


#endif /* GEOMETRIC_CULLING_H_ */
