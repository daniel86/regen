#include <regen/textures/texture-state.h>
#include <regen/av/video-texture.h>
#include <regen/scene/scene-interaction.h>

namespace regen {
	class VideoToggleInteration : public SceneInteraction {
	public:
		bool interactWith(const ref_ptr<StateNode> &node) override {
			bool hasInteracted = false;
			node->foreachWithType<TextureState>([&hasInteracted](TextureState &state) {
				hasInteracted = interactWithVideo(state);
				return hasInteracted;
			});
			return hasInteracted;
		}

		static bool interactWithVideo(TextureState &state) {
#ifdef HAS_AV_LIBS
			auto &tex = state.texture();
			if (tex.get()) {
				auto *video = dynamic_cast<VideoTexture*>(tex.get());
				if (video) {
					video->togglePlay();
					return true;
				}
			}
#endif
			return false;
		}
	};

} // regen
