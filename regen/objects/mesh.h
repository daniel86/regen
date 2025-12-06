#ifndef REGEN_MESH_STATE_H_
#define REGEN_MESH_STATE_H_

#include <vector>

#include <regen/scene/state.h>
#include <regen/memory/vbo.h>
#include <regen/gl/vao.h>
#include <regen/memory/ssbo.h>
#include "regen/shader/shader.h"
#include <regen/animation/animation.h>
#include "regen/simulation/physical-object.h"
#include "regen/scene/state-node.h"
#include "regen/camera/sort-mode.h"
#include "regen/memory/element-buffer.h"
#include "regen/scene/state-config.h"
#include "regen/textures/material-state.h"
#include "regen/shapes/bounding-shape.h"

namespace regen {
	/**
	 * \brief A collection of vertices, edges and faces that defines the shape of an object in 3D space.
	 *
	 * When this State is enabled the actual draw call is done. Make sure to setup shader
	 * and server side states before.
	 */
	class Mesh : public State {
	public:
		static constexpr const char *TYPE_NAME = "Mesh";

		/**
		 * \brief A mesh level of detail (LOD) description.
		 *
		 * Each LOD has a specific section of the vertex buffer.
		 * If numVertices=0, then all vertices of the mesh are used.
		 */
		struct MeshLOD {
			struct SharedData {
				uint32_t numVertices = 0;
				uint32_t vertexOffset = 0;
				uint32_t numIndices = 0;
				// offset into the index buffer for this LOD, in bytes.
				uint32_t indexOffset = 0;
				// current number of visible instances for this LOD.
				uint32_t numVisibleInstances = 0;
				// current offset into the instance ID map for this LOD.
				uint32_t instanceOffset = 0;
			};
			MeshLOD() {
				d->numVertices = 0;
				d->vertexOffset = 0;
				d->numIndices = 0;
				d->indexOffset = 0;
				d->numVisibleInstances = 0;
				d->instanceOffset = 0;
			}
			MeshLOD(uint32_t numVertices, uint32_t vertexOffset,
					uint32_t numIndices, uint32_t indexOffset,
					const ref_ptr<Mesh> &impostorMesh = {})
					: impostorMesh(impostorMesh) {
				d->numVertices = numVertices;
				d->vertexOffset = vertexOffset;
				d->numIndices = numIndices;
				d->indexOffset = indexOffset;
				d->numVisibleInstances = 0;
				d->instanceOffset = 0;
			}
			explicit MeshLOD(const ref_ptr<Mesh> &mesh)
					: impostorMesh(mesh) {
				d->numVertices = mesh->numVertices();
				d->vertexOffset = mesh->vertexOffset();
				d->numIndices = mesh->numIndices();
				d->indexOffset = mesh->indexOffset();
				d->numVisibleInstances = 0;
				d->instanceOffset = 0;
			}
			ref_ptr<SharedData> d = ref_ptr<SharedData>::alloc();
			uint32_t numVertices() const { return d->numVertices; }
			uint32_t vertexOffset() const { return d->vertexOffset; }
			uint32_t numIndices() const { return d->numIndices; }
			uint32_t indexOffset() const { return d->indexOffset; }
			// number of visible instances for this LOD.
			uint32_t numVisibleInstances() const { return d->numVisibleInstances; }
			// offset into the instance ID map for this LOD.
			uint32_t instanceOffset() const { return d->instanceOffset; }

			// optional LOD mesh, i.e. a completely different mesh which is used for some
			// LOD levels. This is e.g used for "impostor billboards" where the mesh is
			// replaced by a 2D quad with a texture of the original mesh.
			// Note that the impostor mesh might also enable a custom shader which must be
			// joined into the impostorMesh state (if no shader is joined, then base mesh
			// shader is used).
			// if null, then attributes of base mesh and base mesh shader are used.
			ref_ptr<Mesh> impostorMesh = {};
		};

		/**
		 * Shallow copy constructor.
		 * Vertex data is not copied.
		 * @param meshResource another mesh that provides vertex data.
		 */
		explicit Mesh(const ref_ptr<Mesh> &meshResource);

		/**
		 * Constructor.
		 * @param primitive the GL primitive type.
		 * @param hints buffer update hints for the vertex buffer.
		 * @param vertexLayout layout of the vertex buffer.
		 */
		Mesh(GLenum primitive,
				const BufferUpdateFlags &hints,
				VertexLayout vertexLayout = VERTEX_LAYOUT_INTERLEAVED);

		~Mesh() override;

		/**
		 * Set the mapping mode for the vertex buffer.
		 * Note that mapping will not be possible if the buffer when
		 * map mode is set to BUFFER_MAP_DISABLED.
		 * @param mode the mapping mode to set.
		 */
		void setBufferMapMode(BufferMapMode mode);

		/**
		 * Set the access mode for the vertex buffer.
		 * This will determine how the buffer can be accessed by the CPU and GPU.
		 * Note that GPU_ONLY buffers cannot be modified at all by the CPU,
		 * no copying or mapping is possible!
		 * @param mode the access mode to set.
		 */
		void setClientAccessMode(ClientAccessMode mode);

		/**
		 * Set the material for this mesh.
		 * @param material the material to set.
		 */
		void setMaterial(const ref_ptr<Material> &material);

		/**
		 * @return true if this mesh has a material assigned.
		 */
		bool hasMaterial() const { return material_.get() != nullptr; }

		/**
		 * @return material assigned to this mesh.
		 */
		const ref_ptr<Material> &material() const { return material_; }

		/**
		 * @return VBO that manages the vertex array data.
		 */
		const ref_ptr<VBO> &vertexBuffer() const { return vertexBuffer_; }

		/**
		 * Assign a shader key to this mesh.
		 * This will be used to load the shader in case createShader is used.
		 * @param key the shader key.
		 */
		void setShaderKey(const std::string &key) { shaderKey_ = key; }

		/**
		 * Assign a shader key to this mesh.
		 * This will be used to load the shader in case createShader is used.
		 * @param key the shader key.
		 * @param stage the shader stage.
		 */
		void setShaderKey(const std::string &key, GLenum stage) { shaderStageKeys_[stage] = key; }

		/**
		 * @return true if this mesh has a shader key.
		 */
		bool hasShaderKey() const { return !shaderKey_.empty() || !shaderStageKeys_.empty(); }

		/**
		 * @return Specifies the number of vertices to be rendered.
		 */
		void set_vertexOffset(int32_t v);

		/**
		 * @return Specifies the number of vertices to be rendered.
		 */
		int32_t vertexOffset() const;

		/**
		 * @return Base instance for instanced rendering.
		 */
		uint32_t baseInstance() const;

		/**
		 * @param v Base instance for instanced rendering.
		 */
		void set_baseInstance(uint32_t v);

		/**
		 * @return Number of visible instances of added input data.
		 */
		int32_t numVisibleInstances() const;

		/**
		 * @param v Specifies the number of instances to be rendered.
		 */
		void set_numVisibleInstances(int32_t v);

		/**
		 * Sets the index attribute.
		 * @param indices the index attribute.
		 * @param maxIndex maximal index in the index array.
		 */
		ref_ptr<BufferReference> setIndices(const ref_ptr<ShaderInput> &indices, uint32_t maxIndex);

		/**
		 * @return Specifies the offset to the index buffer in bytes.
		 */
		void set_indexOffset(uint32_t v);

		/**
		 * @return Specifies the number of indices to be rendered.
		 */
		void set_numIndices(int32_t v);

		/**
		 * @return number of indices to vertex data.
		 */
		int numIndices() const;

		/**
		 * @return the maximal index in the index buffer.
		 */
		uint32_t maxIndex() const;

		/**
		 * @return the offset to the index buffer in bytes.
		 */
		uint32_t indexOffset() const;

		/**
		 * @return indexes to the vertex data of this primitive set.
		 */
		const ref_ptr<ShaderInput> &indices() const;

		/**
		 * @return index buffer used by this mesh.
		 */
		uint32_t indexBuffer() const;

		/**
		 * Sets the indirect draw buffer.
		 * @param indirectDrawBuffer the indirect draw buffer.
		 * @param baseDrawIdx base draw index.
		 * @param numDrawLayers number of draw layers for each LOD level.
		 */
		void setIndirectDrawBuffer(
					const ref_ptr<SSBO> &indirectDrawBuffer,
					uint32_t baseDrawIdx,
					uint32_t numDrawLayers);

		/**
		 * Create an indirect draw buffer for this mesh.
		 * This will create a draw command for each LOD level and layer.
		 * Note that this will switch the draw command such that each mesh draw
		 * is done numDrawLayers times for each LOD level.
		 * Usually the LOD system is used to create the indirect draw buffer,
		 * but for some cases it is convenient to create it manually.
		 * @param numDrawLayers the number of draw layers for each LOD level.
		 */
		void createIndirectDrawBuffer(uint32_t numDrawLayers);

		/**
		 * @return true if this input container has an index buffer.
		 */
		bool hasIndirectDrawBuffer() const { return indirectDrawBuffer_.get() != nullptr; }

		/**
		 * @return the base draw index in the indirect draw buffer.
		 */
		uint32_t baseDrawIndex() const { return baseDrawIdx_; }

		/**
		 * @return Offset to the indirect draw call in bytes.
		 */
		void set_indirectOffset(uint32_t v) { indirectOffset_ = v; }

		/**
		 * @param v the number of multi draw calls.
		 */
		void set_multiDrawCount(int32_t v) { multiDrawCount_ = v; }

		/**
		 * @return the indirect draw buffer.
		 */
		const ref_ptr<SSBO> &indirectDrawBuffer() const { return indirectDrawBuffer_; }

		/**
		 * Create a shader for this mesh.
		 * This will also call the create shader function of all attached LOD meshes.
		 * @param parentNode the parent node of this mesh in the scene graph.
		 */
		virtual void createShader(const ref_ptr<StateNode> &parentNode);

		/**
		 * Update the vertex data in the GPU buffer.
		 * This will copy all added vertex attributes into the VBO.
		 * @return the buffer reference of the updated vertex buffer.
		 */
		ref_ptr<BufferReference> updateVertexData();

		/**
		 * Assign a shader state to this mesh.
		 * Update VAO that is used to render from array data.
		 * And setup uniforms and textures not handled in Shader class.
		 * Basically all uniforms and textures declared as parent nodes of
		 * a Shader instance are auto-enabled by that Shader. All remaining uniforms
		 * and textures are activated in Mesh::enable.
		 * @param cfg the state configuration.
		 * @param shader the mesh shader.
		 */
		void updateVAO(const StateConfig &cfg, const ref_ptr<Shader> &shader);

		/**
		 * Update VAO using last StateConfig.enable.
		 */
		void updateVAO();

		/**
		 * Update VAO using given buffer name.
		 * @param bufferName the buffer name to bind to ARRAY_BUFFER.
		 */
		void updateVAO(uint32_t bufferName);

		/**
		 * Update the level of detail based on camera distance.
		 * @param cameraDistance distance to camera.
		 */
		void updateLOD(float cameraDistance);

		/**
		 * Get the level of detail based on camera distance.
		 * @param distanceSquared squared distance to camera.
		 * @return the level of detail.
		 */
		uint32_t getLODLevel(float distanceSquared, const Vec4i &lodShift) const;

		/**
		 * Activate given LOD level.
		 */
		void activateLOD(uint32_t lodLevel);

		/**
		 * Ensure the mesh has at least one LOD level.
		 */
		void ensureLOD();

		/**
		 * Update the visibility of instances for the given LOD level.
		 * This will set the number of visible instances and the instance offset
		 * for the next LOD level.
		 * @param lodLevel the LOD level to update.
		 * @param numInstances number of visible instances.
		 * @param instanceOffset offset into the instance ID map.
		 */
		void updateVisibility(uint32_t lodLevel, uint32_t numInstances, uint32_t instanceOffset);

		/**
		 * Reset visibility of all LODs.
		 * This will set the number of visible instances to 0 for all LODs
		 * and reset the instance offset.
		 * @param resetToInvisible if true, then all LODs are set to 0 visible instances, else first LOD has all instances visible.
		 */
		void resetVisibility(bool resetToInvisible=false);

		/**
		 * @return the current LOD level.
		 */
		uint32_t lodLevel() const { return *lodLevel_.get(); }

		/**
		 * @return the list of LODs of this mesh.
		 */
		const std::vector<MeshLOD> &meshLODs() const { return meshLODs_; }

		/**
		 * @return thresholds for LOD levels.
		 */
		const ref_ptr<ShaderInput3f>& u_lodThresholds() const { return lodThresholds_; }

		/**
		 * @return thresholds for LOD levels.
		 */
		const Vec3f& lodThresholds() const { return v_lodThresholds_; }

		/**
		 * Set the thresholds for LOD levels.
		 * @param thresholds thresholds for LOD levels.
		 */
		void setLODThresholds(const Vec3f &thresholds);

		/**
		 * Set the LODs of this mesh.
		 */
		void setMeshLODs(const std::vector<MeshLOD> &meshLODs);

		/**
		 * Add a LOD to this mesh.
		 * @param meshLOD the LOD to add.
		 */
		void addMeshLOD(const MeshLOD &meshLOD);

		/**
		 * @return number of LODs.
		 */
		uint32_t numLODs() const { return numLODs_; }

		/**
		 * Sets the sort mode for LOD levels, if back-to-front
		 * is used, then low LOD levels are rendered first,
		 * @param mode the sort mode to set.
		 */
		void setSortMode(SortMode mode) { lodSortMode_ = mode; }

		/**
		 * Sets the cull shape for this mesh.
		 * It holds some state that is used to cull the mesh
		 * @param cullShape the cull shape to set.
		 */
		void setCullShape(const ref_ptr<State> &cullShape);

		/**
		 * Set the instance buffer for this mesh.
		 * @param instanceBuffer the instance buffer to set.
		 */
		void setInstanceBuffer(const ref_ptr<SSBO> &instanceBuffer);

		/**
		 * @return the cull shape.
		 */
		ref_ptr<State> cullShape() const { return cullShape_; }

		/**
		 * @return true if this mesh has a cull shape.
		 */
		bool hasCullShape() const { return cullShape_.get() != nullptr; }

		/**
		 * Assign a bounding shape to this mesh.
		 * Optionally create a GBU buffer and attach it to the mesh.
		 * @param shape the bounding shape.
		 */
		void setBoundingShape(const ref_ptr<BoundingShape> &shape);

		/**
		 * Set the indexed shapes for this mesh.
		 * @param shapes the indexed shapes.
		 */
		void setIndexedShapes(const ref_ptr<std::vector<ref_ptr<BoundingShape>>> &shapes);

		/**
		 * @return the indexed shape, if any.
		 */
		const ref_ptr<BoundingShape> &indexedShape(uint32_t instanceIdx) const;

		/**
		 * @return true if this mesh has indexed shapes.
		 */
		bool hasIndexedShapes() const { return indexedShapes_.get() != nullptr; }

		/**
		 * Create a bounding sphere or box for this mesh based on the
		 * min and max positions of vertices.
		 */
		void createBoundingSphere();

		/**
		 * Create a bounding box for this mesh based on the
		 * min and max positions of vertices.
		 * @param isOBB true if the bounding box should be an oriented bounding box.
		 */
		void createBoundingBox(bool isOBB);

		/**
		 * @return the bounding shape, if any.
		 */
		bool hasBoundingShape() const { return boundingShape_.get() != nullptr; }

		/**
		 * @return the bounding shape.
		 */
		const ref_ptr<BoundingShape>& boundingShape() const { return boundingShape_; }

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
		uint32_t geometryStamp() const { return geometryStamp_; }

		/**
		 * Increment the geometry stamp.
		 */
		void nextGeometryStamp() { geometryStamp_++; }

		/**
		 * @param range buffer range to use for transform feedback.
		 */
		void setFeedbackRange(const BufferRange *range) { feedbackRange_ = range; }

		/**
		 * Add an animation to this mesh.
		 * The purpose of this is that the mesh holds a reference to the animation.
		 * @param animation the animation to add.
		 */
		void addAnimation(const ref_ptr<Animation> &animation) { animations_.push_back(animation); }

		/**
		 * @return the shared state.
		 */
		ref_ptr<State> sharedState() const { return sharedState_; }

		/**
		 * Make a draw call with this mesh. This will internally
		 * call enable and disable.
		 * @param rs the render state.
		 */
		void draw(RenderState *rs);

		/**
		 * Load shader keys from the input node.
		 * @param input the scene input node.
		 */
		void loadShaderConfig(LoadingContext &ctx, scene::SceneInputNode &input);

		// override
		void enable(RenderState *) override;

		/**
		 * render primitives from array data.
		 * @param primitive Specifies what kind of primitives to render.
		 */
		void draw(GLenum primitive) const;

		/**
		 * render primitives from array data.
		 * @param primitive Specifies what kind of primitives to render.
		 */
		void drawIndexed(GLenum primitive) const;

		/**
		 * draw multiple instances of a range of elements.
		 * @param primitive Specifies what kind of primitives to render.
		 */
		void drawInstances(GLenum primitive) const;

		/**
		 * draw multiple instances of a set of elements.
		 * @param primitive Specifies what kind of primitives to render.
		 */
		void drawInstancesIndexed(GLenum primitive) const;

		/**
		 * draw multiple instances of a range of elements with base instance.
		 * @param primitive Specifies what kind of primitives to render.
		 */
		void drawBaseInstances(GLenum primitive) const;

		/**
		 * draw multiple instances of a set of elements with base instance.
		 * @param primitive Specifies what kind of primitives to render.
		 */
		void drawBaseInstancesIndexed(GLenum primitive) const;

		/**
		 * render primitives from array data using indirect draw call.
		 * @param primitive Specifies what kind of primitives to render.
		 */
		void drawIndirect(GLenum primitive) const;

		/**
		 * render primitives from array data using indirect draw call.
		 * @param primitive Specifies what kind of primitives to render.
		 */
		void drawIndirectIndexed(GLenum primitive) const;

		/**
		 * render primitives from array data using multi indirect draw call.
		 * @param primitive Specifies what kind of primitives to render.
		 */
		void drawMultiIndirect(GLenum primitive) const;

		/**
		 * render primitives from array data using multi indexed indirect draw call.
		 * @param primitive Specifies what kind of primitives to render.
		 */
		void drawMultiIndirectIndexed(GLenum primitive) const;

	protected:
		GLenum primitive_;
		ref_ptr<VBO> vertexBuffer_;
		ref_ptr<ElementBuffer> elementBuffer_;
		ref_ptr<Material> material_;

		ref_ptr<VAO> vao_;
		std::list<InputLocation> vaoAttributes_;
		std::map<int32_t, std::list<InputLocation>::iterator> vaoLocations_;

		ref_ptr<SSBO> instanceBuffer_;
		std::vector<MeshLOD> meshLODs_;
		ref_ptr<ShaderInput3f> lodThresholds_;
		Vec3f v_lodThresholds_;
		// note: it is important that this is shared between copies of the mesh
		//       as the lod state only changes lod level of the original mesh.
		ref_ptr<uint32_t> lodLevel_;
		uint32_t lastNumVertices_ = 0u;
		uint32_t numLODs_ = 1u;
		SortMode lodSortMode_ = SortMode::FRONT_TO_BACK;

		ref_ptr<State> cullShape_;
		ref_ptr<BoundingShape> boundingShape_;
		ref_ptr<std::vector<ref_ptr<BoundingShape>>> indexedShapes_;
		int32_t shapeType_ = -1;

		// indirect draw buffer data
		uint32_t baseDrawIdx_ = 0u;
		uint32_t numDrawLayers_ = 1u;
		uint32_t numDrawLODs_ = 0u;
		int32_t multiDrawCount_ = 1u;
		uint32_t indirectOffset_ = 0u;
		ref_ptr<SSBO> indirectDrawBuffer_;
		std::vector<int32_t> indirectDrawGroups_;

		ref_ptr<Shader> meshShader_;
		std::string shaderKey_;
		std::map<GLenum, std::string> shaderStageKeys_;

		const BufferRange *feedbackRange_ = nullptr;

		bool hasInstances_ = false;

		ref_ptr<Mesh> sourceMesh_;
		std::set<Mesh *> meshViews_;
		bool isMeshView_ = false;

		Vec3f minPosition_;
		Vec3f maxPosition_;
		unsigned int geometryStamp_ = 0u;

		std::vector<ref_ptr<PhysicalObject>> physicalObjects_;
		std::vector<ref_ptr<Animation> > animations_;

		void (Mesh::*draw_)(GLenum) const;

		void updateDrawFunction();

		void updateVAO_();

		void createShader(const ref_ptr<StateNode> &parentNode, StateConfig &shaderConfigurer);

		void addShaderInput(const std::string &name, const ref_ptr<ShaderInput> &in);

		void drawMesh(RenderState *rs);

		void drawMeshLOD(RenderState *rs, uint32_t lodLevel, uint32_t drawIdx, int32_t multiDrawCount);

		void activateLOD_(uint32_t lodLevel);

	private:
		struct SharedData; // forward declaration
		ref_ptr<SharedData> shared_;
		// a state shared among all copies of this mesh.
		ref_ptr<State> sharedState_;
	};
} // namespace

#endif /* REGEN_MESH_STATE_H_ */
