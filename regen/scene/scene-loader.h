#ifndef REGEN_SCENE_LOADER_H_
#define REGEN_SCENE_LOADER_H_

#include <string>

#include <regen/states/state-node.h>
#include <regen/scene/scene-input.h>
#include "regen/utility/event-object.h"
#include "regen/utility/ref-ptr.h"
#include "regen/scene/resource.h"
#include "regen/states/state.h"
#include "scene-input.h"
#include "regen/states/state-node.h"
#include "scene-processors.h"
#include "regen/physics/bullet-physics.h"
#include "regen/application.h"

namespace regen::scene {
	// forward declaration due to circular dependency.
	class ResourceManager;

	/**
	 * A named animation range.
	 * Unit might be ticks.
	 */
	struct AnimRange {
		/**
		 * Default constructor.
		 */
		AnimRange() : range(Vec2d(0.0, 0.0)) {}

		/**
		 * Constructor with name and range.
		 * @param n The range name.
		 * @param r The range value.
		 * @param i The animation index.
		 */
		AnimRange(std::string_view n, const Vec2d &r, GLuint i = 0)
				: name(n), range(r), channelIndex(i) {}

		/**
		 * Constructor with name, range and channel name.
		 * @param n The range name.
		 * @param r The range value.
		 * @param channelName The channel name.
		 */
		AnimRange(std::string_view n, const Vec2d &r, std::string channelName)
				: name(n), range(r), channelName(channelName) {}

		/** The range name. */
		std::string name;
		/** The range value. */
		Vec2d range;
		/** The animation index. */
		GLuint channelIndex = 0;
		/** The channel name. */
		std::string channelName;
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
	class SceneLoader {
	public:
		/**
		 * Default constructor.
		 * @param application Application that provides window-size, resize events, ....
		 * @param inputProvider The Scene input.
		 */
		SceneLoader(
				Application *application,
				const ref_ptr<SceneInput> &inputProvider);

		/**
		 * Constructor with user-defined resource manager and physics framework.
		 * @param application Application that provides window-size, resize events, ....
		 * @param inputProvider The Scene input.
		 * @param resources The resource manager instance.
		 * @param physics The physics framework instance.
		 */
		SceneLoader(
				Application *application,
				const ref_ptr<SceneInput> &inputProvider,
				const ref_ptr<ResourceManager> &resources,
				const ref_ptr<BulletPhysics> &physics);

		/**
		 * @return The application instance.
		 */
		Application *application() const { return application_; }

		/**
		 * @return The scene graph root node, is never null.
		 */
		ref_ptr<SceneInputNode> getRoot() const;

		/**
		 * @return The window viewport.
		 */
		const ref_ptr<ShaderInput2i> &getViewport() const;

		/**
		 * @return The mouse position.
		 */
		const ref_ptr<ShaderInput2f> &getMouseTexco() const;

		/**
		 * Add Application event handler.
		 * @param eventID The Application event id.
		 * @param eventHandler The EventHandler.
		 */
		void addEventHandler(GLuint eventID, const ref_ptr<EventHandler> &eventHandler);

		/**
		 * Note: Event handlers are not automatically removed.
		 * @return list of event handlers used by resources and nodes.
		 */
		auto &getEventHandler() const { return eventHandler_; }

		/**
		 * Node processors may create physical objects in the physics engine.
		 * @return The associated physics framework instance.
		 */
		ref_ptr<BulletPhysics> getPhysics() const { return physics_; }

		/**
		 * @return The ResourceManager instance.
		 */
		auto &getResources() const { return resources_; }

		/**
		 * Process input node with given category and name.
		 * Only Node processors considered for toplevel node.
		 * @param parent The parent StateNode.
		 * @param nodeName Name of the node.
		 * @param nodeCategory Category of the node.
		 */
		void processNode(
				const ref_ptr<StateNode> &parent,
				const std::string &nodeName,
				const std::string &nodeCategory);

		void processNode(const ref_ptr<StateNode> &parent, const std::string &nodeName) {
			processNode(parent, nodeName, "node");
		}

		void processNode(const ref_ptr<StateNode> &parent) {
			processNode(parent, "root", "node");
		}

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
		auto &nodeProcessors() const { return nodeProcessors_; }

		/**
		 * @return Category-State processor map.
		 */
		auto &stateProcessors() const { return stateProcessors_; }

		/**
		 * @param category The processor category.
		 * @return A StateNode processor or a null reference.
		 */
		ref_ptr<NodeProcessor> getNodeProcessor(const std::string &category);

		/**
		 * @param category The processor category.
		 * @return A State processor or a null reference.
		 */
		ref_ptr<StateProcessor> getStateProcessor(const std::string &category);

		/**
		 * @param assetID the AssetImporter resource id.
		 * @return Animation ranges associated to asset.
		 */
		std::vector<AnimRange> getAnimationRanges(const std::string &assetID);

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
		ref_ptr<State> getState(const std::string &id) const;

		/**
		 * @param name The name of the object.
		 * @param obj The object.
		 */
		int putNamedObject(const ref_ptr<StateNode> &obj);

		ref_ptr<Resource> getResource(const std::string &type, const std::string &name);

		template<class ResourceType>
		ref_ptr<ResourceType> getResource(const std::string &name) {
			return ref_ptr<ResourceType>::dynamicCast(
					getResource(ResourceType::TYPE_NAME, name));
		}

		void putResource(const std::string &type, const std::string &name, const ref_ptr<Resource> &v);

		template<class ResourceType>
		void putResource(const std::string &name, const ref_ptr<ResourceType> &v) {
			return putResource(ResourceType::TYPE_NAME, name, v);
		}

		void loadResources(const std::string &name);

	protected:
		Application *application_;
		std::list<ref_ptr<EventHandler> > eventHandler_;

		ref_ptr<SceneInput> inputProvider_;
		std::map<std::string, ref_ptr<NodeProcessor> > nodeProcessors_;
		std::map<std::string, ref_ptr<StateProcessor> > stateProcessors_;
		ref_ptr<ResourceManager> resources_;
		std::map<std::string, ref_ptr<StateNode> > nodes_;
		std::map<std::string, ref_ptr<State> > states_;
		ref_ptr<BulletPhysics> physics_;

		void init();

		void loadShapes();

		void processState(
				const ref_ptr<StateNode> &parentNode,
				const ref_ptr<State> &parentState,
				const std::string &nodeCategory,
				const ref_ptr<SceneInputNode> &input);
	};
}


#endif /* REGEN_SCENE_LOADER_H_ */
