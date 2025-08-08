#ifndef REGEN_TEXTURE_BINDER_H_
#define REGEN_TEXTURE_BINDER_H_

#include "texture.h"

namespace regen {
	/**
	 * \brief A ring-buffer like binding manager for textures.
	 *
	 * The class provides a global singleton that manages texture bindings trying to
	 * avoid re-binding textures that are already bound to a texture unit.
	 */
	class TextureBinder {
	public:
		TextureBinder();

		~TextureBinder() = default;

		/**
		 * Reset the texture binder, clearing all bindings.
		 */
		static void reset();

		/**
		 * Bind a texture to the next available texture unit.
		 * If the texture is already bound, it will return the unit it was bound to.
		 * @param tex The texture to bind.
		 * @return The texture unit the texture was bound to.
		 */
		static int32_t bind(Texture *tex);

		/**
		 * Release a texture, clearing its binding.
		 * This does not delete the texture, it just clears the binding.
		 * @param tex The texture to release.
		 */
		static void release(Texture *tex);

		/**
		 * Rebind a texture to its current binding unit.
		 * This is useful if the texture's properties have changed and it needs to be re-bound.
		 * @param tex The texture to rebind.
		 */
		static void rebind(Texture *tex);
	private:
		std::vector<Texture*> activeBindings_; //!< The currently bound textures.
		std::vector<uint32_t> activeBindingIDs_; //!< The IDs of the currently bound textures.
		int32_t nextUnit_ = 0; //!< The next texture unit to bind to.

		static TextureBinder &instance() {
			// Note: intentional shutdown leak, as textures might access this
			//  in their destructor on system shutdown, after the static memory
			//  has been cleared!
			thread_local static auto* binder = new TextureBinder();
			return *binder;
		}

		void bind(Texture *tex, int32_t unit);
	};
} // namespace

#endif /* REGEN_TEXTURE_BINDER_H_ */
