#ifndef REGEN_MESH_VECTOR_H_
#define REGEN_MESH_VECTOR_H_

#include <vector>
#include <regen/meshes/mesh-state.h>
#include "regen/text/texture-mapped-text.h"
#include "particles.h"
#include "assimp-importer.h"

namespace regen {
	/**
	 * A vector of meshes.
	 */
	class MeshVector : public std::vector<ref_ptr<Mesh> >, public Resource {
	public:
		static constexpr const char *TYPE_NAME = "Mesh";

		MeshVector() = default;

		explicit MeshVector(GLuint numMeshes) : std::vector<ref_ptr<Mesh> >(numMeshes) {}

		static ref_ptr<MeshVector> load(LoadingContext &ctx, scene::SceneInputNode &input);

		MeshVector &operator=(const std::vector<ref_ptr<Mesh> > &rhs) {
			std::vector<ref_ptr<Mesh> >::operator=(rhs);
			return *this;
		}

	protected:
		static ref_ptr<MeshVector> createAssetMeshes(LoadingContext &ctx, scene::SceneInputNode &input, const ref_ptr<AssetImporter> &importer);

		static ref_ptr<Particles> createParticleMesh(LoadingContext &ctx, scene::SceneInputNode &input, const GLuint numParticles);

		static ref_ptr<TextureMappedText> createTextMesh(LoadingContext &ctx, scene::SceneInputNode &input);
	};
} // namespace

#endif /* REGEN_MESH_VECTOR_H_ */
