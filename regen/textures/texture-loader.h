#ifndef REGEN_TEXTURE_LOADER_H_
#define REGEN_TEXTURE_LOADER_H_

#include <stdexcept>

#include <regen/textures/texture.h>
#include <regen/utility/ref-ptr.h>
#include <regen/compute/vector.h>
#include "texture-config.h"
#include "image-data.h"

namespace regen {
	/**
	 * \brief Interface for texture loaders.
	 */
	class ITextureLoader {
	public:
		virtual ~ITextureLoader() = default;

		/**
		 * \brief Check if the loader can load the given file extension.
		 * \param fileExt the file extension to check.
		 * \return true if the loader can load the file extension, false otherwise.
		 */
		virtual bool canLoad(std::string_view fileExt) const = 0;

		/**
		 * \brief Load an image from file.
		 * \param file the file to load.
		 * \param cfg the texture configuration.
		 * \return the loaded image data.
		 * \throws Error if loading fails.
		 */
		virtual ImageDataArray load(std::string_view file, const TextureConfig& cfg) = 0;
	};

	/**
	 * \brief Registry for texture loaders.
	 *
	 * This class manages a registry of texture loaders.
	 * It allows registering new loaders and loading images using the appropriate loader based on the file extension.
	 */
	class TextureLoaderRegistry {
	public:
		static TextureLoaderRegistry& instance() {
			static TextureLoaderRegistry instance;
			return instance;
		}

		/**
		 * \brief Register a new texture loader.
		 * \param loader the texture loader to register.
		 */
		static void registerLoader(std::unique_ptr<ITextureLoader> loader);

		/**
		 * \brief Load an image from file using the appropriate loader.
		 * \param file the file to load.
		 * \param cfg the texture configuration.
		 * \return the loaded image data.
		 * \throws Error if no loader is found or if loading fails.
		 */
		static ImageDataArray load(std::string_view file, const TextureConfig& cfg);

	private:
		std::vector<std::unique_ptr<ITextureLoader>> loaders_;
	};
}

namespace regen::textures {
	/**
	 * \brief An error occurred loading the Texture.
	 */
	class Error : public std::runtime_error {
	public:
		/**
		 * @param message the error message.
		 */
		explicit Error(const std::string &message) : std::runtime_error(message) {}
	};

	/**
	 * Load a Texture from file. Guess if it is a Texture2D or Texture3D.
	 * Force specified internal format.
	 * Scale to forced size (if forced size != 0).
	 * Setup mipmapping after loading the file.
	 */
	ref_ptr<Texture> load(std::string_view file, const TextureConfig &texCfg = TextureConfig::getDefault());

	/**
	 * Update a Texture from file.
	 * @param tex the Texture to update.
	 * @param file the file to load.
	 */
	void reload(const ref_ptr<Texture> &tex, std::string_view file);

	/**
	 * Load a Texture from RAW data. Guess if it is a Texture2D or Texture3D.
	 * Force specified internal format.
	 * Scale to forced size (if forced size != 0).
	 * Setup mipmapping after loading the file.
	 */
	ref_ptr<Texture> load(
			uint32_t textureType,
			uint32_t numBytes,
			const void *rawData,
			const TextureConfig &texCfg = TextureConfig::getDefault());

	/**
	 * Load a Texture2DArray from file.
	 * Force specified internal format.
	 * Scale to forced size (if forced size != 0).
	 * Setup mipmapping after loading the file.
	 */
	ref_ptr<Texture2DArray> loadArray(
			const std::string &textureDirectory,
			const std::string &textureNamePattern,
			const TextureConfig &texCfg = TextureConfig::getDefault());

	/**
	 * Load a Texture2DArray from file.
	 * Force specified internal format.
	 * Scale to forced size (if forced size != 0).
	 * Setup mipmapping after loading the file.
	 */
	ref_ptr<Texture2DArray> loadArray(
			const std::vector<TextureDescription> &textureDescriptions,
			const TextureConfig &texCfg = TextureConfig::getDefault());

	/**
	 * Load a TextureCube from file.
	 * The file is expected to be a regular 2D image containing
	 * multiple faces arranged next to each other.
	 * Force specified internal format.
	 * Scale to forced size (if forced size != 0).
	 * Setup mipmapping after loading the file.
	 */
	ref_ptr<TextureCube> loadCube(
			std::string_view file,
			bool flipBackFace = false,
			const TextureConfig &texCfg = TextureConfig::getDefault());

	/**
	 * Loads RAW Texture from file.
	 */
	ref_ptr<Texture> loadRAW(
			const std::string &path,
			const Vec3ui &size,
			uint32_t numComponents,
			uint32_t bytesPerComponent);

	/**
	 * 1D texture that contains a color spectrum.
	 */
	ref_ptr<Texture> loadSpectrum(
			double t1,
			double t2,
			int numTexels,
			bool useMipmaps = true);
}
// namespace

#endif /* REGEN_TEXTURE_LOADER_H_ */
