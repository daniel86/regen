#include "light-camera-csm.h"

using namespace regen;

#undef CSM_USE_SINGLE_VIEW

static inline Vec2f findZRange(const Vec3f &lightDir, const Vec3f *frustumPoints) {
    Vec2f range;
    range.x = std::numeric_limits<float>::max();
    range.y = std::numeric_limits<float>::lowest();

    for (GLint i = 0; i < 8; ++i) {
        float projection = frustumPoints[i].dot(lightDir);
        if (projection < range.x) {
            range.x = projection;
        }
        if (projection > range.y) {
            range.y = projection;
        }
    }

    return range;
}

static inline float findZValue(const Vec3f &lightDir, const Vec3f &point) {
	return point.dot(lightDir);
}

LightCamera_CSM::LightCamera_CSM(
		const ref_ptr<Light> &light,
		const ref_ptr<Camera> &userCamera,
		unsigned int numLayer)
		: Camera(numLayer),
		  LightCamera(light, this),
		  userCamera_(userCamera),
		  userCameraFrustum_(numLayer),
		  userFrustumCentroids_(numLayer) {
	shaderDefine("RENDER_TARGET", "2D_ARRAY");

	// Set matrix array size
#ifndef CSM_USE_SINGLE_VIEW
	view_->set_elementCount(numLayer_);
	view_->set_forceArray(true);
	view_->setUniformDataUntyped(nullptr);
	viewInv_->set_elementCount(numLayer_);
	viewInv_->set_forceArray(true);
	viewInv_->setUniformDataUntyped(nullptr);
#endif
	proj_->set_elementCount(numLayer_);
	proj_->set_forceArray(true);
	proj_->setUniformDataUntyped(nullptr);
	projInv_->set_elementCount(numLayer_);
	projInv_->set_forceArray(true);
	projInv_->setUniformDataUntyped(nullptr);

	viewProj_->set_elementCount(numLayer_);
	viewProj_->set_forceArray(true);
	viewProj_->setUniformDataUntyped(nullptr);
	viewProjInv_->set_elementCount(numLayer_);
	viewProjInv_->set_forceArray(true);
	viewProjInv_->setUniformDataUntyped(nullptr);

	near_->set_elementCount(numLayer_);
	near_->set_forceArray(true);
	near_->setUniformDataUntyped(nullptr);

	far_->set_elementCount(numLayer_);
	far_->set_forceArray(true);
	far_->setUniformDataUntyped(nullptr);

	position_->set_elementCount(numLayer_);
	position_->set_forceArray(true);
	position_->setUniformDataUntyped(nullptr);
	position_->setVertex(0, Vec3f(0.0f));

	lightMatrix_->set_elementCount(numLayer_);
	lightMatrix_->set_forceArray(true);
	lightMatrix_->setUniformDataUntyped(nullptr);
	setInput(lightMatrix_);

	updateDirectionalLight();
}

void LightCamera_CSM::setSplitWeight(GLdouble splitWeight) {
	splitWeight_ = splitWeight;
}

bool LightCamera_CSM::updateLight() {
	return updateDirectionalLight();
}

bool LightCamera_CSM::updateDirectionalLight() {
	auto changed = updateFrustumSplit();
	changed = updateLightView() || changed;
	changed = updateLightProjection() || changed;
	if (changed) {
		updateViewProjection1();
		// Transforms world space coordinates to homogenous light space
		for (int i = 0; i < lightMatrix_->elementCount(); ++i) {
			lightMatrix_->setVertex(i, viewProj_->getVertex(i) * Mat4f::bias());
		}
		camStamp_ += 1;
	}

	return changed;
}

bool LightCamera_CSM::updateFrustumSplit() {
	bool hasChanged = false;
	// Update near/far values when user camera projection changed
	auto userProjStamp = userCamera_->projection()->stamp();
	if (userProjectionStamp_ != userProjStamp) {
		userProjectionStamp_ = userProjStamp;
		const Mat4f &proj = userCamera_->projection()->getVertex(0);
		// update frustum splits
		userCamera_->frustum()[0].split(splitWeight_, userCameraFrustum_);
		// update near/far values
		auto *farValues = (GLfloat *) far_->clientDataPtr();
		auto *nearValues = (GLfloat *) near_->clientDataPtr();
		for (int i = 0; i < numLayer_; ++i) {
			auto &u_frustum = userCameraFrustum_[i];
			// frustum_->far() is originally in eye space - tell's us how far we can see.
			// Here we compute it in camera homogeneous coordinates. Basically, we calculate
			// proj * (0, 0, far, 1)^t and then normalize to [0; 1]
			// Note: this is used in shaders for computing z coordinate for shadow map lookup
			farValues[i] = 0.5 * (-u_frustum.far * proj(2, 2) + proj(3, 2)) / u_frustum.far + 0.5;
			nearValues[i] = 0.5 * (-u_frustum.near * proj(2, 2) + proj(3, 2)) / u_frustum.near + 0.5;
		}
		far_->nextStamp();
		near_->nextStamp();
		hasChanged = true;
	}

	// re-compute user frustum points if needed
	auto userPosStamp = userCamera_->position()->stamp();
	auto userDirStamp = userCamera_->direction()->stamp();
	if (hasChanged || userPosStamp != userPositionStamp_ || userDirStamp != userDirectionStamp_) {
		userPositionStamp_ = userPosStamp;
		userDirectionStamp_ = userDirStamp;
		auto &userPos = userCamera_->position()->getVertex(0);
		auto &userDir = userCamera_->direction()->getVertex(0);
		for (int i = 0; i < numLayer_; ++i) {
			userCameraFrustum_[i].update(userPos, userDir);
			userFrustumCentroids_[i] = userPos + userDir *
					((userCameraFrustum_[i].far - userCameraFrustum_[i].near) * 0.5f + userCameraFrustum_[i].near);
		}
		hasChanged = true;
	}

	return hasChanged;
}

bool LightCamera_CSM::updateLightView() {
	if (lightDirStamp_ == light_->direction()->stamp() && lightPosStamp_ == userPositionStamp_) { return false; }
	lightDirStamp_ = light_->direction()->stamp();
	lightPosStamp_ = userPositionStamp_;

	auto f = -light_->direction()->getVertex(0);
	f.normalize();
	direction_->setVertex(0, f);
#ifdef CSM_USE_SINGLE_VIEW
	// NOTE: The nvidia example uses a single view matrix for all layers with position at (0,0,0).
	// but for some reason I have some issues with this approach.
	// Others use the centroid, and for some reason a small offset, it is not hard
	// to compute the position such that the near plane is at 1.0 eg. but do not know
	// if this is advantageous.
	// Equivalent to getLookAtMatrix(pos=(0,0,0), dir=f, up=(-1,0,0))
	Vec3f s(0.0f, -f.z, f.y);
	s.normalize();
	view_->setVertex(0, Mat4f(
			0.0f, s.y * f.z - s.z * f.y, -f.x, 0.0f,
			s.y, s.z * f.x, -f.y, 0.0f,
			s.z, -s.y * f.x, -f.z, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
	));
	viewInv_->setVertex(0, view_->getVertex(0).lookAtInverse());
	for (int i = 0; i < numLayer_; ++i) {
		position_->setVertex(i, Vec3f::zero());
	}
#else
	for (int i = 0; i < numLayer_; ++i) {
#if 0
		auto &u_frustum = userCameraFrustum_[i];
		// compute the z-range of the frustum in light space as if light were positioned at (0,0,0)
		auto zRange = findZRange(f, u_frustum.points);
		// get z value of the centroid in light space as if light were positioned at (0,0,0)
		auto centroid_z = findZValue(f, userFrustumCentroids_[i]);
		// offset by min z such that we get a near value of 0.0 for the frustum points
		centroid_z -= zRange.x;
		// move near plane to 1.0
		centroid_z += 1.0f;
		position_->setVertex(i, userFrustumCentroids_[i] - f*centroid_z);
#else
		position_->setVertex(i, userFrustumCentroids_[i] - f*userCameraFrustum_[i].far);
#endif
		view_->setVertex(i, Mat4f::lookAtMatrix(
			position_->getVertex(i), f, Vec3f::up()));
		viewInv_->setVertex(i, view_->getVertex(i).lookAtInverse());
	}
#endif
	return true;
}

bool LightCamera_CSM::updateLightProjection() {
	// finally re-compute projection matrices, and frustum of light camera
	auto viewStamp = view_->stamp();
	if (viewStamp == viewStamp_) { return false; }
	viewStamp_ = viewStamp;

	for (int layerIndex = 0; layerIndex < numLayer_; ++layerIndex) {
		auto &u_frustum = userCameraFrustum_[layerIndex];
		Bounds<Vec3f> bounds_ls(
			std::numeric_limits<float>::max(),
			std::numeric_limits<float>::lowest());
		for (int frustumIndex = 0; frustumIndex < 8; ++frustumIndex) {
#ifdef CSM_USE_SINGLE_VIEW
			auto point_ls = view_->getVertex(0) ^
					Vec4f(u_frustum.points[frustumIndex], 1.0f);
#else
			auto point_ls = view_->getVertex(layerIndex) ^
					Vec4f(u_frustum.points[frustumIndex], 1.0f);
#endif
			bounds_ls.min.setMin(point_ls.xyz_());
			bounds_ls.max.setMax(point_ls.xyz_());
		}
		auto buf = bounds_ls.max;
		bounds_ls.max.z = -bounds_ls.min.z;
		bounds_ls.min.z = -buf.z;

		proj_->setVertex(layerIndex, Mat4f::orthogonalMatrix(
				bounds_ls.min.x, bounds_ls.max.x,
				bounds_ls.min.y, bounds_ls.max.y,
				bounds_ls.min.z, bounds_ls.max.z));
		projInv_->setVertex(layerIndex,
				proj_->getVertex(layerIndex).orthogonalInverse());
		frustum_[layerIndex].setOrtho(
				bounds_ls.min.x, bounds_ls.max.x,
				bounds_ls.min.y, bounds_ls.max.y,
				bounds_ls.min.z, bounds_ls.max.z);
		frustum_[layerIndex].update(
			position_->getVertex(layerIndex),
			direction_->getVertex(0));
	}

	return true;
}
