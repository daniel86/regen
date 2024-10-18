/*
 * input-processors.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_INPUT_PROCESSORS_H_
#define REGEN_SCENE_INPUT_PROCESSORS_H_

#include <regen/states/state.h>
#include <regen/states/state-node.h>

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/resource-manager.h>

namespace regen {
	namespace scene {
		/**
		 * Processing of Scene input.
		 */
		template<class T>
		class InputProcessor {
		public:
			/**
			 * Default Constructor.
			 * @param category The Processor category.
			 */
			explicit InputProcessor(const std::string &category)
					: category_(category) {}

			virtual ~InputProcessor() = default;

			/**
			 * @return The node category of this processor.
			 */
			const std::string &category() { return category_; }

			/**
			 * Process given input node, manipulate parent.
			 * @param parser The SceneParser.
			 * @param input The SceneInputNode providing input data.
			 * @param parent The parent state.
			 */
			virtual void processInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<T> &parent) = 0;

		protected:
			std::string category_;
		};

		/**
		 * Processing of Scene input to StateNode instances.
		 */
		class NodeProcessor : public InputProcessor<StateNode> {
		public:
			/**
			 * Default Constructor.
			 * @param category The Processor category.
			 */
			explicit NodeProcessor(const std::string &category)
					: InputProcessor(category) {}
		};

		/**
		 * Processing of Scene input to State instances.
		 */
		class StateProcessor : public InputProcessor<State> {
		public:
			/**
			 * Default Constructor.
			 * @param category The Processor category.
			 */
			explicit StateProcessor(const std::string &category)
					: InputProcessor(category) {}
		};

		class ResourceStateProvider : public StateProcessor {
		public:
			/**
			 * Default Constructor.
			 * @param category The Processor category.
			 */
			ResourceStateProvider()
					: StateProcessor("resource") {}

			void processInput(
					SceneParser *parser,
					SceneInputNode &input,
					const ref_ptr<State> &parent) {
				parser->getResources()->loadResources(parser, input.getName());
			}
		};
	}
}


#endif /* REGEN_SCENE_INPUT_PROCESSORS_H_ */
