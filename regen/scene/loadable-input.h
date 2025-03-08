#ifndef REGEN_LOADABLE_INPUT_H_
#define REGEN_LOADABLE_INPUT_H_

#include <regen/scene/scene-processors.h>
#include <regen/scene/resource-processor.h>

namespace regen {
	template<class T>
	class LoadableResource : public scene::ResourceProcessor<T> {
	public:
		explicit LoadableResource(const std::string &category)
				: scene::ResourceProcessor<T>(category) {}

		ref_ptr<T> createResource(
				scene::SceneLoader *parser, scene::SceneInputNode &input) override {
			LoadingContext ctx(parser, {});
			return T::load(ctx, input);
		}
	};

	template<class T>
	class LoadableState : public scene::StateProcessor {
	public:
		explicit LoadableState(const std::string &category) : scene::StateProcessor(category) {}

		void processInput(
				scene::SceneLoader *scene,
				scene::SceneInputNode &input,
				const ref_ptr<StateNode> &parent,
				const ref_ptr<State> &state) override {
			LoadingContext ctx(scene, parent);
			auto typedState = T::load(ctx, input);
			if (typedState.get() == nullptr) {
				REGEN_WARN("Unable to load State for '" << input.getDescription() << "'.");
			} else {
				state->joinStates(typedState);
			}
		}
	};

	template<class T>
	class LoadableState2 : public scene::StateProcessor {
	public:
		explicit LoadableState2(const std::string &category) : scene::StateProcessor(category) {}

		void processInput(
				scene::SceneLoader *scene,
				scene::SceneInputNode &input,
				const ref_ptr<StateNode> &parent,
				const ref_ptr<State> &state) override {
			LoadingContext ctx(scene, parent);
			auto typedState = T::load(ctx, input, state);
			if (typedState.get() == nullptr) {
				REGEN_WARN("Unable to load State for '" << input.getDescription() << "'.");
			}
		}
	};

	template<class T>
	class StateResource : public scene::StateProcessor {
	public:
		explicit StateResource(const std::string &category) : scene::StateProcessor(category) {}

		void processInput(
				scene::SceneLoader *scene,
				scene::SceneInputNode &input,
				const ref_ptr<StateNode> &parent,
				const ref_ptr<State> &state) override {
			auto x = scene->getResource<T>(input.getName());
			if (x.get() == nullptr) {
				REGEN_WARN("Unable to load state for '" << input.getDescription() << "'.");
			} else {
				state->joinStates(x);
			}
		}
	};

	template<class T>
	class LoadableNode : public scene::NodeProcessor {
	public:
		explicit LoadableNode(const std::string &category) : scene::NodeProcessor(category) {}

		void processInput(
				scene::SceneLoader *scene,
				scene::SceneInputNode &input,
				const ref_ptr<StateNode> &parent) override {
			LoadingContext ctx(scene, parent);
			auto typedState = T::load(ctx, input);
			if (typedState.get() == nullptr) {
				REGEN_WARN("Unable to load State for '" << input.getDescription() << "'.");
			}
		}
	};
}

#endif /* REGEN_LOADABLE_INPUT_H_ */
