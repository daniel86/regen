#include "light-camera-dir.h"

using namespace regen;

static inline Vec2f findZRange(
		const Mat4f &mat, const Vec3f *frustumPoints) {
	Vec2f range;
	// find the z-range of the current frustum as seen from the light
	// in order to increase precision
#define TRANSFORM_Z(vec) (mat.x[2]*(vec).x + mat.x[6]*(vec).y + mat.x[10]*(vec).z + mat.x[14])
	// note that only the z-component is needed and thus
	// the multiplication can be simplified from mat*vec4f(frustumPoints[0], 1.0f) to..
	GLfloat buf = TRANSFORM_Z(frustumPoints[0]);
	range.x = buf;
	range.y = buf;
	for (GLint i = 1; i < 8; ++i) {
		buf = TRANSFORM_Z(frustumPoints[i]);
		if (buf > range.y) { range.y = buf; }
		if (buf < range.x) { range.x = buf; }
	}
#undef TRANSFORM_Z
	return range;
}

LightCamera_Directional::LightCamera_Directional(
		const ref_ptr<Light> &light,
		const ref_ptr<Camera> &userCamera,
		unsigned int numLayer)
		: Camera(numLayer),
		  LightCamera(light,this),
		  userCamera_(userCamera),
		  userCameraFrustum_(numLayer) {
	shaderDefine("RENDER_TARGET", "2D_ARRAY");

	// Set matrix array size
	proj_->set_elementCount(numLayer_);
	projInv_->set_elementCount(numLayer_);
	viewProj_->set_elementCount(numLayer_);
	viewProjInv_->set_elementCount(numLayer_);

	// Allocate matrices
	proj_->setUniformDataUntyped(nullptr);
	projInv_->setUniformDataUntyped(nullptr);
	viewProj_->setUniformDataUntyped(nullptr);
	viewProjInv_->setUniformDataUntyped(nullptr);

	near_->set_elementCount(numLayer_);
	near_->set_forceArray(true);
	near_->setUniformDataUntyped(nullptr);

	far_->set_elementCount(numLayer_);
	far_->set_forceArray(true);
	far_->setUniformDataUntyped(nullptr);

	lightMatrix_->set_elementCount(numLayer_);
	lightMatrix_->set_forceArray(true);
	lightMatrix_->setUniformDataUntyped(nullptr);
	setInput(lightMatrix_);

	position_->setVertex(0, Vec3f(0.0f));
	updateDirectionalLight();
}

void LightCamera_Directional::setSplitWeight(GLdouble splitWeight) {
	splitWeight_ = splitWeight;
}

bool LightCamera_Directional::updateLight() {
	return updateDirectionalLight();
}

bool LightCamera_Directional::updateDirectionalLight() {
	auto changed = updateLightView();
	changed = updateLightProjection() || changed;
	if(changed) {
		updateViewProjection1();
		// Transforms world space coordinates to homogenous light space
		for (int i=0; i<lightMatrix_->elementCount(); ++i) {
			lightMatrix_->setVertex(i, viewProj_->getVertex(i) * Mat4f::bias());
		}
		camStamp_ += 1;
	}
	return changed;
}

bool LightCamera_Directional::updateLightProjection() {
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
		for (int i = 0; i < numLayer_; ++i) {
			userCameraFrustum_[i].update(
					userCamera_->position()->getVertex(0),
					userCamera_->direction()->getVertex(0));
		}
		hasChanged = true;
	}

	// finally re-compute projection matrices, and frustum of light camera
	auto viewStamp = view_->stamp();
	if (hasChanged || viewStamp != viewStamp_) {
		viewStamp_ = viewStamp;

		for (int i = 0; i < numLayer_; ++i) {
			auto &u_frustum = userCameraFrustum_[i];
			// get the projection matrix with the new z-bounds
			// note the inversion because the light looks at the neg. z axis
			Vec2f zRange = findZRange(view_->getVertex(0), u_frustum.points);

			auto newProj = Mat4f::orthogonalMatrix(
					-1.0, 1.0, -1.0, 1.0, -zRange.y, -zRange.x);
			// find the extends of the frustum slice as projected in light's homogeneous coordinates
			Vec2f xRange(FLT_MAX, FLT_MIN);
			Vec2f yRange(FLT_MAX, FLT_MIN);
			Vec2f xRange1(FLT_MAX, FLT_MIN);
			Vec2f yRange1(FLT_MAX, FLT_MIN);
			Vec2f zRange1(FLT_MAX, FLT_MIN);
			auto mvpMatrix = view_->getVertex(0) * newProj;
			for (const auto & point : u_frustum.points) {
				Vec4f x = mvpMatrix ^ point;
				if (x.x > xRange1.y) { xRange1.y = x.x; }
				if (x.x < xRange1.x) { xRange1.x = x.x; }
				if (x.y > yRange1.y) { yRange1.y = x.y; }
				if (x.y < yRange1.x) { yRange1.x = x.y; }
				x.x /= x.w;
				x.y /= x.w;
				if (x.x > xRange.y) { xRange.y = x.x; }
				if (x.x < xRange.x) { xRange.x = x.x; }
				if (x.y > yRange.y) { yRange.y = x.y; }
				if (x.y < yRange.x) { yRange.x = x.y; }

			}
			newProj *= Mat4f::cropMatrix(xRange.x, xRange.y, yRange.x, yRange.y);
			proj_->setVertex(i, newProj);
			// TODO slow inverse, might be possible to optimize
			projInv_->setVertex(i, newProj.inverse());
			// set the frustum orthogonal projection
			// TODO: not sure about the frustum calculation...
			// TODO: maybe look here and re-do https://learnopengl.com/Guest-Articles/2021/CSM
			/*
			float scaleX = 2.0f / (xRange.y - xRange.x);
			float scaleY = 2.0f / (yRange.y - yRange.x);
			frustum_[i].setOrtho(
				-scaleX, scaleX,
				-scaleY, scaleY,
				-zRange.y, -zRange.x);
			 */
			frustum_[i].setOrtho(
				xRange1.x, xRange1.y,
				yRange1.x, yRange1.y,
				-zRange.y, -zRange.x);
		}
		hasChanged = true;
	}

	return hasChanged;
}

bool LightCamera_Directional::updateLightView() {
	if (lightDirStamp_ == light_->direction()->stamp()) { return false; }
	lightDirStamp_ = light_->direction()->stamp();

	const Vec3f &dir = light_->direction()->getVertex(0);
	Vec3f f(-dir.x, -dir.y, -dir.z);
	f.normalize();
	direction_->setVertex(0, f);
	Vec3f s(0.0f, -f.z, f.y);
	s.normalize();
	// Equivalent to getLookAtMatrix(pos=(0,0,0), dir=f, up=(-1,0,0))
	view_->setVertex(0, Mat4f(
			0.0f, s.y * f.z - s.z * f.y, -f.x, 0.0f,
			s.y, s.z * f.x, -f.y, 0.0f,
			s.z, -s.y * f.x, -f.z, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
	));
	viewInv_->setVertex(0,
		view_->getVertex(0).lookAtInverse());
	return true;
}
