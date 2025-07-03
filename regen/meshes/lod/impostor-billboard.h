#ifndef REGEN_IMPOSTOR_BILLBOARD_H
#define REGEN_IMPOSTOR_BILLBOARD_H

#include <regen/meshes/mesh-state.h>
#include <regen/shapes/bounds.h>
#include <regen/meshes/mesh-vector.h>
#include <regen/textures/texture.h>
#include "regen/states/fbo-state.h"
#include "regen/camera/array-camera.h"

namespace regen {
	/**
	 * \brief A billboard mesh that mimics a more complex mesh.
	 *
	 * To this end, we render the mesh to albedo, normal and depth textures
	 * and then use this information in deferred shading pipeline.
	 * The mesh can be rendered from different perspectives from which the
	 * most favorable one is chosen for rendering in the shader.
	 */
	class ImpostorBillboard : public Mesh {
	public:
		/**
		 * Default constructor.
		 */
		ImpostorBillboard();

		/**
		 * Add a mesh to this billboard.
		 * The billboard will fit tightly around the mesh, and include
		 * all added meshes in the snapshot textures.
		 * @param mesh the mesh to be imitated
		 * @param drawState optional draw state for the mesh
		 */
		void addMesh(const ref_ptr<Mesh> &mesh, const ref_ptr<State> &drawState={});

		/**
		 * Use a custom shader for updating the snapshot textures.
		 * @param key the shader import key.
		 */
		void setSnapshotShaderKey(const std::string &key) { snapshotShaderKey_ = key; }

		/**
		 * Sets the depth offset for the billboard.
		 * - 0 means it is placed at the center of the mesh
		 * - 0.5 means it is placed at the "front" of the mesh
		 * - -0.5 means it is placed at the "back" of the mesh
		 * @param offset the depth offset
		 */
		void setDepthOffset(float offset) { depthOffset_->setVertex(0, offset); }

		/**
		 * Sets the number of longitude steps for the snapshot pass.
		 * e.g. 8 steps (default) means that the snapshot is taken every 45° around the globe.
		 * @param numSteps the number of longitude steps
		 */
		void setLongitudeSteps(uint32_t numSteps) { longitudeSteps_ = numSteps; }

		/**
		 * Sets the number of latitude steps for the snapshot pass.
		 * e.g. 0 steps (default) means that the snapshot is taken only at the equator.
		 * e.g. 1 step means that the snapshot is taken at the equator and at the poles.
		 * @param numSteps the number of latitude steps
		 */
		void setLatitudeSteps(uint32_t numSteps) { latitudeSteps_ = numSteps; }

		/**
		 * Sets whether the snapshot pass should only sample the upper hemisphere.
		 * @param hemispherical true if only the upper hemisphere should be sampled
		 */
		void setHemispherical(bool hemispherical) { isHemispherical_ = hemispherical; }

		/**
		 * Sets whether the snapshot pass should include a sample with 90° north latitude.
		 * @param topView true if a sample with 90° north latitude should be included
		 */
		void setHasTopView(bool topView) { hasTopView_ = topView; }

		/**
		 * Sets whether the snapshot pass should include a sample with 90° south latitude.
		 * @param bottomView true if a sample with 90° south latitude should be included
		 */
		void setHasBottomView(bool bottomView) { hasBottomView_ = bottomView; }

		/**
		 * @param mips true if the albedo texture should use mipmaps
		 */
		void setUseAlbedoMips(bool mips) { useAlbedoMips_ = mips; }

		/**
		 * Sets whether the snapshot pass should use normal correction.
		 * @param normalCorrection true if normal correction should be used
		 */
		void setUseNormalCorrection(bool normalCorrection) { useNormalCorrection_ = normalCorrection; }

		/**
		 * Sets whether the snapshot pass should use depth correction.
		 * @param depthCorrection true if depth correction should be used
		 */
		void setUseDepthCorrection(bool depthCorrection) { useDepthCorrection_ = depthCorrection; }

		/**
		 * Sets the size of the snapshot texture, default is 256x256.
		 * The size is the same for all snapshots.
		 * @param width the width of the snapshot texture
		 * @param height the height of the snapshot texture
		 */
		void setSnapshotTextureSize(uint32_t width, uint32_t height) {
			snapshotWidth_ = width;
			snapshotHeight_ = height;
		}

		/**
		 * @return the number of perspectives/snapshots used for the billboard.
		 */
		uint32_t numSnapshots() const { return numSnapshotViews_; }

		/**
		 * Update the snapshot view data.
		 * This only needs to be done once after the billboard parameters
		 * regarding sampling have been set.
		 */
		void updateSnapshotViews();

		/**
		 * Create a snapshot of the input widgets.
		 * Can be called multiple times, e.g. for cases where the mesh changes.
		 * For static meshes, call this only once.
		 */
		void createSnapshot();

		/**
		 * A global state for the snapshot pass.
		 * Allows the user to insert some states into the pass for all meshes.
		 * Usually is not needed to touch.
		 * @return the state for the snapshot pass
		 */
		ref_ptr<State>& snapshotState() { return snapshotState_; }

		/**
		 * Load an impostor billboard from a scene input node.
		 * @param ctx the loading context
		 * @param input the scene input node
		 * @return the impostor billboard
		 */
		static ref_ptr<ImpostorBillboard> load(LoadingContext &ctx, scene::SceneInputNode &input);

		// override
		void createShader(const ref_ptr<StateNode> &parentNode) override;

	protected:
		struct ImitatedMesh {
			// the mesh state that will do the draw call for doing a snapshot.
			ref_ptr<Mesh> meshOrig;
			ref_ptr<Mesh> meshCopy;
			// each mesh has an optional draw state with mesh-specific configuration
			// for the snapshot pass. by default, no state is used.
			ref_ptr<State> drawState;
			// the shader which is generated for the snapshot pass.
			// the shader uses multiple render targets to draw into all
			// array layers of the snapshot texture in one pass.
			ref_ptr<ShaderState> shaderState;
		};
		// The meshes that are imitated by this billboard.
		// Often models are made from different meshes, e.g. a trunk and leaves of a tree,
		// so we support multiple meshes here.
		std::vector<ImitatedMesh> meshes_;
		float meshBoundsRadius_ = 0.0f;
		std::array<Vec3f,8> meshCornerPoints_;
		Vec3f meshCenterPoint_ = Vec3f(0.0f);

		// 8 -> sample every 45° around the globe
		uint32_t longitudeSteps_ = 8u;
		// 0 -> only sample with 0° latitude (equator)
		// 1 -> also sample top/bottom if requested
		// 2 -> sample with 0°, 45° and 90° latitude
		uint32_t latitudeSteps_ = 0u;
		// include a sample with 90° north latitude
		bool hasTopView_ = false;
		// include a sample with 90° south latitude
		bool hasBottomView_ = false;
		// only sample with positive latitude (also no bottom view)
		bool isHemispherical_ = true;
		// the number of snapshots taken for the billboard
		uint32_t numSnapshotViews_ = 0u;
		// the size of each snapshot texture in pixels
		uint32_t snapshotWidth_ = 256u;
		uint32_t snapshotHeight_ = 256u;

		// a state that is only enabled for the snapshot pass
		ref_ptr<State> snapshotState_;
		// this is an "array camera" with a layer for each snapshot view.
		ref_ptr<ArrayCamera> snapshotCamera_;
		bool hasInitializedResources_ = false;
		bool hasAttributes_ = false;
		bool useNormalCorrection_ = true;
		bool useDepthCorrection_ = false;
		std::string snapshotShaderKey_ = "regen.models.impostor.update";

		// container for billboard uniforms
		ref_ptr<ShaderInput1f> depthOffset_;
		ref_ptr<ShaderInput3f> modelOrigin_;

		// a container for the view data
		ref_ptr<SSBO> impostorBuffer_;
		// array of view directions, one for each snapshot
		ref_ptr<ShaderInput4f> snapshotDirs_;
		// meshes may have different aspect ratios, so we need to store the bounds
		ref_ptr<ShaderInput4f> snapshotOrthoBounds_;
		// the depth ranges for each snapshot, used to compute the depth
		ref_ptr<ShaderInput2f> snapshotDepthRanges_;

		// render target for snapshot pass. We use array textures to store
		// the snapshots, each element of the array is a snapshot from a different
		// perspective. The FBO has attachments for albedo, normal and depth
		// that can be used for the billboard in direct or deferred rendering pipelines.
		// The layout is "longitude-major", starting at the equator and going up to the poles,
		// the northern hemisphere is sampled first (after the equator), then the southern hemisphere.
		ref_ptr<FBOState> snapshotFBO_;
		ref_ptr<Texture2DArray> snapshotAlbedo_;
		ref_ptr<Texture2DArray> snapshotNormal_;
		ref_ptr<Texture2DArrayDepth> snapshotDepth_;
		bool useAlbedoMips_ = true;

		void updateAttributes();

		void createResources();

		void ensureResourcesExist();

		void updateNumberOfViews();

		void addSnapshotView(uint32_t viewIdx, const Vec3f &dir, const Vec3f &up=Vec3f::up());
	};
} // namespace

#endif /* REGEN_IMPOSTOR_BILLBOARD_H */
