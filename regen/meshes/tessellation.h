/*
 * tesselation.h
 *
 *  Created on: Dec 27, 2013
 *      Author: daniel
 */

#ifndef TESSELLATION_H_
#define TESSELLATION_H_

#include <vector>

#include <regen/math/vector.h>

namespace regen {
	/**
	 * A vertex of a triangle face.
	 */
	struct TriangleVertex {
		/**
		 * @param _p The vertex position.
		 * @param _i The vertex index.
		 */
		TriangleVertex(const Vec3f &_p, const GLuint &_i) : p(_p), i(_i) {}

		TriangleVertex() : i(0) {}

		/** The vertex position. */
		Vec3f p;
		/** The vertex index. */
		GLuint i;
	};

	/**
	 * A face of a triangle mesh.
	 */
	struct TriangleFace {
		/**
		 * @param _v1 First face vertex.
		 * @param _v2 Second face vertex.
		 * @param _v3 Third face vertex.
		 */
		TriangleFace(
				const TriangleVertex &_v1,
				const TriangleVertex &_v2,
				const TriangleVertex &_v3)
				: v1(_v1), v2(_v2), v3(_v3) {}

		TriangleFace() = default;

		/** First face vertex. */
		TriangleVertex v1;
		/** Second face vertex. */
		TriangleVertex v2;
		/** Third face vertex. */
		TriangleVertex v3;
	};

	/**
	 * Tessellation input triangles.
	 * Each tessellation step divides each triangle face in
	 * 4 smaller triangles.
	 * @param lod Number of tessellation steps.
	 * @param inputFaces input triangles.
	 * @return tessellated faces.
	 */
	std::vector<TriangleFace> tessellate(GLuint lod, std::vector<TriangleFace> &inputFaces);

	/**
	 * Tessellation input triangle.
	 * Each tessellation step divides each triangle face in
	 * 4 smaller triangles.
	 * @param lod Number of tessellation steps.
	 * @param inputFace input triangle.
	 * @return tessellated face.
	 */
	std::vector<TriangleFace> tessellate(GLuint lod, TriangleFace &inputFace);
}

#endif /* TESSELATION_H_ */
