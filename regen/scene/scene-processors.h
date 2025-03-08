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
#include <regen/scene/scene-input.h>

namespace regen {
	namespace scene {
		class SceneLoader;

		/**
		 * Processing of Scene input.
		 */
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

		protected:
			std::string category_;
		};

		/**
		 * Processing of Scene input to StateNode instances.
		 */
		class NodeProcessor : public InputProcessor {
		public:
			/**
			 * Default Constructor.
			 * @param category The Processor category.
			 */
			explicit NodeProcessor(const std::string &category)
					: InputProcessor(category) {}

			virtual void processInput(
					regen::scene::SceneLoader *scene,
					SceneInputNode &input,
					const ref_ptr<StateNode> &parent) = 0;
		};

		/**
		 * Processing of Scene input to State instances.
		 */
		class StateProcessor : public InputProcessor {
		public:
			/**
			 * Default Constructor.
			 * @param category The Processor category.
			 */
			explicit StateProcessor(const std::string &category)
					: InputProcessor(category) {}

			virtual void processInput(
					regen::scene::SceneLoader *scene,
					SceneInputNode &input,
					const ref_ptr<StateNode> &parent,
					const ref_ptr<State> &state) = 0;
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
					regen::scene::SceneLoader *scene,
					SceneInputNode &input,
					const ref_ptr<StateNode> &parent,
					const ref_ptr<State> &state) override;
		};
	}
}


#endif /* REGEN_SCENE_INPUT_PROCESSORS_H_ */
