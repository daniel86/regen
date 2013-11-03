/*
 * scene-xml.h
 *
 *  Created on: Oct 26, 2013
 *      Author: daniel
 */

#ifndef SCENE_XML_H_
#define SCENE_XML_H_

#include <regen/states/state-node.h>
#include <regen/states/fbo-state.h>
#include <regen/meshes/mesh-state.h>
#include <regen/meshes/particles.h>
#include <regen/meshes/assimp-importer.h>
#include <regen/meshes/texture-mapped-text.h>
#include <regen/states/texture-state.h>
#include <regen/shading/shadow-map.h>
#include <regen/physics/bullet-physics.h>
#include <regen/utility/ref-ptr.h>
#include <regen/utility/xml.h>
#include <regen/utility/font.h>
#include <regen/meshes/sky.h>
#include <regen/application.h>

using namespace regen;

struct BoneAnimRange {
  BoneAnimRange()
  : name(""), range(Vec2d(0.0,0.0)) {}
  BoneAnimRange(const string &n, const Vec2d &r)
  : name(n), range(r) {}
  string name;
  Vec2d range;
};

class SceneXML {
public:
  SceneXML(Application *app, const string &sceneFile);
  ~SceneXML();

  ref_ptr<BulletPhysics> getPhysics();
  ref_ptr<regen::Font> getFont(const string &id);
  ref_ptr<FBO> getFBO(const string &id);
  ref_ptr<Texture> getTexture(const string &id);
  ref_ptr<SkyScattering> getSky(const string &id);
  ref_ptr<Camera> getCamera(const string &id);
  ref_ptr<Light> getLight(const string &id);
  ref_ptr<ShadowMap> getShadowMap(const string &id);
  vector< ref_ptr<Mesh> > getMesh(const string &id);
  ref_ptr<AssimpImporter> getAsset(const string &id);
  ref_ptr<StateNode> getNode(const string &id);
  vector<BoneAnimRange> getAnimationRanges(const string &assetId);

  rapidxml::xml_node<>* getXMLNode(const string &id);

  void processDocument(
      const ref_ptr<StateNode> &parent,
      const string &nodeId="root");

private:
  struct ShadowCaster {
    ref_ptr<StateNode> node;
    vector<string> targets;
  };

  Application *app_;

  rapidxml::xml_document<> doc_;
  string inputFile_;
  ifstream xmlInput_;
  vector<char> buffer_;

  ref_ptr<BulletPhysics> physics_;
  map<string, ref_ptr<StateNode> > nodes_;
  map<string, ref_ptr<regen::Font> > fonts_;
  map<string, ref_ptr<FBO> > fbos_;
  map<string, ref_ptr<Texture> > textures_;
  map<string, ref_ptr<Light> > lights_;
  map<string, ref_ptr<SkyScattering> > skys_;
  map<string, ref_ptr<Camera> > cameras_;
  map<string, ref_ptr<ShadowMap> > shadowMaps_;
  map<string, ref_ptr<AssimpImporter> > assets_;
  map<string, vector< ref_ptr<Mesh> > > meshes_;
  map<string, ref_ptr<FilterSequence> > filter_;
  list< ref_ptr<ShadowCaster> > shadowCaster_;
  set< Mesh* > usedMeshes_;

  void processDefineNode(
      const ref_ptr<State> &state,
      rapidxml::xml_node<> *xmlNode);
  void processToggleNode(
      const ref_ptr<State> &state,
      rapidxml::xml_node<> *xmlNode);
  void processInputNode(
      const ref_ptr<State> &state,
      rapidxml::xml_node<> *xmlNode);
  void processFBONode(
      const ref_ptr<State> &state,
      rapidxml::xml_node<> *xmlNode);
  void processTextureNode(
      const ref_ptr<State> &state,
      rapidxml::xml_node<> *xmlNode);
  void processBlitNode(
      const ref_ptr<State> &state,
      rapidxml::xml_node<> *xmlNode);
  void processBlendNode(
      const ref_ptr<State> &state,
      rapidxml::xml_node<> *xmlNode);
  void processDepthNode(
      const ref_ptr<State> &state,
      rapidxml::xml_node<> *xmlNode);
  void processTransformNode(
      const ref_ptr<State> &state,
      rapidxml::xml_node<> *xmlNode);
  void processCullNode(
      const ref_ptr<State> &state,
      rapidxml::xml_node<> *xmlNode);
  void processCameraNode(
      const ref_ptr<State> &state,
      rapidxml::xml_node<> *xmlNode);
  void processMaterialNode(
      const ref_ptr<State> &state,
      rapidxml::xml_node<> *xmlNode);

  void processNode(
      const ref_ptr<StateNode> &parent,
      rapidxml::xml_node<> *xmlNode,
      const ref_ptr<State> &state=ref_ptr<State>::alloc());
  void processStateNode(
      const ref_ptr<State> &state,
      rapidxml::xml_node<> *n);

  void processMeshNode(
      const ref_ptr<StateNode> &parent,
      rapidxml::xml_node<> *xmlNode);
  void processFullscreenPassNode(
      const ref_ptr<StateNode> &parent,
      rapidxml::xml_node<> *xmlNode);
  void processLightPassNode(
      const ref_ptr<StateNode> &parent,
      rapidxml::xml_node<> *xmlNode);
  void processDirectShadingNode(
      const ref_ptr<StateNode> &parent,
      rapidxml::xml_node<> *xmlNode);
  void processStateSequenceNode(
      const ref_ptr<StateNode> &parent,
      rapidxml::xml_node<> *xmlNode);
  void processFilterSequenceNode(
          const ref_ptr<StateNode> &parent,
          rapidxml::xml_node<> *xmlNode);

  vector< ref_ptr<Mesh> > createMesh(
      rapidxml::xml_node<> *n);
  ref_ptr<AssimpImporter> createAsset(
      rapidxml::xml_node<> *n);
  vector< ref_ptr<Mesh> > createAssetMeshes(
      const ref_ptr<AssimpImporter> &importer, rapidxml::xml_node<> *n);
  ref_ptr<Particles> createParticleMesh(rapidxml::xml_node<> *n,
      const GLuint numParticles, const string &updateShader);
  ref_ptr<TextureMappedText> createTextMesh(rapidxml::xml_node<> *n);
  ref_ptr<SkyScattering> createSky(rapidxml::xml_node<> *n);

  ref_ptr<regen::Font> createFont(rapidxml::xml_node<> *n);
  ref_ptr<FBO> createFBO(rapidxml::xml_node<> *n);
  ref_ptr<Texture> createTexture(rapidxml::xml_node<> *n);
  ref_ptr<Camera> createCamera(rapidxml::xml_node<> *n);
  ref_ptr<Light> createLight(rapidxml::xml_node<> *n);
  ref_ptr<ShadowMap> createShadowMap(rapidxml::xml_node<> *n);

  void addShadowCaster(
      const ref_ptr<StateNode> &node,
      const string &shadowMapIds);
};

#endif /* SCENE_PROCESSOR_XML_H_ */
