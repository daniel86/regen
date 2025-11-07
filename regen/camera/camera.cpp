#include "camera.h"
#include "light-camera-spot.h"
#include "light-camera-parabolic.h"
#include "light-camera-cube.h"
#include "light-camera-csm.h"
#include "reflection-camera.h"
#include "regen/objects/composite-mesh.h"
#include <regen/shapes/spatial-index.h>

using namespace regen;

namespace regen {
	class CameraMotion : public Animation {
	public:
		explicit CameraMotion(Camera *camera)
				: Animation(false, true),
				  camera_(camera) {}

		void animate(double dt) override {
			if(camera_->updatePose()) {
				camera_->updateShaderData(dt);
			}
		}

	private:
		Camera *camera_;
	};
}

Camera::Camera(unsigned int numLayer, const BufferUpdateFlags &updateFlags)
		: State(),
		  numLayer_(numLayer),
		  frustum_(numLayer) {
	// add shader constants via defines
	shaderDefine("RENDER_LAYER", REGEN_STRING(numLayer_));

	projParams_.resize(1);
	projParams_[0] = ProjectionParams(0.1f, 100.0f, 8.0f / 6.0f, 60.0f);
	sh_projParams_ = ref_ptr<ShaderInput4f>::alloc("cameraProjParams");
	sh_projParams_->setUniformData(projParams_[0].asVec4());

	direction_.resize(1);
	direction_[0] = Vec4f(0, 0, -1, 0);
	sh_direction_ = ref_ptr<ShaderInput4f>::alloc("cameraDirection");
	sh_direction_->setUniformData(direction_[0]);
	sh_direction_->setSchema(InputSchema::direction());

	position_.resize(1);
	position_[0] = Vec4f(0.0, 1.0, 4.0, 0.0);
	sh_position_ = ref_ptr<ShaderInput4f>::alloc("cameraPosition");
	sh_position_->setUniformData(position_[0]);
	sh_position_->setSchema(InputSchema::position());

	vel_ = Vec4f(0.0f);
	sh_vel_ = ref_ptr<ShaderInput4f>::alloc("cameraVelocity");
	sh_vel_->setUniformData(vel_);

	viewData_.resize(2, Mat4f::identity());
	view_ = std::span<Mat4f>(viewData_).subspan(0, 1);
	viewInv_ = std::span<Mat4f>(viewData_).subspan(1, 1);

	viewProjData_.resize(2, Mat4f::identity());
	viewProj_ = std::span<Mat4f>(viewProjData_).subspan(0, 1);
	viewProjInv_ = std::span<Mat4f>(viewProjData_).subspan(1, 1);

	projData_.resize(2, Mat4f::identity());
	proj_ = std::span<Mat4f>(projData_).subspan(0, 1);
	projInv_ = std::span<Mat4f>(projData_).subspan(1, 1);

	sh_view_ = ref_ptr<ShaderInputMat4>::alloc("viewMatrix");
	sh_view_->setUniformData(Mat4f::identity());
	sh_view_->setSchema(InputSchema::transform());
	sh_viewInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseViewMatrix");
	sh_viewInv_->setUniformData(Mat4f::identity());
	sh_viewInv_->setSchema(InputSchema::transform());

	sh_proj_ = ref_ptr<ShaderInputMat4>::alloc("projectionMatrix");
	sh_proj_->setUniformData(Mat4f::identity());
	sh_proj_->setSchema(InputSchema::transform());
	sh_projInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseProjectionMatrix");
	sh_projInv_->setUniformData(Mat4f::identity());
	sh_projInv_->setSchema(InputSchema::transform());

	sh_viewProj_ = ref_ptr<ShaderInputMat4>::alloc("viewProjectionMatrix");
	sh_viewProj_->setUniformData(Mat4f::identity());
	sh_viewProj_->setSchema(InputSchema::transform());
	sh_viewProjInv_ = ref_ptr<ShaderInputMat4>::alloc("inverseViewProjectionMatrix");
	sh_viewProjInv_->setUniformData(Mat4f::identity());
	sh_viewProjInv_->setSchema(InputSchema::transform());

	// TODO: I think we really need t use buffer container here!
	cameraBlock_ = ref_ptr<UBO>::alloc("Camera", updateFlags);
	cameraBlock_->addStagedInput(sh_view_);
	cameraBlock_->addStagedInput(sh_viewInv_);
	cameraBlock_->addStagedInput(sh_viewProj_);
	cameraBlock_->addStagedInput(sh_viewProjInv_);
	cameraBlock_->addStagedInput(sh_direction_);
	cameraBlock_->addStagedInput(sh_position_);
	cameraBlock_->addStagedInput(sh_vel_);
	// these change less frequent:
	cameraBlock_->addStagedInput(sh_projParams_);
	cameraBlock_->addStagedInput(sh_proj_);
	cameraBlock_->addStagedInput(sh_projInv_);
	setInput(cameraBlock_);
}

static inline void flushWritten(
		ClientBuffer &clientBuffer,
		uint32_t mappedIndex,
		uint32_t endOffset,
		BufferRange2ui &writtenRange) {
	if (writtenRange.size > 0) {
		clientBuffer.markWrittenTo(mappedIndex, writtenRange.offset, writtenRange.size);
		writtenRange.size = 0; // no more data written
	}
	writtenRange.offset = endOffset;
}

void Camera::updateShaderData(float dt) {
	// Ensure this function is not called while another update is in progress.
	// Wait until the update flag is set to false.
	while (isUpdating_.load(std::memory_order_acquire)) {
		CPU_PAUSE(); // busy wait, we expect very short duration of wait here.
	}
	isUpdating_.store(true, std::memory_order_release);

	// Update velocity
	if (dt > 0.0f) {
		vel_.xyz_() = (position_[0].xyz_() - lastPosition_) / dt;
	}
	lastPosition_ = position_[0].xyz_();

	if (isAudioListener()) {
		AudioListener::set3f(AL_POSITION, position(0));
		AudioListener::set3f(AL_VELOCITY, velocity());
		AudioListener::set6f(AL_ORIENTATION, Vec6f(direction(0), Vec3f::up()));
	}
	const bool viewChanged = (lastViewStamp1_ != viewStamp_);
	const bool projChanged = (lastProjStamp1_ != projStamp_);
	auto &clientBuffer = *cameraBlock_->clientBuffer().get();

	if (clientBuffer.hasSegments() && clientBuffer.hasClientData()) {
		auto mapped = clientBuffer.mapRange(
				BUFFER_GPU_WRITE,
				0u, clientBuffer.dataSize());
		uint32_t offset = 0, dataSize, dataSize2;
		BufferRange2ui writtenRange;

		dataSize = view_.size() * sizeof(Mat4f) * 2;
		if (viewChanged) {
			lastViewStamp1_ = viewStamp_;
			std::memcpy(mapped.w + offset, viewData_.data(), dataSize);
			sh_view_->clientBuffer()->nextStamp(mapped.w_index);
			sh_viewInv_->clientBuffer()->nextStamp(mapped.w_index);
			offset = dataSize;
			writtenRange.size = dataSize;
		} else {
			offset = dataSize;
			writtenRange.offset = offset;
		}

		dataSize = viewProj_.size() * sizeof(Mat4f) * 2;
		if (viewChanged || projChanged) {
			std::memcpy(mapped.w + offset, viewProjData_.data(), dataSize);
			sh_viewProj_->clientBuffer()->nextStamp(mapped.w_index);
			sh_viewProjInv_->clientBuffer()->nextStamp(mapped.w_index);
			offset += dataSize;
			writtenRange.size += dataSize;
		} else {
			offset += dataSize;
			flushWritten(clientBuffer, mapped.w_index, offset, writtenRange);
		}

		dataSize = direction_.size() * sizeof(Vec4f);
		if (lastDirStamp1_ != directionStamp_) {
			lastDirStamp1_ = directionStamp_;
			std::memcpy(mapped.w + offset, direction_.data(), dataSize);
			sh_direction_->clientBuffer()->nextStamp(mapped.w_index);
			offset += dataSize;
			writtenRange.size += dataSize;
		} else {
			offset += dataSize;
			flushWritten(clientBuffer, mapped.w_index, offset, writtenRange);
		}

		if (lastPosStamp1_ != positionStamp_) {
			lastPosStamp1_ = positionStamp_;
			dataSize = position_.size() * sizeof(Vec4f);
			std::memcpy(mapped.w + offset, position_.data(), dataSize);
			offset += dataSize;
			dataSize2 = sizeof(Vec4f);
			std::memcpy(mapped.w + offset, &vel_.x, dataSize2);
			offset += dataSize2;
			sh_position_->clientBuffer()->nextStamp(mapped.w_index);
			sh_vel_->clientBuffer()->nextStamp(mapped.w_index);
			writtenRange.size += dataSize + dataSize2;
		} else {
			offset += position_.size() * sizeof(Vec4f);
			offset += sizeof(Vec4f);
			flushWritten(clientBuffer, mapped.w_index, offset, writtenRange);
		}

		dataSize = projParams_.size() * sizeof(ProjectionParams);
		if (projChanged) {
			std::memcpy(mapped.w + offset, projParams_.data(), dataSize);
			sh_projParams_->clientBuffer()->nextStamp(mapped.w_index);
			offset += dataSize;
			writtenRange.size += dataSize;
		} else {
			offset += dataSize;
			flushWritten(clientBuffer, mapped.w_index, offset, writtenRange);
		}

		dataSize = proj_.size() * sizeof(Mat4f) * 2;
		if (projChanged) {
			lastProjStamp1_ = projStamp_;
			std::memcpy(mapped.w + offset, projData_.data(), dataSize);
			sh_proj_->clientBuffer()->nextStamp(mapped.w_index);
			sh_projInv_->clientBuffer()->nextStamp(mapped.w_index);
			offset += dataSize;
			writtenRange.size += dataSize;
		} else {
			offset += dataSize;
			flushWritten(clientBuffer, mapped.w_index, offset, writtenRange);
		}

		if (sh_clipPlane_.get()) {
			dataSize = clipPlane_.size() * sizeof(Vec4f);
			if (lastClipPlaneStamp1_ != clipPlaneStamp_) {
				lastClipPlaneStamp1_ = clipPlaneStamp_;
				std::memcpy(mapped.w + offset, clipPlane_.data(), dataSize);
				sh_clipPlane_->clientBuffer()->nextStamp(mapped.w_index);
				//offset += dataSize;
				writtenRange.size += dataSize;
			}
		}

		flushWritten(clientBuffer, mapped.w_index, offset, writtenRange);
		clientBuffer.unmapRange(BUFFER_GPU_WRITE, 0u, 0u, mapped.w_index);
	} else {
		// The client buffer of the UBO was not initialized.
		// So we need to write the data to individual shader inputs.
		if (viewChanged) {
			lastViewStamp1_ = viewStamp_;
			auto m_v = sh_view_->mapClientDataRaw(BUFFER_GPU_WRITE);
			std::memcpy(m_v.w, view_.data(), view_.size() * sizeof(Mat4f));
			m_v.unmap();

			auto m_v_i = sh_viewInv_->mapClientDataRaw(BUFFER_GPU_WRITE);
			std::memcpy(m_v_i.w, viewInv_.data(), viewInv_.size() * sizeof(Mat4f));
			m_v_i.unmap();
		}

		if (viewChanged || projChanged) {
			auto m_vp = sh_viewProj_->mapClientDataRaw(BUFFER_GPU_WRITE);
			std::memcpy(m_vp.w, viewProj_.data(), viewProj_.size() * sizeof(Mat4f));
			m_vp.unmap();

			auto m_vp_i = sh_viewProjInv_->mapClientDataRaw(BUFFER_GPU_WRITE);
			std::memcpy(m_vp_i.w, viewProjInv_.data(), viewProjInv_.size() * sizeof(Mat4f));
			m_vp_i.unmap();
		}

		if (lastDirStamp1_ != directionStamp_) {
			lastDirStamp1_ = directionStamp_;
			auto m_dir = sh_direction_->mapClientDataRaw(BUFFER_GPU_WRITE);
			std::memcpy(m_dir.w, direction_.data(), direction_.size() * sizeof(Vec4f));
			m_dir.unmap();
		}

		if (lastPosStamp1_ != positionStamp_) {
			lastPosStamp1_ = positionStamp_;
			auto m_pos = sh_position_->mapClientDataRaw(BUFFER_GPU_WRITE);
			std::memcpy(m_pos.w, position_.data(), position_.size() * sizeof(Vec4f));
			m_pos.unmap();

			auto m_vel = sh_vel_->mapClientDataRaw(BUFFER_GPU_WRITE);
			std::memcpy(m_vel.w, &vel_.x, sizeof(Vec4f));
			m_vel.unmap();
		}

		if (projChanged) {
			lastProjStamp1_ = projStamp_;
			auto m_pp = sh_projParams_->mapClientDataRaw(BUFFER_GPU_WRITE);
			std::memcpy(m_pp.w, projParams_.data(), projParams_.size() * sizeof(ProjectionParams));
			m_pp.unmap();

			auto m_p = sh_proj_->mapClientDataRaw(BUFFER_GPU_WRITE);
			std::memcpy(m_p.w, proj_.data(), proj_.size() * sizeof(Mat4f));
			m_p.unmap();

			auto m_p_i = sh_projInv_->mapClientDataRaw(BUFFER_GPU_WRITE);
			std::memcpy(m_p_i.w, projInv_.data(), projInv_.size() * sizeof(Mat4f));
			m_p_i.unmap();
		}
	}

	isUpdating_.store(false, std::memory_order_release);
}

bool Camera::updateCamera() {
	if (updateView() || projStamp_ != lastProjStamp_) {
		updateViewProjection1();
		lastProjStamp_ = projStamp_;
		camStamp_ += 1u;
		return true;
	} else {
		return false;
	}
}

bool Camera::updateView() {
	if (positionStamp_ == lastPosStamp_ && directionStamp_ == lastDirStamp_) { return false; }
	lastPosStamp_ = positionStamp_;
	lastDirStamp_ = directionStamp_;

	auto numViewLayers = view_.size();
	for (unsigned int i = 0; i < numViewLayers; ++i) {
		auto &dir = getClamped(direction_, i);
		if (std::abs(dir.xyz_().dot(Vec3f::up())) > 0.999f) {
			view_[i] = Mat4f::lookAtMatrix(
					getClamped(position_, i).xyz_(),
					dir.xyz_(), localUp_);
			viewInv_[i] = view_[i].lookAtInverse();
		} else {
			view_[i] = Mat4f::lookAtMatrix(
					getClamped(position_, i).xyz_(),
					dir.xyz_(), Vec3f::up());
			viewInv_[i] = view_[i].lookAtInverse();
		}
	}
	viewStamp_ += 1u;
	camStamp_ += 1u;

	return true;
}

void Camera::updateViewProjection1() {
	auto numViewLayers = view_.size();
	auto numProjLayers = proj_.size();
	auto maxIndex = std::max(numViewLayers, numProjLayers);
	for (unsigned int i = 0; i < maxIndex; ++i) {
		updateViewProjection(
				numProjLayers > 1 ? i : 0,
				numViewLayers > 1 ? i : 0);
	}
	updateFrustumBuffer();
}

void Camera::updateViewProjection(unsigned int projectionIndex, unsigned int viewIndex) {
	auto maxIndex = std::max(projectionIndex, viewIndex);
	viewProj_[maxIndex] = view_[viewIndex] * proj_[projectionIndex];
	viewProjInv_[maxIndex] = projInv_[projectionIndex] * viewInv_[viewIndex];
	frustum_[maxIndex].update(
			getClamped(position_, maxIndex).xyz_(),
			getClamped(direction_, maxIndex).xyz_());
	viewProjStamp_ += 1u;
	camStamp_ += 1u;
}

void Camera::setPerspective(const ProjectionParams &params) {
	setPerspective(
			params.aspect,
			params.fov,
			params.near,
			params.far);
}

void Camera::setPerspective(float aspect, float fov, float near, float far) {
	bool hasLayeredProjection = proj_.size() > 1;
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
	setClamped(projParams_, layer, ProjectionParams(near, far, aspect, fov));
	frustum_[layer].setPerspective(aspect, fov, near, far);

	setClamped(proj_, layer, Mat4f::projectionMatrix(fov, aspect, near, far));
	setClamped(projInv_, layer, getClamped(proj_, layer).projectionInverse());
	isOrtho_ = false;
	projStamp_ += 1u;
	camStamp_ += 1u;
}

void Camera::setOrtho(float left, float right, float bottom, float top, float near, float far) {
	for (unsigned int i = 0; i < numLayer_; ++i) {
		setOrtho(left, right, bottom, top, near, far, i);
	}
}

void Camera::setOrtho(float left, float right, float bottom, float top, float near, float far, unsigned int layer) {
	setClamped(projParams_, layer, ProjectionParams{
			near, far,
			abs((right - left) / (top - bottom)), 0.0f});
	frustum_[layer].setOrtho(left, right, bottom, top, near, far);
	proj_[layer] = Mat4f::orthogonalMatrix(left, right, bottom, top, near, far);
	projInv_[layer] = proj_[layer].orthogonalInverse();
	isOrtho_ = true;
	projStamp_ += 1u;
	camStamp_ += 1u;
}

void Camera::set_isAudioListener(GLboolean isAudioListener) {
	isAudioListener_ = isAudioListener;
	if (isAudioListener_) {
		AudioListener::set3f(AL_POSITION, position_[0].xyz_());
		AudioListener::set3f(AL_VELOCITY, vel_.xyz_());
		AudioListener::set6f(AL_ORIENTATION, Vec6f(
				direction_[0].xyz_(),
				Vec3f::up()));
	}
}

bool Camera::updatePose() {
	bool updated = false;
	if (attachedTF_.get()) {
		if (poseStamp_ != attachedTF_->stamp()) {
			poseStamp_ = attachedTF_->stamp();
			setPosition(0, attachedTF_->position(0).r);
			updated = true;
		}
	}
	if (updated) {
		updateCamera();
	}
	return updated;
}

ref_ptr<UBO> Camera::getFrustumBuffer() {
	if (!frustumBuffer_.get()) {
		createFrustumBuffer();
	}
	return frustumBuffer_;
}

void Camera::createFrustumBuffer() {
	frustumBuffer_ = ref_ptr<UBO>::alloc("FrustumBuffer", cameraBlock_->stagingUpdateHint());
	frustumBuffer_->setStagingAccessMode(BUFFER_CPU_WRITE);
	// each frustum has 6 planes, so we need 6 * numLayer_ Vec4f
	frustumData_ = ref_ptr<ShaderInput4f>::alloc("frustumPlanes", 6 * numLayer_);
	frustumData_->setUniformUntyped();
	frustumBuffer_->addStagedInput(frustumData_);
	frustumBuffer_->update();
	setInput(frustumBuffer_);
}

void Camera::updateFrustumBuffer() {
	if (!frustumBuffer_.get()) return;

	auto frustum_cpu =
			frustumData_->mapClientData<Vec4f>(BUFFER_GPU_WRITE);
	for (size_t i = 0; i < frustum_.size(); ++i) {
		auto &frustumPlanes = frustum_[i].planes;
		for (int j = 0; j < 6; ++j) {
			frustum_cpu.w[i * 6 + j] = frustumPlanes[j].equation();
		}
	}
}

void Camera::attachToPosition(const ref_ptr<ModelTransformation> &attached) {
	attachedTF_ = attached;
	poseStamp_ = 0;
	if (!attachedMotion_.get()) {
		attachedMotion_ = ref_ptr<CameraMotion>::alloc(this);
		attachedMotion_->startAnimation();
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

	for (auto &child: input.getChildren("culling")) {
		if (child->hasAttribute("index")) {
			auto spatialIndex = ctx.scene()->getResource<SpatialIndex>(child->getValue("index"));
			if (spatialIndex.get() == nullptr) {
				REGEN_WARN("Unable to find SpatialIndex for '" << input.getDescription() << "'.");
				continue;
			}

			ref_ptr<Camera> lodCamera;
			if (input.hasAttribute("lod-camera")) {
				lodCamera = ctx.scene()->getResource<Camera>(input.getValue("lod-camera"));
				if (lodCamera.get() == nullptr) {
					REGEN_WARN("Unable to find LOD Camera for '" << input.getDescription() << "'.");
				}
			}
			if (!lodCamera.get()) {
				lodCamera = cam;
			}

			spatialIndex->addCamera(cam, lodCamera,
				input.getValue<SortMode>("sort", SortMode::FRONT_TO_BACK),
				input.getValue<Vec4i>("lod-shift", Vec4i::zero()));
		} else {
			REGEN_WARN("Ignoring culling node without index for '" << input.getDescription() << "'.");
		}
	}

	return cam;
}

int getHiddenFacesMask(scene::SceneInputNode &input) {
	int hiddenFacesMask = 0;
	if (input.hasAttribute("hide-faces")) {
		auto val = input.getValue<std::string>("hide-faces", "");
		std::vector<std::string> faces;
		boost::split(faces, val, boost::is_any_of(","));
		for (auto & it : faces) {
			CubeCamera::Face face;
			std::stringstream(it) >> face;
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
			if (input.hasAttribute("uniform-z-range")) {
				dirCam->setUseUniformDepthRange(input.getValue<bool>("uniform-z-range", true));
			}
			if (input.hasAttribute("depth-padding")) {
				dirCam->setDepthPadding(input.getValue<double>("depth-padding", 0.0));
			}
			if (input.hasAttribute("ortho-padding")) {
				dirCam->setOrthoPadding(input.getValue<double>("ortho-padding", 0.0));
			}
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

	// make sure initial data is good.
	lightCamera->updateCamera();
	lightCamera->updateShaderData(0.0f);
	dynamic_cast<LightCamera *>(lightCamera.get())->updateShadowData();

	ctx.scene()->putState(input.getName(), lightCamera);

	return lightCamera;
}

ProjectionUpdater::ProjectionUpdater(const ref_ptr<Camera> &cam,
									 const ref_ptr<Screen> &screen)
	: EventHandler(),
	  cam_(cam),
	  screen_(screen) {
}

void ProjectionUpdater::call(EventObject *, EventData *) {
	auto windowViewport = screen_->viewport().r;
	auto windowAspect = (GLfloat) windowViewport.x / (GLfloat) windowViewport.y;

	auto &lastProjParams = cam_->projParams()[0];
	if (cam_->isOrtho()) {
		// keep the ortho width and adjust height based on aspect ratio
		auto width = cam_->frustum()[0].nearPlaneHalfSize.x * 2.0f;
		auto height = width / windowAspect;
		cam_->setOrtho(
				-width / 2.0f, width / 2.0f,
				-height / 2.0f, height / 2.0f,
				lastProjParams.near,
				lastProjParams.far);
	} else {
		cam_->setPerspective(
				windowAspect,
				lastProjParams.fov,
				lastProjParams.near,
				lastProjParams.far);
	}
	if(cam_->updateCamera()) {
		cam_->updateShaderData(0.0f);
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
			ref_ptr<CompositeMesh> compositeMesh =
					ctx.scene()->getResource<CompositeMesh>(input.getValue("reflector"));
			if (compositeMesh.get() == nullptr || compositeMesh->meshes().empty()) {
				REGEN_WARN("Unable to find Mesh for '" << input.getDescription() << "'.");
				return {};
			}
			const std::vector<ref_ptr<Mesh> > &vec = compositeMesh->meshes();
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
			cam->attachToPosition(tf);
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
			cam->attachToPosition(tf);
		} else if (input.hasAttribute("tf")) {
			REGEN_WARN("Unable to find ModelTransformation for '" << input.getDescription() << "'.");
		}
		ctx.scene()->putState(input.getName(), cam);
		return cam;
	} else {
		ref_ptr<Camera> cam = ref_ptr<Camera>::alloc(1);
		cam->set_isAudioListener(
				input.getValue<bool>("audio-listener", false));
		cam->setPosition(0, input.getValue<Vec3f>("position", Vec3f(0.0f, 2.0f, -2.0f)));
		if (input.hasAttribute("up")) {
			cam->setLocalUp(input.getValue<Vec3f>("up", Vec3f::right()));
		}

		auto dir = input.getValue<Vec3f>("direction", Vec3f(0.0f, 0.0f, 1.0f));
		dir.normalize();
		cam->setDirection(0, dir);

		if (camType == "ortho" || camType == "orthographic" || camType == "orthogonal") {
			auto width = input.getValue<GLfloat>("width", 10.0f);
			auto height = input.getValue<GLfloat>("height", 10.0f);
			cam->setOrtho(
					-width / 2.0f, width / 2.0f,
					-height / 2.0f, height / 2.0f,
					input.getValue<GLfloat>("near", 0.1f),
					input.getValue<GLfloat>("far", 200.0f));
		} else {
			auto viewport = ctx.scene()->screen()->viewport();
			cam->setPerspective(
					(GLfloat) viewport.r.x / (GLfloat) viewport.r.y,
					input.getValue<GLfloat>("fov", 45.0f),
					input.getValue<GLfloat>("near", 0.1f),
					input.getValue<GLfloat>("far", 200.0f));
			// Update frustum when window size changes
			ctx.scene()->addEventHandler(Scene::RESIZE_EVENT,
										 ref_ptr<ProjectionUpdater>::alloc(cam, ctx.scene()->screen()));
		}
		cam->updateCamera();
		cam->updateShaderData(0.0f);
		ctx.scene()->putState(input.getName(), cam);

		return cam;
	}
}
