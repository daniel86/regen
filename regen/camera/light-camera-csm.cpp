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
	shaderDefine("RENDER_TARGET_MODE", "CASCADE");

#ifndef CSM_USE_SINGLE_VIEW
	{
		viewData_.resize(numLayer * 2, Mat4f::identity());
		view_    = std::span<Mat4f>(viewData_).subspan(0, numLayer);
		viewInv_ = std::span<Mat4f>(viewData_).subspan(numLayer, numLayer);
		sh_view_->set_numArrayElements(numLayer_);
		sh_viewInv_->set_numArrayElements(numLayer_);
		sh_view_->set_forceArray(true);
		sh_viewInv_->set_forceArray(true);
		sh_view_->setUniformUntyped();
		sh_viewInv_->setUniformUntyped();
	}
#endif
	{
		viewProjData_.resize(numLayer * 2, Mat4f::identity());
		viewProj_    = std::span<Mat4f>(viewProjData_).subspan(0, numLayer);
		viewProjInv_ = std::span<Mat4f>(viewProjData_).subspan(numLayer, numLayer);
		sh_viewProj_->set_numArrayElements(numLayer_);
		sh_viewProjInv_->set_numArrayElements(numLayer_);
		sh_viewProj_->set_forceArray(true);
		sh_viewProjInv_->set_forceArray(true);
		sh_viewProj_->setUniformUntyped();
		sh_viewProjInv_->setUniformUntyped();
	}
	{
		projData_.resize(numLayer * 2, Mat4f::identity());
		proj_    = std::span<Mat4f>(projData_).subspan(0, numLayer);
		projInv_ = std::span<Mat4f>(projData_).subspan(numLayer, numLayer);
		sh_proj_->set_numArrayElements(numLayer_);
		sh_projInv_->set_numArrayElements(numLayer_);
		sh_proj_->set_forceArray(true);
		sh_projInv_->set_forceArray(true);
		sh_proj_->setUniformUntyped();
		sh_projInv_->setUniformUntyped();
	}

	projParams_.resize(numLayer);
	sh_projParams_->set_numArrayElements(numLayer_);
	sh_projParams_->set_forceArray(true);
	sh_projParams_->setUniformUntyped();

	position_.resize(numLayer, Vec4f::zero());
	sh_position_->set_numArrayElements(numLayer_);
	sh_position_->set_forceArray(true);
	sh_position_->setUniformUntyped();
	sh_position_->setVertex(0, Vec4f::zero());

	v_lightMatrix_.resize(numLayer, Mat4f::identity());
	sh_lightMatrix_->set_numArrayElements(numLayer_);
	sh_lightMatrix_->set_forceArray(true);
	sh_lightMatrix_->setUniformUntyped();
	setInput(shadowBuffer_);
	// these are needed to compute the CSM layer given a position
	setInput(sh_projParams_, "lightProjParams");
	setInput(userCamera_->cameraBlock(), "UserCamera", "_User");

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
		for (unsigned int i = 0; i < v_lightMatrix_.size(); ++i) {
			v_lightMatrix_[i] = viewProjection(i) * Mat4f::bias();
		}
		camStamp_ += 1;
	}

	return changed;
}

bool LightCamera_CSM::updateFrustumSplit() {
	bool hasChanged = false;
	// Update near/far values when user camera projection changed
	auto userProjStamp = userCamera_->projectionStamp();
	if (userProjectionStamp_ != userProjStamp) {
		userProjectionStamp_ = userProjStamp;
		auto &proj = userCamera_->projection(0);
		ProjectionParams projParams;
		// update frustum splits
		userCamera_->frustum()[0].split(splitWeight_, userCameraFrustum_);
		// update near/far values
		for (unsigned int i = 0; i < numLayer_; ++i) {
			auto &u_frustum = userCameraFrustum_[i];
			// frustum_->far() is originally in eye space - tell's us how far we can see.
			// Here we compute it in camera homogeneous coordinates. Basically, we calculate
			// proj * (0, 0, far, 1)^t and then normalize to [0; 1]
			// Note: this is used in shaders for computing z coordinate for shadow map lookup
			projParams.near = 0.5 * (-u_frustum.near * proj(2, 2) + proj(3, 2)) / u_frustum.near + 0.5;
			projParams.far  = 0.5 * (-u_frustum.far * proj(2, 2) + proj(3, 2)) / u_frustum.far + 0.5;
			projParams.aspect = proj(0, 0) / proj(1, 1);
			projParams.fov = 0.0f;
			setProjParams(i, projParams);
		}
		hasChanged = true;
	}

	// re-compute user frustum points if needed
	auto userPosStamp = userCamera_->positionStamp();
	auto userDirStamp = userCamera_->directionStamp();
	if (hasChanged || userPosStamp != userPositionStamp_ || userDirStamp != userDirectionStamp_) {
		userPositionStamp_ = userPosStamp;
		userDirectionStamp_ = userDirStamp;
		auto &userPos = userCamera_->position(0);
		auto &userDir = userCamera_->direction(0);
		for (unsigned int i = 0; i < numLayer_; ++i) {
			userCameraFrustum_[i].update(userPos, userDir);
			userFrustumCentroids_[i] = userPos + userDir *
					((userCameraFrustum_[i].far - userCameraFrustum_[i].near) * 0.5f + userCameraFrustum_[i].near);
		}
		hasChanged = true;
	}

	return hasChanged;
}

bool LightCamera_CSM::updateLightView() {
	if (lightDirStamp_ == light_->direction()->stampOfReadData() &&
		lightPosStamp_ == userPositionStamp_) { return false; }
	lightDirStamp_ = light_->direction()->stampOfReadData();
	lightPosStamp_ = userPositionStamp_;

	auto f = -light_->directionStaged(0).r;
	f.normalize();
	setDirection(0, f);
#ifdef CSM_USE_SINGLE_VIEW
	// NOTE: The nvidia example uses a single view matrix for all layers with position at (0,0,0).
	// but for some reason I have some issues with this approach.
	// Others use the centroid, and for some reason a small offset, it is not hard
	// to compute the position such that the near plane is at 1.0 eg. but do not know
	// if this is advantageous.
	// Equivalent to getLookAtMatrix(pos=(0,0,0), dir=f, up=(-1,0,0))
	Vec3f s(0.0f, -f.z, f.y);
	s.normalize();
	setView(0, Mat4f(
			0.0f, s.y * f.z - s.z * f.y, -f.x, 0.0f,
			s.y, s.z * f.x, -f.y, 0.0f,
			s.z, -s.y * f.x, -f.z, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
	));
	setViewInverse(0, view_[0].lookAtInverse());
	for (int i = 0; i < numLayer_; ++i) {
		setPosition(i, Vec3f::zero());
	}
#else
	for (unsigned int i = 0; i < numLayer_; ++i) {
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
		setPosition(i, userFrustumCentroids_[i] - f*centroid_z);
#else
		setPosition(i, userFrustumCentroids_[i] - f*userCameraFrustum_[i].far);
#endif
		setView(i, Mat4f::lookAtMatrix(position(i), f, Vec3f::up()));
		setViewInverse(i, view(i).lookAtInverse());
	}
#endif
	return true;
}

bool LightCamera_CSM::updateLightProjection() {
	// finally re-compute projection matrices, and frustum of light camera
	auto stamp = viewStamp();
	if (stamp == viewStamp_) { return false; }
	viewStamp_ = stamp;

	for (unsigned int layerIndex = 0; layerIndex < numLayer_; ++layerIndex) {
		auto &u_frustum = userCameraFrustum_[layerIndex];
		Bounds<Vec3f> bounds_ls(
			std::numeric_limits<float>::max(),
			std::numeric_limits<float>::lowest());
		for (int frustumIndex = 0; frustumIndex < 8; ++frustumIndex) {
#ifdef CSM_USE_SINGLE_VIEW
			auto point_ls = view_[0] ^ Vec4f(u_frustum.points[frustumIndex], 1.0f);
#else
			auto point_ls = view(layerIndex) ^
					Vec4f(u_frustum.points[frustumIndex], 1.0f);
#endif
			bounds_ls.min.setMin(point_ls.xyz_());
			bounds_ls.max.setMax(point_ls.xyz_());
		}
		auto buf = bounds_ls.max;
		bounds_ls.max.z = -bounds_ls.min.z;
		bounds_ls.min.z = -buf.z;

		setProjection(layerIndex, Mat4f::orthogonalMatrix(
				bounds_ls.min.x, bounds_ls.max.x,
				bounds_ls.min.y, bounds_ls.max.y,
				bounds_ls.min.z, bounds_ls.max.z));
		setProjectionInverse(layerIndex,
				projection(layerIndex).orthogonalInverse());
		frustum_[layerIndex].setOrtho(
				bounds_ls.min.x, bounds_ls.max.x,
				bounds_ls.min.y, bounds_ls.max.y,
				bounds_ls.min.z, bounds_ls.max.z);
		frustum_[layerIndex].update(
			position(layerIndex), direction(0));
	}

	return true;
}
