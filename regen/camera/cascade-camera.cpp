#include "cascade-camera.h"

using namespace regen;

namespace regen {
	static constexpr bool CSM_USE_SINGLE_VIEW = false;
}

LightCamera_CSM::LightCamera_CSM(
		const ref_ptr<Light> &light,
		const ref_ptr<Camera> &userCamera,
		unsigned int numLayer)
		: Camera(numLayer),
		  LightCamera(light, this),
		  userCamera_(userCamera),
		  userCameraFrustum_(numLayer),
		  userFrustumCentroids_(numLayer),
		  lightSpaceBounds_(numLayer, Bounds<Vec3f>::create(Vec3f::posMax(), Vec3f::negMax())) {
	shaderDefine("RENDER_TARGET", "2D_ARRAY");
	shaderDefine("RENDER_TARGET_MODE", "CASCADE");

	if constexpr (!CSM_USE_SINGLE_VIEW){
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
	if constexpr (CSM_USE_SINGLE_VIEW) {
		// NOTE: The nvidia example uses a single view matrix for all layers with position at (0,0,0).
		// but for some reason I have some issues with this approach.
		// Others use the centroid, and for some reason a small offset, it is not hard
		// to compute the position such that the near plane is at 1.0 eg. but do not know
		// if this is advantageous.
		// Equivalent to getLookAtMatrix(pos=(0,0,0), dir=f, up=(-1,0,0))
		Vec3f s(0.0f, -f.z, f.y);
		s.normalize();
		setView(0, Mat4f{
				0.0f, s.y * f.z - s.z * f.y, -f.x, 0.0f,
				s.y, s.z * f.x, -f.y, 0.0f,
				s.z, -s.y * f.x, -f.z, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f });
		setViewInverse(0, view_[0].lookAtInverse());
		for (int i = 0; i < numLayer_; ++i) {
			setPosition(i, Vec3f::zero());
		}
	} else {
		for (unsigned int i = 0; i < numLayer_; ++i) {
			setPosition(i, userFrustumCentroids_[i]);
			setView(i, Mat4f::lookAtMatrix(position(i), f, Vec3f::up()));
			setViewInverse(i, view(i).lookAtInverse());
		}
	}
	return true;
}

bool LightCamera_CSM::updateLightProjection() {
	// finally re-compute projection matrices, and frustum of light camera
	auto stamp = viewStamp();
	if (stamp == viewStamp_) { return false; }
	viewStamp_ = stamp;

	auto zRange = Vec2f(
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::lowest());
	for (unsigned int layerIndex = 0; layerIndex < numLayer_; ++layerIndex) {
		auto &u_frustum = userCameraFrustum_[layerIndex];
		auto &bounds = lightSpaceBounds_[layerIndex];
		bounds.min = Vec3f::posMax();
		bounds.max = Vec3f::negMax();
		for (int frustumIndex = 0; frustumIndex < 8; ++frustumIndex) {
			Vec4f point_ls;
			if constexpr (CSM_USE_SINGLE_VIEW) {
				point_ls = view_[0] ^ Vec4f::create(u_frustum.points[frustumIndex], 1.0f);
			} else {
				point_ls = view(layerIndex) ^ Vec4f::create(u_frustum.points[frustumIndex], 1.0f);
			}
			bounds.min.setMin(point_ls.xyz());
			bounds.max.setMax(point_ls.xyz());
			zRange.x = std::min(zRange.x, point_ls.z);
			zRange.y = std::max(zRange.y, point_ls.z);
		}
	}

	for (unsigned int layerIndex = 0; layerIndex < numLayer_; ++layerIndex) {
		auto &bounds = lightSpaceBounds_[layerIndex];
		if (useUniformDepthRange_) {
			// use z-range that fits all frustums. This is less efficient, but
			// avoids some artifacts when sampling between cascades.
			bounds.min.z = zRange.x;
			bounds.max.z = zRange.y;
		}
		if (depthPadding_ > 0.0) {
			// add some padding to the orthographic projection
			float zPadding = depthPadding_ * (zRange.y - zRange.x);
			bounds.min.z -= zPadding;
			bounds.max.z += zPadding;
		}
		if (orthoPadding_ > 0.0) {
			// also pad x/y
			float xPadding = orthoPadding_ * (bounds.max.x - bounds.min.x);
			float yPadding = orthoPadding_ * (bounds.max.y - bounds.min.y);
			bounds.min.x -= xPadding;
			bounds.max.x += xPadding;
			bounds.min.y -= yPadding;
			bounds.max.y += yPadding;
		}
		setProjection(layerIndex, Mat4f::orthogonalMatrix(
				bounds.min.x, bounds.max.x,
				bounds.min.y, bounds.max.y,
				bounds.min.z, bounds.max.z));
		setProjectionInverse(layerIndex,
				projection(layerIndex).orthogonalInverse());
		frustum_[layerIndex].setOrtho(
				bounds.min.x, bounds.max.x,
				bounds.min.y, bounds.max.y,
				bounds.min.z, bounds.max.z);
		frustum_[layerIndex].update(
			position(layerIndex), direction(0));
	}

	return true;
}
