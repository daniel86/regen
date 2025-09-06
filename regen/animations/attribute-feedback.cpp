#include "attribute-feedback.h"
#include "regen/meshes/mesh-vector.h"
#include "regen/states/state-configurer.h"

using namespace regen;

AttributeFeedbackAnimation::AttributeFeedbackAnimation(
		const ref_ptr<Mesh> &inputMesh,
		const std::string &shaderKey)
		: Animation(true, false),
		  animationNode_(ref_ptr<StateNode>::alloc()),
		  inputMesh_(inputMesh),
		  shaderKey_(shaderKey) {
	setAnimationName("attribute-feedback");
	animationNode_->state()->joinStates(animationState());
	// create the feedback mesh with original attributes
	feedbackMesh_ = ref_ptr<Mesh>::alloc(inputMesh_);
	// create feedback state
	feedbackState_ = ref_ptr<FeedbackState>::alloc(
			inputMesh_->primitive(),
			inputMesh_->numVertices(),
			VERTEX_LAYOUT_INTERLEAVED);
	feedbackState_->set_feedbackStage(GL_VERTEX_SHADER);
	animationNode_->state()->joinStates(feedbackState_);

	bufferSize_ = 0u;
	originalAttributes_.reserve(inputMesh_->inputs().size());
	feedbackAttributes_.reserve(inputMesh_->inputs().size());
	for (auto &namedAttribute : inputMesh_->inputs()) {
		auto &attribute = namedAttribute.in_;
		if (!attribute->isVertexAttribute()) continue;
		auto feedbackAttribute = feedbackState_->addFeedback(attribute);
		originalAttributes_.push_back(namedAttribute);
		feedbackAttributes_.emplace_back(feedbackAttribute, namedAttribute.name_);
		bufferSize_ += feedbackAttribute->inputSize();
	}
}

void AttributeFeedbackAnimation::initializeResources() {
	// make sure the feedback buffer is initialized
	feedbackState_->initializeResources();
	// Overwrite the original attributes with the feedback attributes.
	// note: we assume here no VAO was created yet for inputMesh_
	for (uint64_t attributeIndex = 0; attributeIndex < originalAttributes_.size(); ++attributeIndex) {
		auto &feedbackAttribute = feedbackAttributes_[attributeIndex];
		inputMesh_->setInput(feedbackAttribute.in_, feedbackAttribute.name_);
	}

	// create shader state
	shaderState_ = ref_ptr<ShaderState>::alloc();
	animationNode_->state()->joinStates(shaderState_);
	animationNode_->state()->joinStates(feedbackMesh_);

	// compile the feedback shader
	StateConfigurer shaderConfigurer;
	shaderConfigurer.addNode(animationNode_.get());
	shaderState_->createShader(shaderConfigurer.cfg(), shaderKey_);
	feedbackMesh_->updateVAO(shaderConfigurer.cfg(), shaderState_->shader());
}

void AttributeFeedbackAnimation::glAnimate(regen::RenderState *rs, GLdouble dt) {
	animationNode_->traverse(rs);
}

ref_ptr<AttributeFeedbackAnimation> AttributeFeedbackAnimation::load(scene::SceneLoader *scene, scene::SceneInputNode &input) {
	// Try to find the mesh
	auto meshID = input.getValue("mesh-id");
	auto meshIndex = input.getValue<uint32_t>("mesh-index", 0u);
	auto meshVec = scene->getResource<MeshVector>(meshID);
	if (meshVec.get() == nullptr || meshVec->empty()) {
		REGEN_WARN("Unable to find Mesh for '" << input.getDescription() << "'.");
		return {};
	}
	if (meshVec->size() <= meshIndex) {
		REGEN_WARN("Mesh index " << meshIndex << " is out of range for '" << input.getDescription() << "'.");
		meshIndex = 0u;
	}
	auto &mesh = (*meshVec.get())[meshIndex];
	auto shaderKey = input.getValue("shader");
	auto animation = ref_ptr<AttributeFeedbackAnimation>::alloc(mesh, shaderKey);
	// create a state using children of the input node
	auto &animationNode = animation->animationNode_;
	for (auto &child: input.getChildren()) {
		auto stateProcessor = scene->getStateProcessor(child->getCategory());
		if (stateProcessor.get() != nullptr) {
			stateProcessor->processInput(
					scene, *child.get(),
					animationNode, animationNode->state());
		} else {
			REGEN_WARN("No state processor registered for '" << child->getDescription() << "'.");
		}
	}
	animation->initializeResources();
	animation->startAnimation();
	return animation;
}
