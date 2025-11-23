#ifndef REGEN_TEXTURE_LOCATION_H_
#define REGEN_TEXTURE_LOCATION_H_

#include <map>
#include <set>

#include "regen/shader/shader-input.h"
#include <regen/textures/texture.h>

namespace regen {
	/**
	 * \brief Maps texture to shader location.
	 */
	struct TextureLocation {
		std::string name;    /**< name in shader. **/
		int location; /**< the texture location. */
		ref_ptr<Texture> tex; /**< the texture. */
		int uploadChannel; /**< last uploaded channel. */
		/**
		 * @param _name texture name.
		 * @param _tex the texture.
		 * @param _location texture location in shader.
		 */
		TextureLocation(const std::string &_name, const ref_ptr<Texture> &_tex, int _location)
				: name(_name), location(_location), tex(_tex), uploadChannel(-1) {}

		TextureLocation()
				: name(""), location(-1), uploadChannel(-1) {}
	};
} // namespace

#endif /* REGEN_TEXTURE_LOCATION_H_ */
