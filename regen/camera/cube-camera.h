/*
 * cube-camera.h
 *
 *  Created on: Dec 15, 2013
 *      Author: daniel
 */

#ifndef CUBE_CAMERA_H_
#define CUBE_CAMERA_H_

#include <regen/camera/camera.h>
#include "regen/states/blend-state.h"

namespace regen {
	/**
	 * A camera with n layer looking at n cube faces.
	 */
	class CubeCamera : public Camera {
	public:
		/**
		 * Cube face enumeration.
		 */
		enum Face {
			POS_X = 1 << 0,
			NEG_X = 1 << 1,
			POS_Y = 1 << 2,
			NEG_Y = 1 << 3,
			POS_Z = 1 << 4,
			NEG_Z = 1 << 5
		};

		/**
		 * @param hiddenFacesMask the mask of hidden faces.
		 */
		explicit CubeCamera(int hiddenFacesMask=0);

		/**
		 * @param face a cube face index (0-5).
		 * @return true if the cube face is visible.
		 */
		bool isCubeFaceVisible(int face) const;

	protected:
		unsigned int posStamp_ = 0;
		int hiddenFacesMask_;

		bool updateView() override;

		void updateViewProjection1() override;
	};

	std::ostream &operator<<(std::ostream &out, const CubeCamera::Face &v);

	std::istream &operator>>(std::istream &in, CubeCamera::Face &v);
} // namespace

#endif /* CUBE_CAMERA_H_ */
