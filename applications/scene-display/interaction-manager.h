#ifndef REGEN_INTERACTION_MANAGER_H
#define REGEN_INTERACTION_MANAGER_H

#include <regen/scene/scene-interaction.h>

namespace regen {
	class InteractionManager {
	public:
		static void registerInteraction(const std::string &interactionName, const ref_ptr<SceneInteraction> &interaction) {
			interactions()[interactionName] = interaction;
		}

		static void removeInteraction(const std::string &interactionName) {
			interactions().erase(interactionName);
		}

		static ref_ptr<SceneInteraction> getInteraction(const std::string &interactionName) {
			auto it = interactions().find(interactionName);
			if (it != interactions().end()) {
				return it->second;
			}
			return {};
		}

	protected:
		static std::map<std::string, ref_ptr<SceneInteraction>>& interactions() {
			static std::map<std::string, ref_ptr<SceneInteraction>> value;
			return value;
		}
	};
}

#endif //REGEN_INTERACTION_MANAGER_H
