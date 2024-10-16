/*
 * qt-camera-events.h
 *
 *  Created on: Oct 26, 2014
 *      Author: daniel
 */

#ifndef QT_CAMERA_EVENTS_H_
#define QT_CAMERA_EVENTS_H_

#include <regen/camera/camera-manipulator.h>

namespace regen {
	/**
	 * Simple event handler used for first person and third person cameras.
	 */
	class QtFirstPersonEventHandler : public EventHandler {
	public:
		QtFirstPersonEventHandler(const ref_ptr<FirstPersonCameraTransform> &m);

		/**
		 * @param val the camera sensitivity
		 */
		void set_sensitivity(GLfloat val);

		// Override
		void call(EventObject *evObject, EventData *data);

	protected:
		ref_ptr<FirstPersonCameraTransform> m_;
		GLboolean buttonPressed_;
		GLfloat sensitivity_;
	};
}


#endif /* QT_CAMERA_EVENTS_H_ */
