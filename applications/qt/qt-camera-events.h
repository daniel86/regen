/*
 * qt-camera-events.h
 *
 *  Created on: Oct 26, 2014
 *      Author: daniel
 */

#ifndef QT_CAMERA_EVENTS_H_
#define QT_CAMERA_EVENTS_H_

#include <regen/camera/camera-controller.h>

namespace regen {
	/**
	 * Simple event handler used for first person and third person cameras.
	 */
	class QtFirstPersonEventHandler : public EventHandler {
	public:
		explicit QtFirstPersonEventHandler(
				const ref_ptr<CameraController> &m,
				const std::vector<CameraCommandMapping> &keyMappings);

		/**
		 * @param val the camera sensitivity
		 */
		void set_sensitivity(GLfloat val);

		// Override
		void call(EventObject *evObject, EventData *data) override;

	protected:
		ref_ptr<CameraController> m_;
		std::map<std::string, CameraCommandMapping> keyMappings_;
		GLboolean buttonPressed_;
		GLfloat sensitivity_;
	};
}


#endif /* QT_CAMERA_EVENTS_H_ */
