#ifndef REGEN_SCREEN_H
#define REGEN_SCREEN_H

#include <regen/glsl/shader-input.h>

namespace regen {
	/**
	 * @brief The Screen class represents the viewport of the scene.
	 * It provides the viewport size and shader input for rendering.
	 */
	class Screen {
	public:
		/**
		 * @brief Constructs a Screen object with the specified viewport size.
		 * @param viewport The size of the viewport.
		 */
		explicit Screen(const Vec2i &viewport);

		/**
		 * @return the viewport size.
		 */
		const Vec2i &viewport() const { return v_viewport_; }

		/**
		 * @return the viewport shader input.
		 */
		const ref_ptr<ShaderInput2i> &sh_viewport() const { return sh_viewport_; }

		/**
		 * @return the current stamp of the viewport shader input.
		 */
		uint32_t stamp() const { return sh_viewport_->stamp(); }

		/**
		 * @brief Sets the viewport size and updates the shader input.
		 * @param viewport The new size of the viewport.
		 */
		void setViewport(const Vec2i &viewport);

	protected:
		ref_ptr<ShaderInput2i> sh_viewport_;
		Vec2i v_viewport_;
	};

} // namespace

#endif // REGEN_SCREEN_H

