/*
 * test-camera-manipulator.h
 *
 *  Created on: 15.10.2012
 *      Author: daniel
 */

#ifndef TEST_CAMERA_MANIPULATOR_H_
#define TEST_CAMERA_MANIPULATOR_H_

#include <ogle/animations/camera-manipulator.h>
#include <ogle/states/camera.h>

#include <applications/ogle-application.h>

class TestCamManipulator : public LookAtCameraManipulator
{
public:
  TestCamManipulator(OGLEApplication &app,
      ref_ptr<PerspectiveCamera> perspectiveCamera);
};

#endif /* TEST_CAMERA_MANIPULATOR_H_ */
