/*
 * mesh.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_RESOURCE_MESH_H_
#define REGEN_SCENE_RESOURCE_MESH_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/resources.h>

#include <regen/meshes/mesh-state.h>

namespace regen {
	namespace scene {
		/**
		 * Provides MeshVector instances from SceneInputNode data.
		 */
		class MeshResource : public ResourceProvider<MeshVector> {
		public:
			MeshResource();

			// Override
			ref_ptr<MeshVector> createResource(
					SceneParser *parser, SceneInputNode &input) override;

		protected:
			ref_ptr<MeshVector> createAssetMeshes(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<AssetImporter> &importer);

			ref_ptr<Particles> createParticleMesh(
					SceneParser *parser,
					SceneInputNode &input,
					const GLuint numParticles,
					const std::string &updateShader);

			ref_ptr<TextureMappedText> createTextMesh(
					SceneParser *parser,
					SceneInputNode &input);
		};
	}
}

#endif /* REGEN_SCENE_RESOURCE_MESH_H_ */
