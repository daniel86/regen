/*
 * application.h
 *
 *  Created on: 15.10.2012
 *      Author: daniel
 */

#ifndef ___APPLICATION_H_
#define ___APPLICATION_H_

#include <boost/filesystem.hpp>

#include <regen/utility/event-object.h>
#include <regen/states/state-node.h>
#include "regen/states/geometric-picking.h"
#include "regen/scene/scene-interaction.h"

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
	class Application : public EventObject {
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
		static GLuint KEY_EVENT;

		/** keyboard event data. */
		class KeyEvent : public EventData {
		public:
			/** key up or down ?. */
			GLboolean isUp;
			/** mouse x position. */
			GLint x;
			/** mouse y position. */
			GLint y;
			/** the pressed key. */
			GLint key;
		};

		/** mouse button event id. */
		static GLuint BUTTON_EVENT;

		/** mouse button event data. */
		class ButtonEvent : public EventData {
		public:
			/** pressed or released? */
			GLboolean pressed;
			/** is it a double click event? */
			GLboolean isDoubleClick;
			/** the mouse button. */
			GLint button;
			/** mouse x position. */
			GLint x;
			/** mouse y position. */
			GLint y;
		};

		/** mouse motion event id. */
		static GLuint MOUSE_MOTION_EVENT;

		/** mouse motion event data. */
		class MouseMotionEvent : public EventData {
		public:
			/** time difference to last motion event. */
			GLdouble dt;
			/** mouse x position difference */
			GLint dx;
			/** mouse y position difference */
			GLint dy;
		};

		/** Resize event. */
		static GLuint RESIZE_EVENT;

		/** mouse left/entered the window. */
		class MouseLeaveEvent : public EventData {
		public:
			/** mouse left/entered the window. */
			GLboolean entered;
		};

		/** mouse left/entered the window. */
		static GLuint MOUSE_LEAVE_EVENT;

		/**
		 * @param argc argument count.
		 * @param argv array of arguments.
		 */
		Application(const int &argc, const char **argv);

		/**
		 * @return true if GL context is ready to be used.
		 */
		GLboolean isGLInitialized() const;

		/**
		 * Initialize default loggers.
		 */
		void setupLogging();

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
		const ref_ptr<RootNode> &renderTree() const;

		/**
		 * @return the window size.
		 */
		const ref_ptr<ShaderInput2i> &windowViewport() const;

		/**
		 * @return the current mouse position relative to GL window.
		 */
		const ref_ptr<ShaderInput2f> &mousePosition() const;

		/**
		 * @return the current mouse position in range [0,1].
		 */
		const ref_ptr<ShaderInput2f> &mouseTexco() const;

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
		const ref_ptr<ShaderInput1i> isMouseEntered() const;

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
		auto& getObjectWithName(const std::string &name) const { return namedToObject_.at(name); }

		/**
		 * @param id The id of the object.
		 * @return The object or a null reference.
		 */
		auto& getObjectWithID(const int &id) const { return idToObject_.at(id); }

		/**
		 * @return The named objects.
		 */
		auto& namedObjects() const { return namedToObject_; }

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
		void setHoveredObject(const ref_ptr<StateNode> &obj, const GeomPicking::PickData *pickData);

		/**
		 * Unsets the hovered object.
		 */
		void unsetHoveredObject();

		/**
		 * @return the hovered object.
		 */
		auto& hoveredObject() const { return hoveredObject_; }

		/**
		 * @return the hovered object picking data.
		 */
		auto& hoveredObjectPickData() const { return hoveredObjectPickData_; }

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
		 * Clears application render tree to be empty.
		 */
		void clear();

	protected:
		ref_ptr<RootNode> renderTree_;
		std::map<std::string, NamedObject> namedToObject_;
		std::map<int, ref_ptr<StateNode>> idToObject_;
		std::map<std::string, ref_ptr<SceneInteraction>> interactions_;

		RenderState *renderState_;
		ref_ptr<StateNode> hoveredObject_;
		GeomPicking::PickData hoveredObjectPickData_;

		std::list<std::string> requiredExt_;
		std::list<std::string> optionalExt_;

		ref_ptr<ShaderInput2i> windowViewport_;

		ref_ptr<ShaderInput1i> isMouseEntered_;
		ref_ptr<ShaderInput2f> mousePosition_;
		ref_ptr<ShaderInput2f> mouseTexco_;

		boost::posix_time::ptime lastMotionTime_;
		boost::posix_time::ptime lastDisplayTime_;
		boost::posix_time::ptime lastUpdateTime_;

		GLboolean isGLInitialized_;

		void setupShaderLoading();

		void initGL();

		void drawGL();

		void updateGL();

		void resizeGL(const Vec2i &size);

		void updateMousePosition();
	};

} // namespace

#endif // ___APPLICATION_H_

