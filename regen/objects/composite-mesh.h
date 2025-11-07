#ifndef REGEN_COMPOSITE_MESH_H_
#define REGEN_COMPOSITE_MESH_H_

#include <vector>
#include <queue>
#include <regen/objects/mesh.h>
#include "regen/objects/text/texture-mapped-text.h"
#include "particles.h"
#include "assimp-importer.h"

namespace regen {
	/**
	 * \brief A collection of Mesh objects forming an object.
	 */
	class CompositeMesh : public Resource {
	public:
		static constexpr const char *TYPE_NAME = "Mesh";

		/**
		 * Load a CompositeMesh from the given input node.
		 * @param ctx the loading context.
		 * @param input the scene input node.
		 * @return the loaded CompositeMesh.
		 */
		static ref_ptr<CompositeMesh> load(LoadingContext &ctx, scene::SceneInputNode &input);

		/**
		 * Create an empty CompositeMesh.
		 */
		CompositeMesh() = default;

		/**
		 * Add a mesh to this composite mesh.
		 * @param mesh the mesh to add.
		 */
		void addMesh(const ref_ptr<Mesh> &mesh) { meshes_.push_back(mesh); }

		/**
		 * @return the meshes in this composite mesh.
		 */
		const std::vector<ref_ptr<Mesh>>& meshes() const { return meshes_; }

		/**
		 * Load mesh indices from the input node.
		 * @param input the scene input node.
		 * @param prefix the prefix for the mesh index attributes.
		 * @return the loaded mesh indices.
		 */
		static std::vector<uint32_t> loadIndexRange(
			scene::SceneInputNode &input,
			const std::string &prefix = "mesh");

		/**
		 * Load mesh indices from the input node and push the corresponding meshes
		 * to the mesh queue.
		 * @param input the scene input node.
		 * @param compositeMesh the composite mesh containing the meshes.
		 * @param meshQueue the queue to push the meshes to.
		 * @param prefix the prefix for the mesh index attributes.
		 */
		static void loadIndexRange(
			scene::SceneInputNode &input,
			const ref_ptr<CompositeMesh> &compositeMesh,
			std::queue<std::pair<ref_ptr<Mesh>, uint32_t> > &meshQueue,
			const std::string &prefix = "mesh");

	protected:
		std::vector<ref_ptr<Mesh>> meshes_;

		static ref_ptr<CompositeMesh> createCompositeMesh(LoadingContext &ctx, scene::SceneInputNode &input,
		                                                  const ref_ptr<AssetImporter> &importer);

		static ref_ptr<Particles> createParticleMesh(LoadingContext &ctx, scene::SceneInputNode &input,
		                                             const GLuint numParticles);

		static ref_ptr<TextureMappedText> createTextMesh(LoadingContext &ctx, scene::SceneInputNode &input);
	};
} // namespace

#endif /* REGEN_COMPOSITE_MESH_H_ */
