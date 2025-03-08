#include "geometric-picking.h"
#include "atomic-states.h"
#include "depth-state.h"
#include "feedback-state.h"
#include "regen/scene/node-processor.h"

using namespace regen;

GeomPicking::GeomPicking(const ref_ptr<Camera> &camera, const ref_ptr<ShaderInput2f> &mouseTexco)
		: StateNode(),
		  camera_(camera),
		  hasPickedObject_(false) {
	maxPickedObjects_ = 100;
	// skip picking some frames
	pickInterval_ = 50.0;
	dt_ = 0.0;

	// create uniforms encoding the mouse position
	mouseTexco_ = mouseTexco;
	mousePosVS_ = ref_ptr<ShaderInput3f>::alloc("mousePosVS");
	mousePosVS_->setUniformData(Vec3f(0.0f));
	state_->joinShaderInput(mousePosVS_);

	mouseDirVS_ = ref_ptr<ShaderInput3f>::alloc("mouseDirVS");
	mouseDirVS_->setUniformData(Vec3f(0.0f, 1.0f, 0.0f));
	state_->joinShaderInput(mouseDirVS_);

	// skip fragment shader, only up to geometry shader is needed
	state_->joinStates(ref_ptr<ToggleState>::alloc(RenderState::RASTARIZER_DISCARD, GL_TRUE));

	// create attributes used for picking
	pickObjectID_ = ref_ptr<ShaderInput1i>::alloc("pickObjectID");
	pickObjectID_->setUniformData(0);
	pickInstanceID_ = ref_ptr<ShaderInput1i>::alloc("pickInstanceID");
	pickInstanceID_->setUniformData(0);
	pickDepth_ = ref_ptr<ShaderInput1f>::alloc("pickDepth");
	pickDepth_->setUniformData(1.0f);

	// setup transform feedback buffer
	bufferSize_ = sizeof(PickData) * maxPickedObjects_;
	feedbackBuffer_ = ref_ptr<VBO>::alloc(VBO::USAGE_FEEDBACK);
	vboRef_ = feedbackBuffer_->alloc(bufferSize_);
	bufferRange_ = ref_ptr<BufferRange>::alloc();
	bufferRange_->buffer_ = vboRef_->bufferID();

	// setup transform feedback specification, this is needed for shaders to know what to output
	feedbackState_ = ref_ptr<FeedbackSpecification>::alloc(maxPickedObjects_);
	feedbackState_->set_feedbackMode(GL_INTERLEAVED_ATTRIBS);
	feedbackState_->set_feedbackStage(GL_GEOMETRY_SHADER);
	feedbackState_->addFeedback(pickDepth_);
	feedbackState_->addFeedback(pickInstanceID_);
	feedbackState_->addFeedback(pickObjectID_);
	state_->joinStates(feedbackState_);
}

GeomPicking::~GeomPicking() = default;

void GeomPicking::pick(RenderState *rs, GLuint feedbackCount) {
	rs->copyReadBuffer().push(vboRef_->bufferID());
	auto *bufferData = (PickData *) glMapBufferRange(
			GL_COPY_READ_BUFFER,
			vboRef_->address(),
			bufferSize_,
			GL_MAP_READ_BIT);
	// find pick result with max depth (camera looks in negative z direction)
	PickData *bestPicked = nullptr;
	for (GLuint i = 0; i < feedbackCount; ++i) {
		PickData &picked_x = bufferData[i];
		if (picked_x.objectID < 0) { continue; }
		if (!bestPicked || picked_x.depth > bestPicked->depth) {
			bestPicked = &picked_x;
		}
	}
	if (bestPicked) {
		pickedObject_.depth = bestPicked->depth;
		pickedObject_.instanceID = bestPicked->instanceID;
		pickedObject_.objectID = bestPicked->objectID;
		hasPickedObject_ = true;
	} else {
		hasPickedObject_ = false;
	}
	glUnmapBuffer(GL_COPY_READ_BUFFER);
	rs->copyReadBuffer().pop();
}

void GeomPicking::updateMouse() {
	auto inverseProjectionMatrix = camera_->projectionInverse()->getVertex(0);
	auto mouse = mouseTexco_->getVertex(0);
	// find view space mouse ray intersecting the frustum
	Vec2f mouseNDC = mouse.r * 2.0 - Vec2f(1.0);
	// in NDC space the ray starts at (mx,my,0) and ends at (mx,my,1)
	Vec4f mouseRayNear = inverseProjectionMatrix.r ^ Vec4f(mouseNDC, 0.0, 1.0);
	Vec4f mouseRayFar = inverseProjectionMatrix.r ^ Vec4f(mouseNDC, 1.0, 1.0);
	mouseRayNear.xyz_() /= mouseRayNear.w;
	mouseRayFar.xyz_() /= mouseRayFar.w;
	mousePosVS_->setVertex(0, mouseRayNear.xyz_());
	mouseDirVS_->setVertex(0, mouseRayNear.xyz_() - mouseRayFar.xyz_());
}

void GeomPicking::traverse(RenderState *rs) {
	updateMouse();
	state_->enable(rs);

	int feedbackCount = 0;
	GLuint feedbackQuery = 0;
	glGenQueries(1, &feedbackQuery);

	for (auto &pickableNode: childs()) {
		auto pickableMesh = pickableNode->findStateWithType<Mesh>();
		if (pickableMesh == nullptr) {
			REGEN_WARN("No mesh found in pickable node.");
			continue;
		}
		// update buffer range of pickable
		bufferRange_->offset_ = feedbackCount * sizeof(PickData);
		bufferRange_->size_ = bufferSize_ - bufferRange_->offset_;
		bufferRange_->offset_ += vboRef_->address();
		pickableMesh->setFeedbackRange(bufferRange_);
		glBeginQuery(GL_PRIMITIVES_GENERATED, feedbackQuery);
		// render pickable
		pickableNode->traverse(rs);

		glEndQuery(GL_PRIMITIVES_GENERATED);
		feedbackCount += static_cast<int>(getGLQueryResult(feedbackQuery));
		if (feedbackCount >= static_cast<int>(maxPickedObjects_)) {
			break;
		}
	}

	glDeleteQueries(1, &feedbackQuery);

	if (feedbackCount > 0) {
		auto numPicks = static_cast<GLuint>(feedbackCount);
		pick(rs, (numPicks < maxPickedObjects_ ? numPicks : maxPickedObjects_));
	} else {
		hasPickedObject_ = false;
	}

	state_->disable(rs);
	GL_ERROR_LOG();
}

namespace regen {
	class PickingUpdater : public State {
	public:
		explicit PickingUpdater(Application *app, const ref_ptr<GeomPicking> &geomPicking)
				: State(), app_(app), geomPicking_(geomPicking) {}

		void disable(RenderState *rs) override {
			State::disable(rs);
			if (geomPicking_->hasPickedObject()) {
				auto &pickedNode = app_->getObjectWithID(geomPicking_->pickedObject()->objectID);
				if (pickedNode.get() != nullptr) {
					app_->setHoveredObject(pickedNode, geomPicking_->pickedObject());
				} else {
					REGEN_WARN("Unable to find object with ID " <<
																geomPicking_->pickedObject()->objectID << " in scene.");
					app_->unsetHoveredObject();
				}
			} else {
				app_->unsetHoveredObject();
			}
		}

	protected:
		Application *app_;
		ref_ptr<GeomPicking> geomPicking_;
	};
}

ref_ptr<GeomPicking> GeomPicking::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto scene = ctx.scene();

	auto userCamera = scene->getResource<Camera>(input.getValue("camera"));
	if (userCamera.get() == nullptr) {
		REGEN_WARN("Unable to find Camera for '" << input.getDescription() << "'.");
		return {};
	}
	auto &mouseTexco = scene->getMouseTexco();
	auto geomPicking = ref_ptr<GeomPicking>::alloc(userCamera, mouseTexco);
	ctx.parent()->addChild(geomPicking);

	// create picking updater
	auto pickingUpdater = ref_ptr<PickingUpdater>::alloc(scene->application(), geomPicking);
	geomPicking->state()->joinStates(pickingUpdater);

	// load children nodes
	scene::SceneNodeProcessor::handleChildren(scene, input, geomPicking);

	return geomPicking;
}
