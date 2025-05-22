/*
 * resource-manager.cpp
 *
 *  Created on: Nov 4, 2013
 *      Author: daniel
 */

#include "resource-manager.h"

using namespace regen::scene;
using namespace regen;
using namespace std;

void ResourceManager::loadResources(SceneLoader *parser, const std::string &id) {
	assets_.getResource(parser, id);
	cameras_.getResource(parser, id);
	fbos_.getResource(parser, id);
	bufferBlocks_.getResource(parser, id);
	fonts_.getResource(parser, id);
	indices_.getResource(parser, id);
	lights_.getResource(parser, id);
	meshes_.getResource(parser, id);
	textures_.getResource(parser, id);
	skies_.getResource(parser, id);
}

ref_ptr<Camera> ResourceManager::getCamera(SceneLoader *parser, const std::string &id) {
	loadResources(parser, id);
	return cameras_.getResource(parser, id);
}

ref_ptr<Light> ResourceManager::getLight(SceneLoader *parser, const std::string &id) {
	loadResources(parser, id);
	return lights_.getResource(parser, id);
}

ref_ptr<regen::Font> ResourceManager::getFont(SceneLoader *parser, const std::string &id) {
	loadResources(parser, id);
	return fonts_.getResource(parser, id);
}

ref_ptr<regen::SpatialIndex> ResourceManager::getIndex(SceneLoader *parser, const std::string &id) {
	loadResources(parser, id);
	return indices_.getResource(parser, id);
}

ref_ptr<FBO> ResourceManager::getFBO(SceneLoader *parser, const std::string &id) {
	loadResources(parser, id);
	return fbos_.getResource(parser, id);
}

ref_ptr<BufferBlock> ResourceManager::getBufferBlock(SceneLoader *parser, const std::string &id) {
	loadResources(parser, id);
	return bufferBlocks_.getResource(parser, id);
}

ref_ptr<Texture> ResourceManager::getTexture(SceneLoader *parser, const std::string &id) {
	loadResources(parser, id);
	return textures_.getResource(parser, id);
}

ref_ptr<Texture2D> ResourceManager::getTexture2D(SceneLoader *parser, const std::string &id) {
	return ref_ptr<Texture2D>::dynamicCast(getTexture(parser, id));
}

ref_ptr<MeshVector> ResourceManager::getMesh(SceneLoader *parser, const std::string &id) {
	loadResources(parser, id);
	return meshes_.getResource(parser, id);
}

ref_ptr<MeshVector> ResourceManager::createMesh(SceneLoader *parser, scene::SceneInputNode &input) {
	auto id = input.getName();
	auto mesh = meshes_.createResource(parser, input);
	meshes_.putResource(id, mesh);
	return mesh;
}

ref_ptr<ModelTransformation> ResourceManager::getTransform(SceneLoader *parser, const std::string &id) {
	return transforms_[id];
}

ref_ptr<AssetImporter> ResourceManager::getAsset(SceneLoader *parser, const std::string &id) {
	loadResources(parser, id);
	return assets_.getResource(parser, id);
}

ref_ptr<Sky> ResourceManager::getSky(SceneLoader *parser, const std::string &id) {
	loadResources(parser, id);
	return skies_.getResource(parser, id);
}

void ResourceManager::putCamera(const std::string &id, const ref_ptr<Camera> &cam) {
	cameras_.putResource(id, cam);
}

void ResourceManager::putLight(const std::string &id, const ref_ptr<Light> &light) {
	lights_.putResource(id, light);
}

void ResourceManager::putFont(const std::string &id, const ref_ptr<regen::Font> &font) {
	fonts_.putResource(id, font);
}

void ResourceManager::putIndex(const std::string &id, const ref_ptr<regen::SpatialIndex> &index) {
	indices_.putResource(id, index);
}

void ResourceManager::putFBO(const std::string &id, const ref_ptr<FBO> &fbo) {
	fbos_.putResource(id, fbo);
}

void ResourceManager::putBufferBlock(const std::string &id, const ref_ptr<BufferBlock> &ubo) {
	bufferBlocks_.putResource(id, ubo);
}

void ResourceManager::putTexture(const std::string &id, const ref_ptr<Texture> &texture) {
	textures_.putResource(id, texture);
}

void ResourceManager::putMesh(const std::string &id, const ref_ptr<MeshVector> &meshes) {
	meshes_.putResource(id, meshes);
}

void ResourceManager::putTransform(const std::string &id, const ref_ptr<ModelTransformation> &transform) {
	transforms_[id] = transform;
}

void ResourceManager::putAsset(const std::string &id, const ref_ptr<AssetImporter> &asset) {
	assets_.putResource(id, asset);
}

void ResourceManager::putSky(const std::string &id, const ref_ptr<Sky> &sky) {
	skies_.putResource(id, sky);
}
