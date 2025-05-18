#ifndef REGEN_ARRAY_CAMERA_H
#define REGEN_ARRAY_CAMERA_H

#include <regen/camera/camera.h>

namespace regen {
	class ArrayCamera : public Camera {
	public:
		/**
		 * @param numLayer the number of layers.
		 */
		explicit ArrayCamera(unsigned int numLayer);
	};
} // namespace

#endif /* REGEN_ARRAY_CAMERA_H */
