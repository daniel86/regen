/*
 * camera.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "camera.h"
#include "regen/camera/light-camera-spot.h"
#include "regen/camera/light-camera-csm.h"
#include "regen/camera/light-camera-parabolic.h"
#include "regen/camera/light-camera-cube.h"
#include <regen/application.h>

using namespace regen::scene;
using namespace regen;
using namespace std;

#include <regen/scene/resource-manager.h>
#include <regen/camera/cube-camera.h>
#include <regen/camera/parabolic-camera.h>
#include <regen/camera/light-camera.h>
#include <regen/camera/reflection-camera.h>

#define REGEN_CAMERA_CATEGORY "camera"

/**
 * Updates Camera Projection when window size changes.
 */
class ProjectionUpdater : public EventHandler {
public:
	/**
	 * Default constructor.
	 * @param cam Camera reference.
	 * @param windowViewport The window dimensions.
	 */
	ProjectionUpdater(const ref_ptr<Camera> &cam,
					  const ref_ptr<ShaderInput2i> &windowViewport)
			: EventHandler(), cam_(cam), windowViewport_(windowViewport) {}

	void call(EventObject *, EventData *) {
		auto windowViewport = windowViewport_->getVertex(0);
		auto windowAspect =
			(GLfloat) windowViewport.r.x / (GLfloat) windowViewport.r.y;
		if (cam_->isOrtho()) {
			// keep the ortho width and adjust height based on aspect ratio
			auto width = cam_->frustum()[0].nearPlaneHalfSize.x * 2.0f;
			auto height = width / windowAspect;
			cam_->setOrtho(
					-width / 2.0f, width / 2.0f,
					-height / 2.0f, height / 2.0f,
					cam_->near()->getVertex(0).r,
					cam_->far()->getVertex(0).r);
		}
		else {
			cam_->setPerspective(
					windowAspect,
					cam_->fov()->getVertex(0).r,
					cam_->near()->getVertex(0).r,
					cam_->far()->getVertex(0).r);
		}
	}

protected:
	ref_ptr<Camera> cam_;
	ref_ptr<ShaderInput2i> windowViewport_;
};

CameraResource::CameraResource()
		: ResourceProvider(REGEN_CAMERA_CATEGORY) {}

ref_ptr<Camera> CameraResource::createResource(
		SceneParser *parser,
		SceneInputNode &input) {
	auto cam = createCamera(parser, input);
	if (cam.get() == nullptr) {
		REGEN_WARN("Unable to create Camera for '" << input.getDescription() << "'.");
		return {};
	}

	if (input.hasAttribute("culling-index")) {
		auto spatialIndex = parser->getResources()->getIndex(
				parser, input.getValue("culling-index"));
		spatialIndex->addCamera(cam);
	}

	return cam;
}

int getHiddenFacesMask(SceneInputNode &input) {
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

ref_ptr<Camera> createLightCamera(
		SceneParser *parser,
		SceneInputNode &input) {
	ref_ptr<Light> light = parser->getResources()->getLight(parser, input.getValue("light"));
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
			auto userCamera = parser->getResources()->getCamera(parser, input.getValue("camera"));
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
	parser->putState(input.getName(), lightCamera);

	return lightCamera;
}

ref_ptr<Camera> CameraResource::createCamera(SceneParser *parser, SceneInputNode &input) {
	auto camType = input.getValue<string>("type", "spot");

	if (input.hasAttribute("reflector") ||
		input.hasAttribute("reflector-normal") ||
		input.hasAttribute("reflector-point")) {
		ref_ptr<Camera> userCamera =
				parser->getResources()->getCamera(parser, input.getValue("camera"));
		if (userCamera.get() == nullptr) {
			REGEN_WARN("Unable to find Camera for '" << input.getDescription() << "'.");
			return {};
		}
		ref_ptr<ReflectionCamera> cam;
		bool hasBackFace = input.getValue<bool>("has-back-face", false);

		if (input.hasAttribute("reflector")) {
			ref_ptr<MeshVector> mesh =
					parser->getResources()->getMesh(parser, input.getValue("reflector"));
			if (mesh.get() == nullptr || mesh->empty()) {
				REGEN_WARN("Unable to find Mesh for '" << input.getDescription() << "'.");
				return {};
			}
			const vector<ref_ptr<Mesh> > &vec = *mesh.get();
			cam = ref_ptr<ReflectionCamera>::alloc(
					userCamera, vec[0], input.getValue<GLuint>("vertex-index", 0u), hasBackFace);
		} else if (input.hasAttribute("reflector-normal")) {
			auto normal = input.getValue<Vec3f>("reflector-normal", Vec3f(0.0f, 1.0f, 0.0f));
			auto position = input.getValue<Vec3f>("reflector-point", Vec3f(0.0f, 0.0f, 0.0f));
			cam = ref_ptr<ReflectionCamera>::alloc(userCamera, normal, position, hasBackFace);
		}
		if (cam.get()) {
			parser->putState(input.getName(), cam);
		}
		return cam;
	}
	else if (input.hasAttribute("light")) {
		auto cam = createLightCamera(parser, input);
		return cam;
	}
	else if (camType == "cube") {
		auto tf = parser->getResources()->getTransform(parser, input.getValue("tf"));
		ref_ptr<CubeCamera> cam = ref_ptr<CubeCamera>::alloc(getHiddenFacesMask(input));
		if (tf.get()) {
			cam->attachToPosition(tf->get());
		}
		parser->putState(input.getName(), cam);
		return cam;
	}
	else if (camType == "parabolic" || camType == "paraboloid") {
		auto tf = parser->getResources()->getTransform(parser, input.getValue("tf"));
		bool hasBackFace = input.getValue<bool>("dual-paraboloid", true);
		auto cam = ref_ptr<ParabolicCamera>::alloc(hasBackFace);
		if (input.hasAttribute("normal")) {
			cam->setNormal(input.getValue<Vec3f>("normal", Vec3f::down()));
		}

		if (tf.get()) {
			cam->attachToPosition(tf->get());
		}
		parser->putState(input.getName(), cam);

		return cam;
	}
	else {
		ref_ptr<Camera> cam = ref_ptr<Camera>::alloc(1);
		cam->set_isAudioListener(
				input.getValue<bool>("audio-listener", false));
		cam->position()->setVertex(0,
								   input.getValue<Vec3f>("position", Vec3f(0.0f, 2.0f, -2.0f)));

		auto dir = input.getValue<Vec3f>("direction", Vec3f(0.0f, 0.0f, 1.0f));
		dir.normalize();
		cam->direction()->setVertex(0, dir);

		if (camType == "ortho" || camType == "orthographic") {
			auto width = input.getValue<GLfloat>("width", 10.0f);
			auto height = input.getValue<GLfloat>("height", 10.0f);
			cam->setOrtho(
					-width / 2.0f, width / 2.0f,
					-height / 2.0f, height / 2.0f,
					input.getValue<GLfloat>("near", 0.1f),
					input.getValue<GLfloat>("far", 200.0f));
		} else {
			auto viewport = parser->getViewport()->getVertex(0);
			cam->setPerspective(
					(GLfloat) viewport.r.x / (GLfloat) viewport.r.y,
					input.getValue<GLfloat>("fov", 45.0f),
					input.getValue<GLfloat>("near", 0.1f),
					input.getValue<GLfloat>("far", 200.0f));
		}
		cam->updateCamera();
		// Update frustum when window size changes
		parser->addEventHandler(Application::RESIZE_EVENT,
								ref_ptr<ProjectionUpdater>::alloc(cam, parser->getViewport()));
		parser->putState(input.getName(), cam);

		return cam;
	}
}
