/*
 * resource-manager.h
 *
 *  Created on: Nov 4, 2013
 *      Author: daniel
 */

#ifndef RESOURCE_MANAGER_H_
#define RESOURCE_MANAGER_H_

#include <regen/scene/resources/asset.h>
#include <regen/scene/resources/camera.h>
#include <regen/scene/resources/fbo.h>
#include <regen/scene/resources/font.h>
#include <regen/scene/resources/light.h>
#include <regen/scene/resources/mesh.h>
#include <regen/scene/resources/texture.h>
#include <regen/scene/resources/sky.h>
#include "regen/scene/resources/ubo-resource.h"
#include "regen/shapes/spatial-index.h"
#include "regen/scene/resources/index-resource.h"

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
			ref_ptr<Camera> getCamera(SceneParser *parser, const std::string &id);

			/**
			 * @param parser The scene parser that contains resources.
			 * @param id the resource id.
			 * @return A Light resource or null reference.
			 */
			ref_ptr<Light> getLight(SceneParser *parser, const std::string &id);

			/**
			 * @param parser The scene parser that contains resources.
			 * @param id the resource id.
			 * @return A FBO resource or null reference.
			 */
			ref_ptr<FBO> getFBO(SceneParser *parser, const std::string &id);

			/**
			 * @param parser The scene parser that contains resources.
			 * @param id the resource id.
			 * @return A UBO resource or null reference.
			 */
			ref_ptr<UBO> getUBO(SceneParser *parser, const std::string &id);

			/**
			 * @param parser The scene parser that contains resources.
			 * @param id the resource id.
			 * @return A Texture resource or null reference.
			 */
			ref_ptr<Texture> getTexture(SceneParser *parser, const std::string &id);

			ref_ptr<Texture2D> getTexture2D(SceneParser *parser, const std::string &id);

			ref_ptr<Sky> getSky(SceneParser *parser, const std::string &id);

			/**
			 * @param parser The scene parser that contains resources.
			 * @param id the resource id.
			 * @return A MeshVector resource or null reference.
			 */
			ref_ptr<MeshVector> getMesh(SceneParser *parser, const std::string &id);

			ref_ptr<ModelTransformation> getTransform(SceneParser *parser, const std::string &id);

			/**
			 * @param parser The scene parser that contains resources.
			 * @param id the resource id.
			 * @return A AssetImporter resource or null reference.
			 */
			ref_ptr<AssetImporter> getAsset(SceneParser *parser, const std::string &id);

			ref_ptr<State> getState(SceneParser *parser, const std::string &id);

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
			 * @param parser The scene parser that contains resources.
			 * @param id the resource id.
			 * @return A Font resource or null reference.
			 */
			ref_ptr<regen::Font> getFont(SceneParser *parser, const std::string &id);

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
			ref_ptr<regen::SpatialIndex> getIndex(SceneParser *parser, const std::string &id);

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
			void putUBO(const std::string &id, const ref_ptr<UBO> &ubo);

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
			void loadResources(SceneParser *parser, const std::string &id);

		protected:
			AssetResource assets_;
			CameraResource cameras_;
			FBOResource fbos_;
			UBOResource ubos_;
			FontResource fonts_;
			IndexResource indices_;
			LightResource lights_;
			MeshResource meshes_;
			SkyResource skies_;
			TextureResource textures_;
			std::map<std::string, ref_ptr<ModelTransformation> > transforms_;
		};
	}
}

#endif /* RESOURCE_MANAGER_H_ */
