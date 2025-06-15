#include "camera.h"
#include "light-camera-spot.h"
#include "light-camera-parabolic.h"
#include "light-camera-cube.h"
#include "light-camera-csm.h"
#include "reflection-camera.h"
#include "regen/scene/scene.h"
#include "regen/meshes/mesh-vector.h"
#include <regen/shapes/spatial-index.h>

using namespace regen;

// FIXME: camera update currently does not ensure the update being atomic for the GL thread!
//         meaning it can and will happen that GL has partial data from the camera update.
//         which will be not super noticeable I think, when changes in the camera per frame are small.
// FIXME: the indexed setVertex is REALLY NOT GOOD! It might cause a lot of copies, need to rewrite
//         the camera classes!

namespace regen {
	class CameraMotion : public Animation {
	public:
		explicit CameraMotion(Camera *camera)
				: Animation(false, true),
				  camera_(camera) {}

		void animate(double dt) override { camera_->updatePose(); }

	private:
		Camera *camera_;
	};
}

Camera::Camera(unsigned int numLayer)
		: HasInputState(ARRAY_BUFFER),
		  numLayer_(numLayer),
		  frustum_(numLayer) {
	// add shader constants via defines
	shaderDefine("RENDER_LAYER", REGEN_STRING(numLayer_));

	projParams_ = ref_ptr<ShaderInput4f>::alloc("cameraProjParams");
	projParams_->setUniformData(Vec4f(0.1f, 100.0f, 8.0f / 6.0f, 60.0f));

	position_ = ref_ptr<ShaderInput4f>::alloc("cameraPosition");
	position_->setUniformData(Vec4f(0.0, 1.0, 4.0, 0.0));
	position_->setSchema(InputSchema::position());

	direction_ = ref_ptr<ShaderInput4f>::alloc("cameraDirection");
	direction_->setUniformData(Vec4f(0, 0, -1, 0));
	direction_->setSchema(InputSchema::direction());

	vel_ = ref_ptr<ShaderInput4f>::alloc("cameraVelocity");
	vel_->setUniformData(Vec4f(0.0f));

	view_ = ref_ptr<ShaderInputMat4>::alloc("viewMatrix");
	view_->setUniformData(Mat4f::identity());
	view_->setSchema(InputSchema::transform());
	viewInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseViewMatrix");
	viewInv_->setUniformData(Mat4f::identity());
	viewInv_->setSchema(InputSchema::transform());

	proj_ = ref_ptr<ShaderInputMat4>::alloc("projectionMatrix");
	proj_->setUniformData(Mat4f::identity());
	proj_->setSchema(InputSchema::transform());
	projInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseProjectionMatrix");
	projInv_->setUniformData(Mat4f::identity());
	projInv_->setSchema(InputSchema::transform());

	viewProj_ = ref_ptr<ShaderInputMat4>::alloc("viewProjectionMatrix");
	viewProj_->setUniformData(Mat4f::identity());
	viewProj_->setSchema(InputSchema::transform());
	viewProjInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseViewProjectionMatrix");
	viewProjInv_->setUniformData(Mat4f::identity());
	viewProjInv_->setSchema(InputSchema::transform());

	// TODO: I think we really need t use buffer container here!
	cameraBlock_ = ref_ptr<UBO>::alloc("Camera");
	cameraBlock_->addBlockInput(view_);
	cameraBlock_->addBlockInput(viewInv_);
	cameraBlock_->addBlockInput(proj_);
	cameraBlock_->addBlockInput(projInv_);
	cameraBlock_->addBlockInput(viewProj_);
	cameraBlock_->addBlockInput(viewProjInv_);
	cameraBlock_->addBlockInput(projParams_);
	cameraBlock_->addBlockInput(position_);
	cameraBlock_->addBlockInput(direction_);
	cameraBlock_->addBlockInput(vel_);
	setInput(cameraBlock_);
}

void Camera::setPerspective(const Vec4f &params) {
	setPerspective(
			params.z,	// aspect
			params.w,		// fov
			params.x,		// near
			params.y);		// far
}

void Camera::setPerspective(float aspect, float fov, float near, float far) {
	bool hasLayeredProjection = proj_->numArrayElements() > 1;
	if (hasLayeredProjection) {
		for (unsigned int i = 0; i < numLayer_; ++i) {
			setPerspective(aspect, fov, near, far, i);
		}
	} else {
		setPerspective(aspect, fov, near, far, 0);
		for (unsigned int i = 1; i < numLayer_; ++i) {
			frustum_[i].setPerspective(aspect, fov, near, far);
		}
	}
}

void Camera::setPerspective(float aspect, float fov, float near, float far, unsigned int layer) {
	projParams_->setVertexClamped(layer, Vec4f(near, far, aspect, fov));
	frustum_[layer].setPerspective(aspect, fov, near, far);

	auto projectionMatrix = Mat4f::projectionMatrix(fov, aspect, near, far);
	proj_->setVertexClamped(layer, projectionMatrix);
	projInv_->setVertexClamped(layer, projectionMatrix.projectionInverse());
	isOrtho_ = false;
}

void Camera::setOrtho(float left, float right, float bottom, float top, float near, float far) {
	for (unsigned int i = 0; i < numLayer_; ++i) {
		setOrtho(left, right, bottom, top, near, far, i);
	}
}

void Camera::setOrtho(float left, float right, float bottom, float top, float near, float far, unsigned int layer) {
	projParams_->setVertexClamped(layer, Vec4f(
			near, far,
			abs((right - left) / (top - bottom)), 0.0f));
	frustum_[layer].setOrtho(left, right, bottom, top, near, far);
	proj_->setVertex(layer, Mat4f::orthogonalMatrix(left, right, bottom, top, near, far));
	projInv_->setVertex(layer, proj_->getVertex(layer).r.orthogonalInverse());
	isOrtho_ = true;
}

bool Camera::updateCamera() {
	auto projectionStamp = proj_->stamp();
	if (updateView() || projectionStamp != projectionStamp_) {
		updateViewProjection1();
		projectionStamp_ = projectionStamp;
		camStamp_ += 1u;
		return true;
	} else {
		return false;
	}
}

bool Camera::updateView() {
	auto posStamp = position_->stamp();
	auto dirStamp = direction_->stamp();
	if (posStamp == posStamp_ && dirStamp == dirStamp_) { return false; }
	posStamp_ = posStamp;
	dirStamp_ = dirStamp;

	auto numViewLayers = view_->numArrayElements();
	for (unsigned int i = 0; i < numViewLayers; ++i) {
		auto dir = direction_->getVertexClamped(i);
		if (std::abs(dir.r.xyz_().dot(Vec3f::up())) > 0.999f) {
			auto viewMatrix = Mat4f::lookAtMatrix(
					position_->getVertexClamped(i).r.xyz_(),
					dir.r.xyz_(), Vec3f::right());
			view_->setVertex(i, viewMatrix);
			viewInv_->setVertex(i, viewMatrix.lookAtInverse());
		} else {
			auto viewMatrix = Mat4f::lookAtMatrix(
					position_->getVertexClamped(i).r.xyz_(),
					dir.r.xyz_(), Vec3f::up());
			view_->setVertex(i, viewMatrix);
			viewInv_->setVertex(i, viewMatrix.lookAtInverse());
		}
	}

	return true;
}

void Camera::updateViewProjection1() {
	auto numViewLayers = view_->numArrayElements();
	auto numProjLayers = proj_->numArrayElements();
	auto maxIndex = std::max(numViewLayers, numProjLayers);
	for (unsigned int i = 0; i < maxIndex; ++i) {
		updateViewProjection(
				numProjLayers > 1 ? i : 0,
				numViewLayers > 1 ? i : 0);
	}
}

void Camera::updateViewProjection(unsigned int projectionIndex, unsigned int viewIndex) {
	auto maxIndex = std::max(projectionIndex, viewIndex);
	viewProj_->setVertex(maxIndex,
						 view_->getVertex(viewIndex).r * proj_->getVertex(projectionIndex).r);
	viewProjInv_->setVertex(maxIndex,
							projInv_->getVertex(projectionIndex).r * viewInv_->getVertex(viewIndex).r);
	frustum_[maxIndex].update(
			position()->getVertexClamped(maxIndex).r.xyz_(),
			direction()->getVertexClamped(maxIndex).r.xyz_());
}

void Camera::set_isAudioListener(GLboolean isAudioListener) {
	isAudioListener_ = isAudioListener;
	if (isAudioListener_) {
		AudioListener::set3f(AL_POSITION, position_->getVertex(0).r.xyz_());
		AudioListener::set3f(AL_VELOCITY, vel_->getVertex(0).r.xyz_());
		AudioListener::set6f(AL_ORIENTATION, Vec6f(
				direction_->getVertex(0).r.xyz_(),
				Vec3f::up()));
	}
}

void Camera::updatePose() {
	bool updated = false;
	if (attachedPosition_.get()) {
		if (poseStamp_ != attachedPosition_->stamp()) {
			poseStamp_ = attachedPosition_->stamp();
			position_->setVertex3(0, attachedPosition_->getVertex(0).r.xyz_());
			updated = true;
		}
	} else if (attachedTransform_.get()) {
		if (poseStamp_ != attachedTransform_->stamp()) {
			poseStamp_ = attachedTransform_->stamp();
			auto m = attachedTransform_->getVertex(0);
			position_->setVertex3(0, m.r.position());
			if (!isAttachedToPosition_) {
				// TODO: change camera orientation based on transform
				//direction_->setVertex(0, (m ^ Vec4f(Vec3f::front(),0.0)).xyz_());
			}
			updated = true;
		}
	}

	if (updated) {
		updateCamera();
	}
}

void Camera::attachToPosition(const ref_ptr<ShaderInput4f> &attachedPosition) {
	attachedPosition_ = attachedPosition;
	attachedTransform_ = {};
	poseStamp_ = 0;
	if (!attachedMotion_.get()) {
		attachedMotion_ = ref_ptr<CameraMotion>::alloc(this);
		attachedMotion_->startAnimation();
	}
}

void Camera::attachToPosition(const ref_ptr<ShaderInputMat4> &attachedTransform) {
	attachedPosition_ = {};
	attachedTransform_ = attachedTransform;
	poseStamp_ = 0;
	isAttachedToPosition_ = true;
	if (!attachedMotion_.get()) {
		attachedMotion_ = ref_ptr<CameraMotion>::alloc(this);
		attachedMotion_->startAnimation();
	}
}

void Camera::attachToTransform(const ref_ptr<ShaderInputMat4> &attachedTransform) {
	attachedPosition_ = {};
	attachedTransform_ = attachedTransform;
	poseStamp_ = 0;
	isAttachedToPosition_ = false;
	if (!attachedMotion_.get()) {
		attachedMotion_ = ref_ptr<CameraMotion>::alloc(this);
		attachedMotion_->startAnimation();
	}
}

bool Camera::hasSphereIntersection(const Vec3f &center, GLfloat radius) const {
	auto projParams = projParams_->mapClientVertex<ProjectionParams>(ShaderData::READ, 0);
	auto d = Plane(
			position()->getVertex(0).r.xyz_(),
			direction()->getVertex(0).r.xyz_()).distance(center);
	return d - radius < projParams.r.far &&
		   d + radius > projParams.r.near;
}

bool Camera::hasSphereIntersection(const Vec3f &center, const Vec3f *points) const {
	auto projParams = projParams_->mapClientVertex<ProjectionParams>(ShaderData::READ, 0);
	Plane p(position()->getVertex(0).r.xyz_(), direction()->getVertex(0).r.xyz_());
	for (int i = 0; i < 8; ++i) {
		auto d = p.distance(center + points[i]);
		if (d > projParams.r.far || d < projParams.r.near)
			return false;
	}
	return true;
}

bool Camera::hasHalfSphereIntersection(const Vec3f &center, GLfloat radius) const {
	auto projParams = projParams_->mapClientVertex<ProjectionParams>(ShaderData::READ, 0);
	// get the distance from the camera to the center of the sphere
	auto d = Plane(
			position()->getVertex(0).r.xyz_(),
			direction()->getVertex(0).r.xyz_()).distance(center);
	// check if the sphere is outside the far plane
	if (d - radius > projParams.r.far) return false;
	// check if the sphere is inside the near plane
	if (d + radius < projParams.r.near) return false;
	// check if the sphere is inside the half sphere
	auto halfSphereRadius = projParams.r.far;
	auto halfSphereNormal = direction()->getVertex(0);
	auto halfSphereCenter = position()->getVertex(0).r.xyz_() + halfSphereNormal.r.xyz_() * halfSphereRadius;
	return Plane(halfSphereCenter, halfSphereNormal.r.xyz_()).distance(center) < radius;
}

bool Camera::hasHalfSphereIntersection(const Vec3f &center, const Vec3f *points) const {
	auto projParams = projParams_->mapClientVertex<ProjectionParams>(ShaderData::READ, 0);
	// get the distance from the camera to the center of the sphere
	auto d = Plane(
			position()->getVertex(0).r.xyz_(),
			direction()->getVertex(0).r.xyz_()).distance(center);
	// check if the sphere is outside the far plane
	if (d > projParams.r.far) return false;
	// check if the sphere is inside the near plane
	if (d < projParams.r.near) return false;
	// check if the sphere is inside the half sphere
	auto halfSphereRadius = projParams.r.far;
	auto halfSphereNormal = direction()->getVertex(0);
	auto halfSphereCenter = position()->getVertex(0).r.xyz_() + halfSphereNormal.r.xyz_() * halfSphereRadius;
	auto halfSphere = Plane(halfSphereCenter, halfSphereNormal.r.xyz_());
	for (int i = 0; i < 8; ++i) {
		if (halfSphere.distance(center + points[i]) < 0) {
			return false;
		}
	}
	return true;
}

bool Camera::hasFrustumIntersection(const Vec3f &center, GLfloat radius) const {
	for (auto &f: frustum_) {
		if (f.hasIntersectionWithSphere(center, radius)) {
			return true;
		}
	}
	return false;
}

bool Camera::hasFrustumIntersection(const Vec3f &center, const Vec3f *points) const {
	for (auto &f: frustum_) {
		if (f.hasIntersectionWithBox(center, points)) {
			return true;
		}
	}
	return false;
}

bool Camera::hasIntersectionWithSphere(const Vec3f &center, GLfloat radius) const {
	if (isOmni_) {
		return hasSphereIntersection(center, radius);
	}
		//else if (isSemiOmni_) {
		//	return hasHalfSphereIntersection(center, radius);
		//}
	else {
		return hasFrustumIntersection(center, radius);
	}
}

bool Camera::hasIntersectionWithBox(const Vec3f &center, const Vec3f *points) const {
	if (isOmni_) {
		return hasSphereIntersection(center, points);
	}
		//else if (isSemiOmni_) {
		//	return hasHalfSphereIntersection(center, points);
		//}
	else {
		return hasFrustumIntersection(center, points);
	}
}

ref_ptr<Camera> Camera::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto cam = createCamera(ctx, input);
	if (cam.get() == nullptr) {
		REGEN_WARN("Unable to create Camera for '" << input.getDescription() << "'.");
		return {};
	}

	if (input.hasAttribute("fixed-lod")) {
		cam->setFixedLOD(input.getValue<LODQuality>("fixed-lod", LODQuality::HIGH));
	}

	if (input.hasAttribute("culling-index")) {
		auto spatialIndex = ctx.scene()->getResource<SpatialIndex>(input.getValue("culling-index"));
		spatialIndex->addCamera(cam, input.getValue<bool>("sort", true));
	}

	return cam;
}

int getHiddenFacesMask(scene::SceneInputNode &input) {
	int hiddenFacesMask = 0;
	if (input.hasAttribute("hide-faces")) {
		auto val = input.getValue<std::string>("hide-faces", "");
		std::vector<std::string> faces;
		boost::split(faces, val, boost::is_any_of(","));
		for (auto it = faces.begin(); it != faces.end(); ++it) {
			CubeCamera::Face face;
			std::stringstream(*it) >> face;
			hiddenFacesMask |= face;
		}
	}
	return hiddenFacesMask;
}

ref_ptr<Camera> createLightCamera(LoadingContext &ctx, scene::SceneInputNode &input) {
	ref_ptr<Light> light = ctx.scene()->getResource<Light>(input.getValue("light"));
	if (light.get() == nullptr) {
		REGEN_WARN("Unable to find Light for '" << input.getDescription() << "'.");
		return {};
	}
	auto numLayer = input.getValue<GLuint>("num-layer", 1u);
	auto splitWeight = input.getValue<GLdouble>("split-weight", 0.9);
	auto cameraType = input.getValue<std::string>("camera-type", "spot");
	auto near = input.getValue<float>("near", 0.1f);
	ref_ptr<Camera> lightCamera;

	switch (light->lightType()) {
		case Light::SPOT: {
			auto spotCam = ref_ptr<LightCamera_Spot>::alloc(light);
			spotCam->setLightNear(near);
			lightCamera = spotCam;
			break;
		}
		case Light::DIRECTIONAL: {
			auto userCamera = ctx.scene()->getResource<Camera>(input.getValue("camera"));
			if (userCamera.get() == nullptr) {
				REGEN_WARN("Unable to find user camera for '" << input.getDescription() << "'.");
				return {};
			}
			auto dirCam = ref_ptr<LightCamera_CSM>::alloc(light, userCamera, numLayer);
			dirCam->setSplitWeight(splitWeight);
			lightCamera = dirCam;
			break;
		}
		case Light::POINT: {
			if (cameraType == "parabolic" || cameraType == "paraboloid") {
				// parabolic camera
				auto isDualParabolic = input.getValue<bool>("dual-paraboloid", true);
				auto parabolic = ref_ptr<LightCamera_Parabolic>::alloc(light, isDualParabolic);
				if (input.hasAttribute("normal")) {
					parabolic->setNormal(input.getValue<Vec3f>("normal", Vec3f::down()));
				}
				parabolic->setLightNear(near);
				lightCamera = parabolic;
			} else {
				// cube camera
				auto cube = ref_ptr<LightCamera_Cube>::alloc(light, getHiddenFacesMask(input));
				cube->setLightNear(near);
				lightCamera = cube;
			}
			break;
		}
	}
	if (lightCamera.get() == nullptr) {
		REGEN_WARN("Unable to create camera for '" << input.getDescription() << "'.");
		return {};
	}
	ctx.scene()->putState(input.getName(), lightCamera);

	return lightCamera;
}

ProjectionUpdater::ProjectionUpdater(const ref_ptr<Camera> &cam,
									 const ref_ptr<ShaderInput2i> &windowViewport)
		: EventHandler(), cam_(cam), windowViewport_(windowViewport) {}

void ProjectionUpdater::call(EventObject *, EventData *) {
	auto windowViewport = windowViewport_->getVertex(0);
	auto windowAspect =
			(GLfloat) windowViewport.r.x / (GLfloat) windowViewport.r.y;
	auto lastProjParams = cam_->projParams()->getVertex(0).r;
	if (cam_->isOrtho()) {
		// keep the ortho width and adjust height based on aspect ratio
		auto width = cam_->frustum()[0].nearPlaneHalfSize.x * 2.0f;
		auto height = width / windowAspect;
		cam_->setOrtho(
				-width / 2.0f, width / 2.0f,
				-height / 2.0f, height / 2.0f,
				lastProjParams.x,
				lastProjParams.y);
	} else {
		cam_->setPerspective(
				windowAspect,
				lastProjParams.w,
				lastProjParams.x,
				lastProjParams.y);
	}
}

ref_ptr<Camera> Camera::createCamera(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto camType = input.getValue<std::string>("type", "spot");

	if (input.hasAttribute("reflector") ||
		input.hasAttribute("reflector-normal") ||
		input.hasAttribute("reflector-point")) {
		ref_ptr<Camera> userCamera =
				ctx.scene()->getResource<Camera>(input.getValue("camera"));
		if (userCamera.get() == nullptr) {
			REGEN_WARN("Unable to find Camera for '" << input.getDescription() << "'.");
			return {};
		}
		ref_ptr<ReflectionCamera> cam;
		bool hasBackFace = input.getValue<bool>("has-back-face", false);

		if (input.hasAttribute("reflector")) {
			ref_ptr<MeshVector> mesh =
					ctx.scene()->getResource<MeshVector>(input.getValue("reflector"));
			if (mesh.get() == nullptr || mesh->empty()) {
				REGEN_WARN("Unable to find Mesh for '" << input.getDescription() << "'.");
				return {};
			}
			const std::vector<ref_ptr<Mesh> > &vec = *mesh.get();
			cam = ref_ptr<ReflectionCamera>::alloc(
					userCamera, vec[0], input.getValue<GLuint>("vertex-index", 0u), hasBackFace);
		} else if (input.hasAttribute("reflector-normal")) {
			auto normal = input.getValue<Vec3f>("reflector-normal", Vec3f(0.0f, 1.0f, 0.0f));
			auto position = input.getValue<Vec3f>("reflector-point", Vec3f(0.0f, 0.0f, 0.0f));
			cam = ref_ptr<ReflectionCamera>::alloc(userCamera, normal, position, hasBackFace);
		}
		if (cam.get()) {
			ctx.scene()->putState(input.getName(), cam);
		}
		return cam;
	} else if (input.hasAttribute("light")) {
		auto cam = createLightCamera(ctx, input);
		return cam;
	} else if (camType == "cube") {
		auto tf = ctx.scene()->getResource<ModelTransformation>(input.getValue("tf"));
		ref_ptr<CubeCamera> cam = ref_ptr<CubeCamera>::alloc(getHiddenFacesMask(input));
		if (tf.get()) {
			if (tf->hasModelMat()) {
				cam->attachToPosition(tf->modelMat());
			} else if (tf->hasModelOffset()) {
				cam->attachToPosition(tf->modelOffset());
			}
		}
		ctx.scene()->putState(input.getName(), cam);
		return cam;
	} else if (camType == "parabolic" || camType == "paraboloid") {
		auto tf = ctx.scene()->getResource<ModelTransformation>(input.getValue("tf"));
		bool hasBackFace = input.getValue<bool>("dual-paraboloid", true);
		auto cam = ref_ptr<ParabolicCamera>::alloc(hasBackFace);
		if (input.hasAttribute("normal")) {
			cam->setNormal(input.getValue<Vec3f>("normal", Vec3f::down()));
		}

		if (tf.get()) {
			if (tf->hasModelMat()) {
				cam->attachToPosition(tf->modelMat());
			} else if (tf->hasModelOffset()) {
				cam->attachToPosition(tf->modelOffset());
			}
		}
		ctx.scene()->putState(input.getName(), cam);

		return cam;
	} else {
		ref_ptr<Camera> cam = ref_ptr<Camera>::alloc(1);
		cam->set_isAudioListener(
				input.getValue<bool>("audio-listener", false));
		cam->position()->setVertex3(0,
								   input.getValue<Vec3f>("position", Vec3f(0.0f, 2.0f, -2.0f)));

		auto dir = input.getValue<Vec3f>("direction", Vec3f(0.0f, 0.0f, 1.0f));
		dir.normalize();
		cam->direction()->setVertex3(0, dir);

		if (camType == "ortho" || camType == "orthographic" || camType == "orthogonal") {
			auto width = input.getValue<GLfloat>("width", 10.0f);
			auto height = input.getValue<GLfloat>("height", 10.0f);
			cam->setOrtho(
					-width / 2.0f, width / 2.0f,
					-height / 2.0f, height / 2.0f,
					input.getValue<GLfloat>("near", 0.1f),
					input.getValue<GLfloat>("far", 200.0f));
		} else {
			auto viewport = ctx.scene()->getViewport()->getVertex(0);
			cam->setPerspective(
					(GLfloat) viewport.r.x / (GLfloat) viewport.r.y,
					input.getValue<GLfloat>("fov", 45.0f),
					input.getValue<GLfloat>("near", 0.1f),
					input.getValue<GLfloat>("far", 200.0f));
			// Update frustum when window size changes
			ctx.scene()->addEventHandler(Scene::RESIZE_EVENT,
										 ref_ptr<ProjectionUpdater>::alloc(cam, ctx.scene()->getViewport()));
		}
		cam->updateCamera();
		ctx.scene()->putState(input.getName(), cam);

		return cam;
	}
}
