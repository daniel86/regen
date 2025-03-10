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
#include "regen/camera/sorting.h"

namespace regen {
	/**
	 * @brief Geometric culling state node
	 */
	class GeometricCulling : public StateNode {
	public:
		/**
		 * @brief Construct a new Geometric Culling object
		 * @param camera The camera
		 * @param spatialIndex The spatial index
		 * @param shapeName The shape name
		 */
		GeometricCulling(
				const ref_ptr<Camera> &camera,
				const ref_ptr<SpatialIndex> &spatialIndex,
				std::string_view shapeName);

		~GeometricCulling() override = default;

		/**
		 * @brief Set the instance sorting
		 * @param instanceSorting The instance sorting
		 */
		void setInstanceSortMode(SortMode mode) { instanceSortMode_ = mode; }

		void traverse(RenderState *rs) override;

	protected:
		ref_ptr<Camera> camera_;
		ref_ptr<SpatialIndex> spatialIndex_;
		ref_ptr<IndexedShape> shapeIndex_;
		SortMode instanceSortMode_ = SortMode::FRONT_TO_BACK;

		GLuint numInstances_ = 1;
		ref_ptr<ShaderInput1ui> instanceIDMap_;
		ref_ptr<Mesh> mesh_;
		std::vector<std::vector<GLuint>> lodGroups_;

		void updateMeshLOD();

		void computeLODGroups();

		void traverseInstanced_(RenderState *rs, unsigned int numVisible);

		void traverseInstanced1(RenderState *rs);

		void traverseInstanced2(RenderState *rs, const unsigned int *visibleInstances, unsigned int numVisible);
	};
}


#endif /* GEOMETRIC_CULLING_H_ */
