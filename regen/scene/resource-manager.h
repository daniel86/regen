/*
 * resource-manager.h
 *
 *  Created on: Nov 4, 2013
 *      Author: daniel
 */

#ifndef RESOURCE_MANAGER_H_
#define RESOURCE_MANAGER_H_

#include "regen/shapes/spatial-index.h"
#include "regen/meshes/mesh-vector.h"
#include "scene-loader.h"
#include "regen/textures/texture.h"
#include "regen/sky/sky.h"
#include "loadable-input.h"

namespace regen {
	namespace scene {
		/**
		 * Provides typed access to scene resources.
		 */
		class ResourceManager {
		public:
			/**
			 * @param parser The scene parser that contains resources.
			 * @param id the resource id.
			 * @return A Camera resource or null reference.
			 */
			ref_ptr<Camera> getCamera(SceneLoader *parser, const std::string &id);

			/**
			 * @param parser The scene parser that contains resources.
			 * @param id the resource id.
			 * @return A Light resource or null reference.
			 */
			ref_ptr<Light> getLight(SceneLoader *parser, const std::string &id);

			/**
			 * @param parser The scene parser that contains resources.
			 * @param id the resource id.
			 * @return A FBO resource or null reference.
			 */
			ref_ptr<FBO> getFBO(SceneLoader *parser, const std::string &id);

			/**
			 * @param parser The scene parser that contains resources.
			 * @param id the resource id.
			 * @return A BufferBlock resource or null reference.
			 */
			ref_ptr<BufferBlock> getBufferBlock(SceneLoader *parser, const std::string &id);

			/**
			 * @param parser The scene parser that contains resources.
			 * @param id the resource id.
			 * @return A Texture resource or null reference.
			 */
			ref_ptr<Texture> getTexture(SceneLoader *parser, const std::string &id);

			ref_ptr<Texture2D> getTexture2D(SceneLoader *parser, const std::string &id);

			ref_ptr<Sky> getSky(SceneLoader *parser, const std::string &id);

			/**
			 * @param parser The scene parser that contains resources.
			 * @param id the resource id.
			 * @return A MeshVector resource or null reference.
			 */
			ref_ptr<MeshVector> getMesh(SceneLoader *parser, const std::string &id);

			/**
			 * Create a MeshVector from the given SceneInputNode.
			 * @param parser The scene parser that contains resources.
			 * @param input the SceneInputNode to create the MeshVector from.
			 * @return A MeshVector resource or null reference.
			 */
			ref_ptr<MeshVector> createMesh(SceneLoader *parser, scene::SceneInputNode &input);

			ref_ptr<ModelTransformation> getTransform(SceneLoader *parser, const std::string &id);

			/**
			 * @param parser The scene parser that contains resources.
			 * @param id the resource id.
			 * @return A AssetImporter resource or null reference.
			 */
			ref_ptr<AssetImporter> getAsset(SceneLoader *parser, const std::string &id);

			ref_ptr<State> getState(SceneLoader *parser, const std::string &id);

			/**
			 * @param id the resource id.
			 * @param cam A Camera instance.
			 */
			void putCamera(const std::string &id, const ref_ptr<Camera> &cam);

			/**
			 * @param id the resource id.
			 * @param light A Light instance.
			 */
			void putLight(const std::string &id, const ref_ptr<Light> &light);

			/**
			 * @return the light resources of this scene.
			 */
			auto &getLights() { return lights_.resources(); }

			/**
			 * @param parser The scene parser that contains resources.
			 * @param id the resource id.
			 * @return A Font resource or null reference.
			 */
			ref_ptr<regen::Font> getFont(SceneLoader *parser, const std::string &id);

			/**
			 * @param id the resource id.
			 * @param font A Font instance.
			 */
			void putFont(const std::string &id, const ref_ptr<regen::Font> &font);

			/**
			 * @param parser The scene parser that contains resources.
			 * @param id the resource id.
			 * @return A resource or null reference.
			 */
			ref_ptr<regen::SpatialIndex> getIndex(SceneLoader *parser, const std::string &id);

			auto &getIndices() { return indices_.resources(); }

			/**
			 * @param id the resource id.
			 * @param font A resource instance.
			 */
			void putIndex(const std::string &id, const ref_ptr<regen::SpatialIndex> &index);

			/**
			 * @param id the resource id.
			 * @param fbo A FBO instance.
			 */
			void putFBO(const std::string &id, const ref_ptr<FBO> &fbo);

			/**
			 * @param id the resource id.
			 * @param ubo A UBO instance.
			 */
			void putBufferBlock(const std::string &id, const ref_ptr<BufferBlock> &ubo);

			/**
			 * @param id the resource id.
			 * @param texture A Texture instance.
			 */
			void putTexture(const std::string &id, const ref_ptr<Texture> &texture);

			/**
			 * @param id the resource id.
			 * @param meshes A MeshVector instance.
			 */
			void putMesh(const std::string &id, const ref_ptr<MeshVector> &meshes);

			void putTransform(const std::string &id, const ref_ptr<ModelTransformation> &transform);

			/**
			 * @param id the resource id.
			 * @param asset A AssetImporter instance.
			 */
			void putAsset(const std::string &id, const ref_ptr<AssetImporter> &asset);

			void putSky(const std::string &id, const ref_ptr<Sky> &sky);

			/**
			 * Load all resources with given id.
			 * @param parser the SceneParser instance.
			 * @param id resource id.
			 */
			void loadResources(SceneLoader *parser, const std::string &id);

		protected:
			LoadableResource<SpatialIndex> indices_ = LoadableResource<SpatialIndex>("index");
			LoadableResource<AssetImporter> assets_ = LoadableResource<AssetImporter>("asset");
			LoadableResource<BufferBlock> bufferBlocks_ = LoadableResource<BufferBlock>("block");
			LoadableResource<Font> fonts_ = LoadableResource<Font>("font");
			LoadableResource<FBO> fbos_ = LoadableResource<FBO>("fbo");
			LoadableResource<Texture> textures_ = LoadableResource<Texture>("texture");
			LoadableResource<Light> lights_ = LoadableResource<Light>("light");
			LoadableResource<Camera> cameras_ = LoadableResource<Camera>("camera");
			LoadableResource<MeshVector> meshes_ = LoadableResource<MeshVector>("mesh");
			LoadableResource<Sky> skies_ = LoadableResource<Sky>("sky");
			std::map<std::string, ref_ptr<ModelTransformation> > transforms_;
		};
	}
}

#endif /* RESOURCE_MANAGER_H_ */
