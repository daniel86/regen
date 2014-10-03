/*
 * resource-manager.cpp
 *
 *  Created on: Nov 4, 2013
 *      Author: daniel
 */

#include "resource-manager.h"
using namespace regen::scene;
using namespace regen;

void ResourceManager::loadResources(SceneParser *parser, const std::string &id)
{
  assets_.getResource(parser,id);
  cameras_.getResource(parser,id);
  fbos_.getResource(parser,id);
  fonts_.getResource(parser,id);
  lights_.getResource(parser,id);
  meshes_.getResource(parser,id);
  textures_.getResource(parser,id);
  skies_.getResource(parser,id);
}

ref_ptr<Camera> ResourceManager::getCamera(SceneParser *parser, const std::string &id)
{
  loadResources(parser,id);
  return cameras_.getResource(parser,id);
}
ref_ptr<Light> ResourceManager::getLight(SceneParser *parser, const std::string &id)
{
  loadResources(parser,id);
  return lights_.getResource(parser,id);
}
ref_ptr<regen::Font> ResourceManager::getFont(SceneParser *parser, const std::string &id)
{
  loadResources(parser,id);
  return fonts_.getResource(parser,id);
}
ref_ptr<FBO> ResourceManager::getFBO(SceneParser *parser, const std::string &id)
{
  loadResources(parser,id);
  return fbos_.getResource(parser,id);
}
ref_ptr<Texture> ResourceManager::getTexture(SceneParser *parser, const std::string &id)
{
  loadResources(parser,id);
  return textures_.getResource(parser,id);
}
ref_ptr<MeshVector> ResourceManager::getMesh(SceneParser *parser, const std::string &id)
{
  loadResources(parser,id);
  return meshes_.getResource(parser,id);
}
ref_ptr<AssetImporter> ResourceManager::getAsset(SceneParser *parser, const std::string &id)
{
  loadResources(parser,id);
  return assets_.getResource(parser,id);
}
ref_ptr<Sky> ResourceManager::getSky(SceneParser *parser, const std::string &id)
{
  loadResources(parser,id);
  return skies_.getResource(parser,id);
}

void ResourceManager::putCamera(const std::string &id, const ref_ptr<Camera> &cam)
{
  cameras_.putResource(id,cam);
}
void ResourceManager::putLight(const std::string &id, const ref_ptr<Light> &light)
{
  lights_.putResource(id,light);
}
void ResourceManager::putFont(const std::string &id, const ref_ptr<regen::Font> &font)
{
  fonts_.putResource(id,font);
}
void ResourceManager::putFBO(const std::string &id, const ref_ptr<FBO> &fbo)
{
  fbos_.putResource(id,fbo);
}
void ResourceManager::putTexture(const std::string &id, const ref_ptr<Texture> &texture)
{
  textures_.putResource(id,texture);
}
void ResourceManager::putMesh(const std::string &id, const ref_ptr<MeshVector> &meshes)
{
  meshes_.putResource(id,meshes);
}
void ResourceManager::putAsset(const std::string &id, const ref_ptr<AssetImporter> &asset)
{
  assets_.putResource(id,asset);
}
void ResourceManager::putSky(const std::string &id, const ref_ptr<Sky> &sky)
{
  skies_.putResource(id,sky);
}
