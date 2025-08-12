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
		auto viewport() const { return sh_viewport_->getVertex(0); }

		/**
		 * @return the viewport shader input.
		 */
		const ref_ptr<ShaderInput2i> &sh_viewport() const { return sh_viewport_; }

		/**
		 * @return the current stamp of the viewport shader input.
		 */
		uint32_t stampOfReadData() const { return sh_viewport_->stampOfReadData(); }

		uint32_t stampOfWriteData() const { return sh_viewport_->stampOfWriteData(); }

		/**
		 * @brief Sets the viewport size and updates the shader input.
		 * @param viewport The new size of the viewport.
		 */
		void setViewport(const Vec2i &viewport);

	protected:
		ref_ptr<ShaderInput2i> sh_viewport_;
	};

} // namespace

#endif // REGEN_SCREEN_H

