/*
 * transform.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_TRANSFORM_H_
#define REGEN_SCENE_TRANSFORM_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>
#include <regen/scene/value-generator.h>
#include <regen/scene/resource-manager.h>

#include <regen/states/model-transformation.h>
#include "regen/animations/transform-animation.h"

#define REGEN_TRANSFORM_STATE_CATEGORY "transform"

static void transformMatrix(
		const string &target, Mat4f &mat, const Vec3f &value) {
	if (target == "translate") {
		mat.x[12] += value.x;
		mat.x[13] += value.y;
		mat.x[14] += value.z;
	}
	else if (target == "scale") {
		mat.scale(value);
	}
	else if (target == "rotate") {
		Quaternion q(0.0, 0.0, 0.0, 1.0);
		q.setEuler(value.x, value.y, value.z);
		mat *= q.calculateMatrix();
	}
	else {
		REGEN_WARN("Unknown distribute target '" << target << "'.");
	}
}

static void transformMatrix(
		SceneParser *parser,
		SceneInputNode &input,
		const ref_ptr<State> &state,
		const ref_ptr<ShaderInputMat4> &matrixInput,
		GLuint numInstances) {
	auto &childs = input.getChildren();
	auto *matrices = (Mat4f *) matrixInput->clientDataPtr();

	for (auto it = childs.begin(); it != childs.end(); ++it) {
		ref_ptr<SceneInputNode> child = *it;
		list<GLuint> indices = child->getIndexSequence(numInstances);

		if (child->getCategory() == "set") {
			ValueGenerator<Vec3f> generator(child.get(), indices.size(),
											child->getValue<Vec3f>("value", Vec3f(0.0f)));
			const auto target = child->getValue<string>("target", "translate");

			for (auto it = indices.begin(); it != indices.end(); ++it) {
				transformMatrix(target, matrices[*it], generator.next());
			}
		}
		else if (child->getCategory() == "animation") {
			REGEN_INFO("TransformAnimation");
			auto transformAnimation = ref_ptr<TransformAnimation>::alloc(matrixInput);

			if (child->hasAttribute("mesh-id")) {
						REGEN_INFO("TransformAnimation: mesh-id");
				auto meshID = child->getValue("mesh-id");
				auto meshIndex = child->getValue<GLuint>("mesh-index", 0u);
				auto meshVec = parser->getResources()->getMesh(parser, meshID);
				if (meshVec.get() != nullptr && meshVec->size()>meshIndex) {
						REGEN_INFO("TransformAnimation: has mesh");
					auto mesh = (*meshVec.get())[meshIndex];
					transformAnimation->setMesh(mesh);
				}
			}

			for (auto &keyFrameNode : child->getChildren("key-frame")) {
				std::optional<Vec3f> framePos = nullopt;
				std::optional<Vec3f> frameDir = nullopt;
				if (keyFrameNode->hasAttribute("position")) {
					framePos = keyFrameNode->getValue<Vec3f>("position", Vec3f(0.0f));
				}
				if (keyFrameNode->hasAttribute("rotation")) {
					frameDir = keyFrameNode->getValue<Vec3f>("rotation", Vec3f(0.0f));
				}
				auto dt = keyFrameNode->getValue<GLdouble>("dt", 1.0);
				transformAnimation->push_back(framePos, frameDir, dt);
			}
			state->attach(transformAnimation);
		}
		else {
			for (auto it = indices.begin(); it != indices.end(); ++it) {
				transformMatrix(child->getCategory(), matrices[*it],
								child->getValue<Vec3f>("value", Vec3f(0.0f)));
			}
		}
	}
}

namespace regen {
	namespace scene {
		/**
		 * Processes SceneInput and creates Transform state.
		 */
		class TransformStateProvider : public StateProcessor {
		public:
			TransformStateProvider()
					: StateProcessor(REGEN_TRANSFORM_STATE_CATEGORY) {}

			// Override
			void processInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<State> &state) override {
				ref_ptr<ModelTransformation> transform = parser->getResources()->getTransform(parser, input.getName());
				if (transform.get() != nullptr) {
					state->joinStates(transform);
					return;
				}
				ref_ptr<SceneInputNode> transformNode = parser->getRoot()->getFirstChild(REGEN_TRANSFORM_STATE_CATEGORY,
																						 input.getName());
				if (transformNode.get() != nullptr && transformNode.get() != &input) {
					processInput(parser, *transformNode.get(), state);
					return;
				}

				bool isInstanced = input.getValue<bool>("is-instanced", false);
				auto numInstances = input.getValue<GLuint>("num-instances", 1u);
				transform = ref_ptr<ModelTransformation>::alloc();

				// Handle instanced model matrix
				if (isInstanced && numInstances > 1) {
					transform->get()->setInstanceData(numInstances, 1, nullptr);
					auto *matrices = (Mat4f *) transform->get()->clientDataPtr();
					for (GLuint i = 0; i < numInstances; i += 1) matrices[i] = Mat4f::identity();
					transformMatrix(parser, input, state, transform->get(), numInstances);
					// add data to vbo
					transform->setInput(transform->get());
				} else {
					transformMatrix(parser, input, state, transform->get(), 1u);
				}

				state->joinStates(transform);
				parser->getResources()->putTransform(input.getName(), transform);
			}
		};
	}
}

#endif /* REGEN_SCENE_TRANSFORM_H_ */
