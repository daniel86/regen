/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef REGEN_CAMERA_ANCHOR_H
#define REGEN_CAMERA_ANCHOR_H

#include "regen/math/vector.h"
#include "regen/states/model-transformation.h"

namespace regen {
	class CameraAnchor {
	public:
		CameraAnchor() = default;

		virtual ~CameraAnchor() = default;

		virtual Vec3f position() = 0;

		virtual Vec3f direction() = 0;
	};

	class FixedCameraAnchor : public CameraAnchor {
	public:
		FixedCameraAnchor(const Vec3f &position, const Vec3f &direction)
				: position_(position), direction_(direction) {}

		~FixedCameraAnchor() override = default;

		Vec3f position() override { return position_; }

		Vec3f direction() override { return direction_; }

	protected:
		Vec3f position_;
		Vec3f direction_;
	};

	class TransformCameraAnchor : public CameraAnchor {
	public:
		enum Mode {
			LOOK_AT_FRONT,
			LOOK_AT_BACK,
			LOOK_AT_TOP
		};

		explicit TransformCameraAnchor(const ref_ptr<ModelTransformation> &transform);

		~TransformCameraAnchor() override = default;

		auto &mode() const { return mode_; }

		void setOffset(const Vec3f &offset) { offset_ = offset; }

		void setMode(Mode mode) { mode_ = mode; }

		Vec3f position() override;

		Vec3f direction() override;

	protected:
		ref_ptr<ModelTransformation> transform_;
		Vec3f offset_;
		Mode mode_;
	};
}

#endif //REGEN_CAMERA_ANCHOR_H
