#ifndef REGEN_SCENE_H
#define REGEN_SCENE_H

#include <boost/filesystem.hpp>

#include <regen/utility/event-object.h>
#include <regen/utility/time.h>
#include <regen/scene/state-node.h>
#include "regen/passes/pick-data.h"
#include "regen/scene/scene-interaction.h"
#include "regen/animation/animation.h"
#include "regen/memory/staging-system.h"
#include "../textures/screen.h"
#include "regen/behavior/world/world-model.h"

// Defeat evil windows defines...
#ifdef KEY_EVENT
#undef KEY_EVENT
#endif
#ifdef BUTTON_EVENT
#undef BUTTON_EVENT
#endif
#ifdef MOUSE_MOTION_EVENT
#undef MOUSE_MOTION_EVENT
#endif
#ifdef RESIZE_EVENT
#undef RESIZE_EVENT
#endif

namespace regen {
	/**
	 * \brief Provides a render tree and keyboard/mouse events.
	 */
	class Scene : public EventObject {
	public:
		/**
		 * Identifies mouse buttons.
		 */
		enum Button {
			MOUSE_BUTTON_LEFT = 0,//!< left-click
			MOUSE_BUTTON_RIGHT,   //!< right-click
			MOUSE_BUTTON_MIDDLE,  //!< wheel pressed
			MOUSE_WHEEL_UP,       //!< wheel-up
			MOUSE_WHEEL_DOWN      //!< wheel-down
		};

		/** keyboard event id. */
		static uint32_t KEY_EVENT;

		/** keyboard event data. */
		class KeyEvent : public EventData {
		public:
			~KeyEvent() override = default;
			/** key up or down ?. */
			bool isUp;
			/** mouse x position. */
			int x;
			/** mouse y position. */
			int y;
			/** the pressed key. */
			int key;
		};

		/** mouse button event id. */
		static uint32_t BUTTON_EVENT;

		/** mouse button event data. */
		class ButtonEvent : public EventData {
		public:
			~ButtonEvent() override = default;
			/** pressed or released? */
			bool pressed;
			/** is it a double click event? */
			bool isDoubleClick;
			/** the mouse button. */
			int button;
			/** mouse x position. */
			int x;
			/** mouse y position. */
			int y;
		};

		/** mouse motion event id. */
		static uint32_t MOUSE_MOTION_EVENT;

		/** mouse motion event data. */
		class MouseMotionEvent : public EventData {
		public:
			~MouseMotionEvent() override = default;
			/** time difference to last motion event. */
			double dt;
			/** mouse x position difference */
			int dx;
			/** mouse y position difference */
			int dy;
		};

		/** Resize event. */
		static uint32_t RESIZE_EVENT;

		/** mouse left/entered the window. */
		class MouseLeaveEvent : public EventData {
		public:
			~MouseLeaveEvent() override = default;
			/** mouse left/entered the window. */
			bool entered;
		};

		/** mouse left/entered the window. */
		static uint32_t MOUSE_LEAVE_EVENT;

		/**
		 * @param argc argument count.
		 * @param argv array of arguments.
		 */
		Scene(const int &argc, const char **argv);

		/**
		 * Sets the world model.
		 * @param worldModel the world model.
		 */
		void setWorldModel(const ref_ptr<WorldModel> &worldModel) { worldModel_ = worldModel; }

		/**
		 * @return the world model.
		 */
		const ref_ptr<WorldModel> &worldModel() const { return worldModel_; }

		/**
		 * Initialize the scene after the render tree is loaded.
		 */
		void initializeScene();

		/**
		 * Clears application render tree to be empty.
		 */
		void clear();

		/**
		 * @return true if GL context is ready to be used.
		 */
		auto isGLInitialized() const { return isGLInitialized_; }

		/**
		 * @return true if vsync is enabled.
		 */
		auto isVSyncEnabled() const { return isVSyncEnabled_; }

		/**
		 * @param enabled enable/disable vsync.
		 */
		auto setVSyncEnabled(const bool &enabled) { isVSyncEnabled_ = enabled; }

		/**
		 * Initialize default loggers.
		 */
		static void setupLogging();

		/**
		 * Adds a path to the list of paths to be searched when the include directive
		 * is used in shaders.
		 * @param path the include path
		 * @return true on success
		 */
		void addShaderPath(const std::string &path);

		/**
		 * @param ext a required extension that will be checked when GL is initialized.
		 */
		void addRequiredExtension(const std::string &ext);

		/**
		 * @param ext an optional extension that will be checked when GL is initialized.
		 */
		void addOptionalExtension(const std::string &ext);

		/**
		 * @return the application render tree.
		 */
		auto &renderTree() const { return renderTree_; }

		/**
		 * @return the window size.
		 */
		const ref_ptr<Screen> &screen() const { return screen_; }

		/**
		 * @return the current mouse position relative to GL window.
		 */
		auto &mousePosition() const { return mousePosition_; }

		/**
		 * @return the current mouse position in range [0,1].
		 */
		auto &mouseTexco() const { return mouseTexco_; }

		/**
		 * @return the current mouse depth in view space.
		 */
		auto &mouseDepth() const { return mouseDepth_; }

		/**
		 * Queue emit MOUSE_LEAVE_EVENT event.
		 */
		void mouseEnter();

		/**
		 * Queue emit MOUSE_LEAVE_EVENT event.
		 */
		void mouseLeave();

		/**
		 * @return mouse left/entered the window.
		 */
		ref_ptr<ShaderInput1i> isMouseEntered() const;

		/**
		 * Queue mouse-move event.
		 * This function is tread safe.
		 * @param pos the event position.
		 */
		void mouseMove(const Vec2i &pos);

		/**
		 * Queue mouse-button event.
		 * This function is tread safe.
		 * @param event the event.
		 */
		void mouseButton(const ButtonEvent &event);

		/**
		 * Queue key-up event.
		 * This function is tread safe.
		 * @param event the event.
		 */
		void keyUp(const KeyEvent &event);

		/**
		 * Queue key-down event.
		 * This function is tread safe.
		 * @param event the event.
		 */
		void keyDown(const KeyEvent &event);

		/**
		 * @param name The name of the object.
		 * @return The object or a null reference.
		 */
		auto &getObjectWithName(const std::string &name) const { return namedToObject_.at(name); }

		/**
		 * @param id The id of the object.
		 * @return The object or a null reference.
		 */
		auto &getObjectWithID(const int &id) const { return idToObject_.at(id); }

		/**
		 * @return The named objects.
		 */
		auto &namedObjects() const { return namedToObject_; }

		/**
		 * @param name The name of the object.
		 * @param obj The object.
		 */
		int putNamedObject(const ref_ptr<StateNode> &obj);

		/**
		 * Sets the hovered object.
		 * @param obj the node of the object.
		 * @param pickData the picking data.
		 */
		void setHoveredObject(const ref_ptr<StateNode> &obj, const PickData *pickData);

		/**
		 * Unsets the hovered object.
		 */
		void unsetHoveredObject();

		/**
		 * @return the hovered object.
		 */
		auto &hoveredObject() const { return hoveredObject_; }

		/**
		 * @return the hovered object picking data.
		 */
		auto &hoveredObjectPickData() const { return hoveredObjectPickData_; }

		/**
		 * @return true if there is a hovered object.
		 */
		bool hasHoveredObject() const { return hoveredObject_.get() != nullptr; }

		/**
		 * @param name name of the node
		 * @param interaction interaction to register
		 */
		void registerInteraction(const std::string &name, const ref_ptr<SceneInteraction> &interaction);

		/**
		 * @param name name of the interaction
		 * @return the interaction or null
		 */
		ref_ptr<SceneInteraction> getInteraction(const std::string &name);

		/**
		 * Updates the time.
		 */
		void updateTime();

		/**
		 * @return the world time.
		 */
		const WorldTime &worldTime() const { return worldTime_; }

		/**
		 * @return the system time.
		 */
		const SystemTime &systemTime() const { return systemTime_; }

		/**
		 * Sets the world time scale.
		 * @param scale the scale.
		 */
		void setWorldTimeScale(const double &scale);

		/**
		 * Sets the world time.
		 */
		void setWorldTime(const time_t &t);

		/**
		 * Sets the world time.
		 */
		void setWorldTime(float t);

		/**
		 * Run some function within a thread with GL context.
		 * NOTE: Returns without waiting for the function to finish.
		 * @param f the function to run.
		 */
		void withGLContext(std::function<void()> f);

		/**
		 * Initializes GL resources of the scene.
		 */
		void initGL();

		/**
		 * Resizes FBOs that have window-relative size.
		 * @param size
		 */
		void resizeGL(const Vec2i &size);

		/**
		 * Draw next frame.
		 */
		void drawGL();

		/**
		 * Used to advance the GL state.
		 */
		void updateGL();

		/**
		 * Called each frame after rendering and GL animations.
		 */
		void flushGL();

	protected:
		ref_ptr<RootNode> renderTree_;
		std::map<std::string, NamedObject> namedToObject_;
		std::map<int, ref_ptr<StateNode>> idToObject_;
		std::map<std::string, ref_ptr<SceneInteraction>> interactions_;

		RenderState *renderState_;
		ref_ptr<StateNode> hoveredObject_;
		PickData hoveredObjectPickData_;

		std::list<std::string> requiredExt_;
		std::list<std::string> optionalExt_;
		std::vector<ref_ptr<Animation>> glCalls_;

		ref_ptr<Screen> screen_;
		ref_ptr<ShaderInput1i> isMouseEntered_;
		ref_ptr<ShaderInput2f> mousePosition_;
		ref_ptr<ShaderInput2f> mouseTexco_;
		ref_ptr<ShaderInput1f> mouseDepth_;
		ref_ptr<ShaderInput1f> timeDelta_;
		ref_ptr<UBO> globalUniforms_;

		ref_ptr<WorldModel> worldModel_;

		boost::posix_time::ptime lastMotionTime_;
		WorldTime worldTime_;
		SystemTime systemTime_;

		bool isGLInitialized_;
		bool isTimeInitialized_;
		bool isVSyncEnabled_;

		void setupShaderLoading();

		void setTime();

		void updateMousePosition();

		void updateBOs();
	};

} // namespace

#endif // REGEN_SCENE_H

