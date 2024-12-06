
#ifndef REGEN_MESH_LOD_H_
#define REGEN_MESH_LOD_H_

#include <regen/states/atomic-states.h>
#include "regen/scene/resources/mesh.h"

namespace regen {
	/**
	 * \brief Describes how the LOD level of a mesh is determined.
	 */
	enum LODMetric {
		LOD_CAMERA_DISTANCE
	};
	enum LODLevel {
		LOD_HIGH	= 0,
		LOD_MEDIUM	= 1,
		LOD_LOW		= 2
	};

	/**
	 * \brief Manages LOD of a mesh.
	 */
	class MeshLOD : public State {
	public:
		MeshLOD(const ref_ptr<Mesh> &mesh, const ref_ptr<ModelTransformation> &meshTransform);

		~MeshLOD() override = default;

		/**
		 * @return the LOD level.
		 */
		virtual LODLevel lodLevel() = 0;

		void enable(RenderState *rs) override;
		void disable(RenderState *rs) override;

	protected:
		LODLevel currentLODLevel_;
		ref_ptr<Mesh> mesh_;
		ref_ptr<ModelTransformation> meshTransform_;
	};

	/**
	 * \brief Changes LOD of a mesh based on distance to camera.
	 */
	class CameraDistanceLOD : public MeshLOD {
	public:
		CameraDistanceLOD(const ref_ptr<Mesh> &mesh,
				const ref_ptr<ModelTransformation> &meshTransform,
				const ref_ptr<Camera> &camera);

		~CameraDistanceLOD() override = default;

		LODLevel lodLevel() override;

		/**
		 * @param closeDistance the distance below which the LOD level is LOD_HIGH.
		 */
		void set_closeDistance(GLfloat closeDistance) { closeDistance_ = closeDistance; }

		/**
		 * @param mediumDistance the distance below which the LOD level is at least LOD_MEDIUM.
		 */
		void set_mediumDistance(GLfloat mediumDistance) { mediumDistance_ = mediumDistance; }

	protected:
		ref_ptr<Camera> camera_;
		GLuint camStamp_;
		GLuint transformStamp_;
		GLfloat closeDistance_;
		GLfloat mediumDistance_;
	};

	std::ostream &operator<<(std::ostream &out, const LODMetric &v);

	std::istream &operator>>(std::istream &in, LODMetric &v);

} // namespace

#endif /* REGEN_MESH_LOD_H_ */
