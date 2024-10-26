/*
 * camera.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "camera.h"
#include <regen/application.h>

using namespace regen::scene;
using namespace regen;
using namespace std;

#include <regen/scene/resource-manager.h>
#include <regen/camera/cube-camera.h>
#include <regen/camera/paraboloid-camera.h>
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
		cam_->updateFrustum(
				(GLfloat) windowViewport_->getVertex(0).x /
				(GLfloat) windowViewport_->getVertex(0).y,
				cam_->fov()->getVertex(0),
				cam_->near()->getVertex(0),
				cam_->far()->getVertex(0));
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
		if (cam.get() == NULL) {
			REGEN_WARN("Unable to create Camera for '" << input.getDescription() << "'.");
			return {};
		}
		parser->putState(input.getName(), cam);
		return cam;
	} else if (input.hasAttribute("light")) {
		ref_ptr<Camera> userCamera =
				parser->getResources()->getCamera(parser, input.getValue("camera"));
		if (userCamera.get() == nullptr) {
			REGEN_WARN("Unable to find Camera for '" << input.getDescription() << "'.");
			return {};
		}
		ref_ptr<Light> light =
				parser->getResources()->getLight(parser, input.getValue("light"));
		if (light.get() == nullptr) {
			REGEN_WARN("Unable to find Light for '" << input.getDescription() << "'.");
			return {};
		}
		auto numLayer = input.getValue<GLuint>("num-layer", 1u);
		auto splitWeight = input.getValue<GLdouble>("split-weight", 0.9);
		auto near = input.getValue<GLdouble>("near", 0.1);
		auto far = input.getValue<GLdouble>("far", 200.0);
		ref_ptr<LightCamera> cam = ref_ptr<LightCamera>::alloc(
				light, userCamera, Vec2f(near, far), numLayer, splitWeight);
		parser->putState(input.getName(), cam);

		// Hide cube shadow map faces.
		if (input.hasAttribute("hide-faces")) {
			const string val = input.getValue<string>("hide-faces", "");
			vector<string> faces;
			boost::split(faces, val, boost::is_any_of(","));
			for (auto it = faces.begin();
				 it != faces.end(); ++it) {
				int faceIndex = atoi(it->c_str());
				cam->set_isCubeFaceVisible(
						GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex, GL_FALSE);
			}
		}

		return cam;
	} else if (input.hasAttribute("cube-mesh")) {
		ref_ptr<Camera> userCamera =
				parser->getResources()->getCamera(parser, input.getValue("camera"));
		if (userCamera.get() == nullptr) {
			REGEN_WARN("Unable to find Camera for '" << input.getDescription() << "'.");
			return {};
		}
		ref_ptr<MeshVector> mesh =
				parser->getResources()->getMesh(parser, input.getValue("cube-mesh"));
		if (mesh.get() == nullptr || mesh->empty()) {
			REGEN_WARN("Unable to find Mesh for '" << input.getDescription() << "'.");
			return {};
		}
		ref_ptr<CubeCamera> cam = ref_ptr<CubeCamera>::alloc((*mesh.get())[0], userCamera);
		parser->putState(input.getName(), cam);

		// Hide cube shadow map faces.
		if (input.hasAttribute("hide-faces")) {
			const string val = input.getValue<string>("hide-faces", "");
			vector<string> faces;
			boost::split(faces, val, boost::is_any_of(","));
			for (auto it = faces.begin();
				 it != faces.end(); ++it) {
				int faceIndex = atoi(it->c_str());
				cam->set_isCubeFaceVisible(
						GL_TEXTURE_CUBE_MAP_POSITIVE_X + faceIndex, GL_FALSE);
			}
		}

		return cam;
	} else if (input.hasAttribute("paraboloid-mesh")) {
		ref_ptr<Camera> userCamera =
				parser->getResources()->getCamera(parser, input.getValue("camera"));
		if (userCamera.get() == nullptr) {
			REGEN_WARN("Unable to find Camera for '" << input.getDescription() << "'.");
			return {};
		}
		ref_ptr<MeshVector> mesh =
				parser->getResources()->getMesh(parser, input.getValue("paraboloid-mesh"));
		if (mesh.get() == nullptr || mesh->empty()) {
			REGEN_WARN("Unable to find Mesh for '" << input.getDescription() << "'.");
			return {};
		}
		bool hasBackFace = input.getValue<bool>("has-back-face", false);

		ref_ptr<ParaboloidCamera> cam = ref_ptr<ParaboloidCamera>::alloc(
				(*mesh.get())[0], userCamera, hasBackFace);
		parser->putState(input.getName(), cam);

		return cam;
	} else {
		ref_ptr<Camera> cam = ref_ptr<Camera>::alloc();
		cam->set_isAudioListener(
				input.getValue<bool>("audio-listener", false));
		cam->position()->setVertex(0,
								   input.getValue<Vec3f>("position", Vec3f(0.0f, 2.0f, -2.0f)));

		auto dir = input.getValue<Vec3f>("direction", Vec3f(0.0f, 0.0f, 1.0f));
		dir.normalize();
		cam->direction()->setVertex(0, dir);

		cam->updateFrustum(
				(GLfloat) parser->getViewport()->getVertex(0).x /
				(GLfloat) parser->getViewport()->getVertex(0).y,
				input.getValue<GLfloat>("fov", 45.0f),
				input.getValue<GLfloat>("near", 0.1f),
				input.getValue<GLfloat>("far", 200.0f));
		// Update frustum when window size changes
		parser->addEventHandler(Application::RESIZE_EVENT,
								ref_ptr<ProjectionUpdater>::alloc(cam, parser->getViewport()));
		parser->putState(input.getName(), cam);

		return cam;
	}
}
