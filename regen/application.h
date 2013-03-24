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

namespace ogle {
/**
 * \brief Provides a render tree and keyboard/mouse events.
 */
class Application : public EventObject
{
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
  struct KeyEvent {
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
  struct ButtonEvent {
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
  struct MouseMotionEvent {
    /** time difference to last motion event. */
    GLdouble dt;
    /** mouse x position difference */
    GLint dx;
    /** mouse y position difference */
    GLint dy;
  };
  /** Resize event. */
  static GLuint RESIZE_EVENT;

  /**
   * @param argc argument count.
   * @param argv array of arguments.
   */
  Application(int &argc, char** argv);

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
  GLboolean addShaderPath(const boost::filesystem::path &path);

  /**
   * @param ext a required extension that will be checked when GL is initialized.
   */
  void addRequiredExtension(const string &ext);
  /**
   * @param ext an optional extension that will be checked when GL is initialized.
   */
  void addOptionalExtension(const string &ext);

  /**
   * @return the application render tree.
   */
  const ref_ptr<RootNode>& renderTree() const;
  /**
   * @return the application render state.
   */
  const ref_ptr<RenderState>& renderState() const;
  /**
   * @return the window size.
   */
  const ref_ptr<ShaderInput2i>& windowViewport() const;
  /**
   * @return the current mouse position relative to GL window.
   */
  const ref_ptr<ShaderInput2f>& mousePosition() const;

  /**
   * Emit mouse-move event.
   * @param pos the event position.
   */
  void mouseMove(const Vec2i &pos);
  /**
   * Emit mouse-button event.
   * @param event the event.
   */
  void mouseButton(const ButtonEvent &event);
  /**
   * Emit key-up event.
   * @param event the event.
   */
  void keyUp(const KeyEvent &event);
  /**
   * Emit key-down event.
   * @param event the event.
   */
  void keyDown(const KeyEvent &event);

protected:
  ref_ptr<RootNode> renderTree_;
  ref_ptr<RenderState> renderState_;

  list<string> requiredExt_;
  list<string> optionalExt_;

  ref_ptr<ShaderInput2i> windowViewport_;

  ref_ptr<ShaderInput2f> mousePosition_;

  boost::posix_time::ptime lastMotionTime_;
  boost::posix_time::ptime lastDisplayTime_;
  boost::posix_time::ptime lastUpdateTime_;

  GLboolean isGLInitialized_;

  GLboolean setupShaderLoading();

  void initGL();
  void drawGL();
  void updateGL();
  void resizeGL(const Vec2i &size);
};
} // namespace

#endif // ___APPLICATION_H_

