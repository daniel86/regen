#ifndef REGEN_TEXTURE_CONFIG_H_
#define REGEN_TEXTURE_CONFIG_H_

#include <string>
#include <utility>
#include <regen/compute/vector.h>

namespace regen {
	/**
	 * \brief Configuration for a texture.
	 *
	 * This class is used to configure the properties of a texture.
	 * It can be used to specify mipmaps, internal format, size, etc.
	 */
	struct TextureConfig {
		bool useMipmaps = false;
		bool keepData = false;
		GLenum forcedInternalFormat = GL_NONE;
		GLenum forcedFormat = GL_NONE;
		Vec3ui forcedSize = Vec3ui::zero();

		static TextureConfig& getDefault() {
			static TextureConfig defaultConfig;
			return defaultConfig;
		}
	};

	/**
	 * \brief Description of a texture.
	 *
	 * This class is used to describe a texture, either by its file name or by clear data.
	 * It can be used to load textures from files or to create textures with clear data.
	 */
	struct TextureDescription {
		explicit TextureDescription(std::string_view fileName) : fileName(fileName) {}
		explicit TextureDescription(byte *clearData) : clearData(clearData) {}

		bool operator<(const TextureDescription &other) const {
			return fileName < other.fileName || clearData < other.clearData;
		}

		bool isFile() const { return !fileName.empty(); }
		bool isClear() const { return clearData != nullptr; }

		const std::string& getFileName() const { return fileName; }
		byte* getClearData() const { return clearData; }
	private:
		std::string fileName;
		byte *clearData = nullptr;
	};
} // namespace

#endif /* REGEN_TEXTURE_CONFIG_H_ */
