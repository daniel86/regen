#ifndef REGEN_TESSELLATION_H_
#define REGEN_TESSELLATION_H_

#include <vector>

#include <regen/compute/vector.h>

namespace regen {
	/**
	 * A vertex of a triangle face.
	 */
	struct TriangleVertex {
		/**
		 * @param _p The vertex position.
		 * @param _i The vertex index.
		 */
		TriangleVertex(const Vec3f &_p, const uint32_t &_i) : p(_p), i(_i) {}

		TriangleVertex() : i(0) {}

		/** The vertex position. */
		Vec3f p;
		/** The vertex index. */
		uint32_t i;
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

	struct TessellationFace {
		TessellationFace() = default;
		TessellationFace(uint32_t _v1, uint32_t _v2, uint32_t _v3)
				: v1(_v1), v2(_v2), v3(_v3) {}
		uint32_t v1;
		uint32_t v2;
		uint32_t v3;
	};

	/**
	 * Tessellation data.
	 */
	struct Tessellation {
		std::vector<Vec3f> vertices;
		std::vector<TessellationFace> inputFaces;
		std::vector<TessellationFace> outputFaces;
	};

	/**
	 * Tessellate input triangles.
	 * Each tessellation step divides each triangle face in
	 * 4 smaller triangles.
	 * @param lod Number of tessellation steps.
	 * @param tessellation tessellation data.
	 */
	void tessellate(uint32_t lod, Tessellation &tessellation);

	/**
	 * Tessellation input triangles.
	 * Each tessellation step divides each triangle face in
	 * 4 smaller triangles.
	 * @param lod Number of tessellation steps.
	 * @param inputFaces input triangles.
	 * @return tessellated faces.
	 */
	std::vector<TriangleFace> tessellate(uint32_t lod, std::vector<TriangleFace> &inputFaces);

	/**
	 * Tessellation input triangle.
	 * Each tessellation step divides each triangle face in
	 * 4 smaller triangles.
	 * @param lod Number of tessellation steps.
	 * @param inputFace input triangle.
	 * @return tessellated face.
	 */
	std::vector<TriangleFace> tessellate(uint32_t lod, TriangleFace &inputFace);
}

#endif /* TESSELATION_H_ */
