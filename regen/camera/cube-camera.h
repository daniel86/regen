#ifndef REGEN_CUBE_CAMERA_H_
#define REGEN_CUBE_CAMERA_H_

#include <regen/camera/camera.h>
#include <regen/camera/light-camera.h>

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

	/**
	 * A camera for a point light source represented by a cubic frustum.
	 */
	class LightCamera_Cube : public CubeCamera, public LightCamera {
	public:
		/**
		 * @param light the light source.
		 * @param hiddenFacesMask the mask of hidden faces.
		 */
		explicit LightCamera_Cube(const ref_ptr<Light> &light, int hiddenFacesMask=0);

		// override
		bool updateLight() override;

	protected:
		bool updateCubeLight();
	private:
		bool updateLightProjection();
		bool updateLightView();
	};

	std::ostream &operator<<(std::ostream &out, const CubeCamera::Face &v);

	std::istream &operator>>(std::istream &in, CubeCamera::Face &v);
} // namespace

#endif /* REGEN_CUBE_CAMERA_H_ */
