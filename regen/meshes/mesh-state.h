/*
 * mesh-state.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef REGEN_MESH_STATE_H_
#define REGEN_MESH_STATE_H_

#include <vector>

#include <regen/states/state.h>
#include <regen/states/state-config.h>
#include <regen/states/feedback-state.h>
#include <regen/gl-types/shader-input-container.h>
#include <regen/gl-types/vbo.h>
#include <regen/gl-types/vao.h>
#include <regen/gl-types/shader.h>
#include <regen/animations/animation.h>
#include "regen/physics/physical-object.h"

namespace regen {
	/**
	 * \brief A collection of vertices, edges and faces that defines the shape of an object in 3D space.
	 *
	 * When this State is enabled the actual draw call is done. Make sure to setup shader
	 * and server side states before.
	 */
	class Mesh : public State, public HasInput {
	public:
		static constexpr const char *TYPE_NAME = "Mesh";

		struct MeshLOD {
			GLuint numVertices;
			GLuint vertexOffset;
			GLuint numIndices;
			GLuint indexOffset;
		};

		/**
		 * Shallow copy constructor.
		 * Vertex data is not copied.
		 * @param meshResource another mesh that provides vertex data.
		 */
		explicit Mesh(const ref_ptr<Mesh> &meshResource);

		/**
		 * @param primitive Specifies what kind of primitives to render.
		 * @param usage VBO usage.
		 */
		Mesh(GLenum primitive, VBO::Usage usage);

		~Mesh() override;

		/**
		 * @param out Set of meshes using the ShaderInputcontainer of this mesh
		 *          (meshes created by copy constructor).
		 */
		void getMeshViews(std::set<Mesh *> &out);

		/**
		 * Update VAO that is used to render from array data.
		 * And setup uniforms and textures not handled in Shader class.
		 * Basically all uniforms and textures declared as parent nodes of
		 * a Shader instance are auto-enabled by that Shader. All remaining uniforms
		 * and textures are activated in Mesh::enable.
		 * @param rs the render state.
		 * @param cfg the state configuration.
		 * @param shader the mesh shader.
		 */
		void updateVAO(
				RenderState *rs,
				const StateConfig &cfg,
				const ref_ptr<Shader> &shader);

		/**
		 * Update VAO using last StateConfig.enable.
		 * @param rs the render state.
		 */
		void updateVAO(RenderState *rs);

		/**
		 * Update the level of detail based on camera distance.
		 * @param cameraDistance distance to camera.
		 */
		void updateLOD(float cameraDistance);

		/**
		 * Get the level of detail based on camera distance.
		 * @param cameraDistance distance to camera.
		 * @return the level of detail.
		 */
		unsigned int getLODLevel(float cameraDistance);

		/**
		 * Activate given LOD level.
		 */
		void activateLOD(GLuint lodLevel);

		/**
		 * @return the current LOD level.
		 */
		auto lodLevel() const { return lodLevel_; }

		/**
		 * Set the far distance for LOD.
		 */
		void setLODFar(float far) { lodFar_ = far; }

		/**
		 * All LODs are stored in the same buffer, so each LOD level is simply expressed
		 * as offset into the buffer.
		 * @return set of LODs of this mesh.
		 */
		auto &meshLODs() const { return meshLODs_; }

		/**
		 * Set the LODs of this mesh.
		 */
		void setMeshLODs(const std::vector<MeshLOD> &meshLODs) { meshLODs_ = meshLODs; }

		/**
		 * @return number of LODs.
		 */
		auto numLODs() const { return meshLODs_.empty() ? 1 : meshLODs_.size(); }

		/**
		 * Set the physical object.
		 */
		void addPhysicalObject(const ref_ptr<PhysicalObject> &physicalObject) {
			physicalObjects_.push_back(physicalObject);
		}

		/**
		 * @return the physical object.
		 */
		auto &physicalObjects() const { return physicalObjects_; }

		/**
		 * @return VAO that is used to render from array data.
		 */
		const ref_ptr<VAO> &vao() const { return vao_; }

		/**
		 * @return face primitive of this mesh.
		 */
		GLenum primitive() const { return primitive_; }

		/**
		 * @param primitive face primitive of this mesh.
		 */
		void set_primitive(GLenum primitive) { primitive_ = primitive; }

		/**
		 * @return the position attribute.
		 */
		ref_ptr<ShaderInput> positions() const;

		/**
		 * @return the normal attribute.
		 */
		ref_ptr<ShaderInput> normals() const;

		/**
		 * @return the color attribute.
		 */
		ref_ptr<ShaderInput> colors() const;

		/**
		 * @return the bone indices attribute.
		 */
		ref_ptr<ShaderInput> boneIndices() const;

		/**
		 * @return the bone weights attribute.
		 */
		ref_ptr<ShaderInput> boneWeights() const;

		/**
		 * The center position of this mesh.
		 */
		Vec3f centerPosition() const;

		/**
		 * The bounds of this mesh.
		 */
		void set_bounds(const Vec3f &min, const Vec3f &max);

		/**
		 * Minimum extends relative to center position.
		 */
		const Vec3f &minPosition() { return minPosition_; }

		/**
		 * Maximum extends relative to center position.
		 */
		const Vec3f &maxPosition() { return maxPosition_; }

		/**
		 * @return the modification stamp of the geometry.
		 */
		auto geometryStamp() const { return geometryStamp_; }

		/**
		 * Increment the geometry stamp.
		 */
		void nextGeometryStamp() { geometryStamp_++; }

		/**
		 * @param range buffer range to use for transform feedback.
		 */
		void setFeedbackRange(const ref_ptr<BufferRange> &range);

		/**
		 * @return the number of primitives generated by transform feedback.
		 */
		auto feedbackCount() const { return feedbackCount_; }

		/**
		 * Add an animation to this mesh.
		 * The purpose of this is that the mesh holds a reference to the animation.
		 * @param animation the animation to add.
		 */
		void addAnimation(const ref_ptr<Animation> &animation) { animations_.push_back(animation); }

		// override
		void enable(RenderState *) override;

		void disable(RenderState *) override;

	protected:
		GLenum primitive_;

		ref_ptr<VAO> vao_;
		std::vector<MeshLOD> meshLODs_;
		float lodFar_ = 160.0f;
		unsigned int lodLevel_ = 0;

		std::list<ShaderInputLocation> vaoAttributes_;
		std::map<GLint, std::list<ShaderInputLocation>::iterator> vaoLocations_;

		ref_ptr<Shader> meshShader_;
		std::map<GLint, ShaderInputLocation> meshUniforms_;

		ref_ptr<BufferRange> feedbackRange_;
		GLuint feedbackCount_;

		GLboolean hasInstances_;

		ref_ptr<Mesh> sourceMesh_;
		std::set<Mesh *> meshViews_;
		GLboolean isMeshView_;

		Vec3f minPosition_;
		Vec3f maxPosition_;
		unsigned int geometryStamp_ = 0u;

		std::vector<ref_ptr<PhysicalObject>> physicalObjects_;

		std::vector<ref_ptr<Animation> > animations_;

		void (ShaderInputContainer::*draw_)(GLenum);

		void updateDrawFunction();

		void addShaderInput(const std::string &name, const ref_ptr<ShaderInput> &in);
	};
} // namespace

namespace regen {
	/**
	 * \brief Mesh that can be used when no vertex shader input
	 * is required.
	 *
	 * This effectively means that you have to generate
	 * geometry that will be rasterized.
	 */
	class AttributeLessMesh : public Mesh {
	public:
		/**
		 * @param numVertices number of vertices used.
		 */
		explicit AttributeLessMesh(GLuint numVertices);
	};
} // namespace

#endif /* REGEN_MESH_STATE_H_ */
