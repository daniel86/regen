/*
 * test-camera-manipulator.cpp
 *
 *  Created on: 15.10.2012
 *      Author: daniel
 */

#include "test-camera-manipulator.h"

class RenderTreeMotionEvent : public EventCallable
{
public:
  RenderTreeMotionEvent(
      TestCamManipulator *camManipulator,
      GLboolean &buttonPressed)
  : EventCallable(),
    camManipulator_(camManipulator),
    buttonPressed_(buttonPressed)
  {}
  virtual void call(EventObject *evObject, void *data)
  {
    // OGLEApplication *app = (OGLEApplication*)evObject;
    OGLEApplication::MouseMotionEvent *ev =
        (OGLEApplication::MouseMotionEvent*)data;
    if(buttonPressed_) {
      camManipulator_->set_height(
          camManipulator_->height() + ((float)ev->dy)*0.02f, ev->dt );
      camManipulator_->setStepLength( ((float)ev->dx)*0.001f, ev->dt );
    }
  }
  TestCamManipulator *camManipulator_;
  const GLboolean &buttonPressed_;
};
class CameraButtonEvent : public EventCallable
{
public:
  CameraButtonEvent(TestCamManipulator *camManipulator)
  : EventCallable(),
    camManipulator_(camManipulator),
    buttonPressed_(GL_FALSE)
  {}
  virtual void call(EventObject *evObject, void *data)
  {
    OGLEApplication::ButtonEvent *ev =
        (OGLEApplication::ButtonEvent*)data;

    if(ev->button == 0) {
      buttonPressed_ = ev->pressed;
      if(ev->pressed) {
        camManipulator_->setStepLength( 0.0f );
      }
      } else if (ev->button == 4 && !ev->pressed) {
        camManipulator_->set_radius( camManipulator_->radius()+0.1f );
      } else if (ev->button == 3 && !ev->pressed) {
        camManipulator_->set_radius( camManipulator_->radius()-0.1f );
    }
  }
  TestCamManipulator *camManipulator_;
  GLboolean buttonPressed_;
};

TestCamManipulator::TestCamManipulator(
    OGLEApplication &app,
    ref_ptr<PerspectiveCamera> perspectiveCamera)
: LookAtCameraManipulator(perspectiveCamera, 10)
{
  set_height( 0.0f );
  set_lookAt( Vec3f(0.0f) );
  set_radius( 5.0f );
  set_degree( 0.0f );
  setStepLength( M_PI*0.01 );

  ref_ptr<CameraButtonEvent> buttonCallable = ref_ptr<CameraButtonEvent>::manage(
      new CameraButtonEvent(this));
  ref_ptr<RenderTreeMotionEvent> mouseMotionCallable = ref_ptr<RenderTreeMotionEvent>::manage(
      new RenderTreeMotionEvent(this, buttonCallable->buttonPressed_));

  app.connect(OGLEApplication::BUTTON_EVENT, ref_ptr<EventCallable>::cast(buttonCallable));
  app.connect(OGLEApplication::MOUSE_MOTION_EVENT, ref_ptr<EventCallable>::cast(mouseMotionCallable));
}
