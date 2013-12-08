/*
 * scene-parser.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_PARSER_H_
#define REGEN_SCENE_PARSER_H_

#include <regen/states/state-node.h>
#include <regen/states/fbo-state.h>
#include <regen/meshes/mesh-state.h>
#include <regen/meshes/particles.h>
#include <regen/meshes/assimp-importer.h>
#include <regen/meshes/texture-mapped-text.h>
#include <regen/states/texture-state.h>
#include <regen/states/shadow-map.h>
#include <regen/physics/bullet-physics.h>
#include <regen/utility/ref-ptr.h>
#include <regen/utility/xml.h>
#include <regen/utility/font.h>
#include <regen/meshes/sky.h>
#include <regen/application.h>

#include <regen/scene/scene-input.h>

namespace regen {
namespace scene {
#ifndef MeshVector
  typedef vector< ref_ptr<Mesh> > MeshVector;
#endif
  // forward declaration due to circular dependency.
  class ResourceManager;
  class NodeProcessor;
  class StateProcessor;

  /**
   * A named animation range.
   * Unit might be ticks.
   */
  struct AnimRange {
    /**
     * Default constructor.
     */
    AnimRange()
    : name(""), range(Vec2d(0.0,0.0)) {}
    /**
     * @param n The range name.
     * @param r The range value.
     */
    AnimRange(const string &n, const Vec2d &r)
    : name(n), range(r) {}
    /** The range name. */
    string name;
    /** The range value. */
    Vec2d range;
  };

  /**
   * Allows processing of input resources.
   * Input resources can define resources and a scene graph
   * that references the resources.
   * SceneParser provides typed access to resources.
   * Scene graph processors can be added dynamically, however
   * a set of standard processors is added by default.
   * These standard processors support all features
   * build into the engine.
   */
  class SceneParser {
  public:
    /**
     * Default constructor.
     * @param application Application that provides window-size, resize events, ....
     * @param inputProvider The Scene input.
     */
    SceneParser(
        Application *application,
        const ref_ptr<SceneInput> &inputProvider);

    /**
     * Constructor with user-defined resource manager and physics framework.
     * @param application Application that provides window-size, resize events, ....
     * @param inputProvider The Scene input.
     * @param resources The resource manager instance.
     * @param physics The physics framework instance.
     */
    SceneParser(
        Application *application,
        const ref_ptr<SceneInput> &inputProvider,
        const ref_ptr<ResourceManager> &resources,
        const ref_ptr<BulletPhysics> &physics);

    /**
     * @return The scene graph root node, is never null.
     */
    ref_ptr<SceneInputNode> getRoot() const;

    /**
     * @return The window viewport.
     */
    const ref_ptr<ShaderInput2i>& getViewport() const;
    /**
     * Add Application event handler.
     * @param eventID The Application event id.
     * @param eventHandler The EventHandler.
     */
    void addEventHandler(GLuint eventID,
        const ref_ptr<EventHandler> &eventHandler);
    /**
     * Note: Event handlers are not automatically removed.
     * @return list of event handlers used by resources and nodes.
     */
    const list< ref_ptr<EventHandler> > getEventHandler() const;

    /**
     * Node processors may create physical objects in the physics engine.
     * @return The associated physics framework instance.
     */
    ref_ptr<BulletPhysics> getPhysics();
    /**
     * @return The ResourceManager instance.
     */
    ref_ptr<ResourceManager> getResources();

    /**
     * Process input node with given category and name.
     * Only Node processors considered for toplevel node.
     * @param parent The parent StateNode.
     * @param nodeName Name of the node.
     * @param nodeCategory Category of the node.
     */
    void processNode(
        const ref_ptr<StateNode> &parent,
        const string &nodeName="root",
        const string &nodeCategory="node");
    /**
     * Process input node with given category and name.
     * Only State processors considered.
     * @param parent The parent State.
     * @param nodeName Name of the node.
     * @param nodeCategory Category of the node.
     */
    void processState(
        const ref_ptr<State> &parent,
        const string &nodeName,
        const string &nodeCategory);

    /**
     * @param x The StateNode processor to use.
     */
    void setNodeProcessor(const ref_ptr<NodeProcessor> &x);
    /**
     * @param x The State processor to use.
     */
    void setStateProcessor(const ref_ptr<StateProcessor> &x);

    /**
     * @return Category-StateNode processor map.
     */
    const map<string, ref_ptr<NodeProcessor> >& nodeProcessors() const;
    /**
     * @return Category-State processor map.
     */
    const map<string, ref_ptr<StateProcessor> >& stateProcessors() const;

    /**
     * @param category The processor category.
     * @return A StateNode processor or a null reference.
     */
    ref_ptr<NodeProcessor> getNodeProcessor(const string &category);
    /**
     * @param category The processor category.
     * @return A State processor or a null reference.
     */
    ref_ptr<StateProcessor> getStateProcessor(const string &category);

    /**
     * @param assetID the AssetImporter resource id.
     * @return Animation ranges associated to asset.
     */
    vector<AnimRange> getAnimationRanges(const std::string &assetID);

    /**
     * @param id The StateNode ID.
     * @param node The StateNode instance.
     */
    void putNode(const std::string &id, const ref_ptr<StateNode> &node);
    /**
     * @param id The StateNode ID.
     * @return The StateNode instance or a null reference.
     */
    ref_ptr<StateNode> getNode(const std::string &id);

    /**
     * @param id The State ID.
     * @param state The State instance.
     */
    void putState(const std::string &id, const ref_ptr<State> &state);
    /**
     * @param id The State ID.
     * @return The State instance or a null reference.
     */
    ref_ptr<State> getState(const std::string &id);

  protected:
    Application *application_;
    list< ref_ptr<EventHandler> > eventHandler_;

    ref_ptr<SceneInput> inputProvider_;
    map<string, ref_ptr<NodeProcessor> > nodeProcessors_;
    map<string, ref_ptr<StateProcessor> > stateProcessors_;
    ref_ptr<ResourceManager> resources_;
    std::map< string, ref_ptr<StateNode> > nodes_;
    std::map< string, ref_ptr<State> > states_;
    ref_ptr<BulletPhysics> physics_;

    void init();
  };
}}


#endif /* REGEN_SCENE_PARSER_H_ */
