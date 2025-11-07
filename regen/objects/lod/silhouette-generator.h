#ifndef REGEN_SILHOUETTE_GENERATOR_H
#define REGEN_SILHOUETTE_GENERATOR_H

#include <regen/objects/mesh.h>

#include <utility>

namespace regen {
	/**
	 * Configuration for silhouette generation.
	 * This defines how the silhouette generator will create the silhouette data.
	 */
	struct SilhouetteConfig {
		// per-LOD tile grid dimension (e.g. {16,8,4})
		std::vector<uint32_t> tileCounts = {8u};
		// alpha threshold to consider pixel opaque
        float alphaCut = 0.01f;
        // tile coverage threshold to mark tile filled
        float coverageThreshold = 0.05f;
        // inflate rects by this many tiles
        uint32_t padPixels = 1u;
	};

	/**
	 * Base class for silhouette generators.
	 * This class defines the interface for generating silhouette data.
	 */
	class SilhouetteGenerator {
	public:
		/**
		 * Constructor.
		 * @param tex the texture to use for silhouette generation
		 * @param cfg configuration for silhouette generation
		 */
		explicit SilhouetteGenerator(
				const ref_ptr<Texture2D> &tex,
				const SilhouetteConfig &cfg = SilhouetteConfig())
				: cfg_(cfg), tex_(tex) {}

		virtual ~SilhouetteGenerator() = default;

		/**
		 * Generate the silhouette data for a specific LOD level.
		 * @param lodLevel the LOD level to generate
		 * @param outRectUVs output vector to store the UV rectangles for the silhouette
		 */
		virtual void generateLOD(uint32_t lodLevel, std::vector<Vec4f> &outRectUVs) = 0;

	protected:
		SilhouetteConfig cfg_;
		ref_ptr<Texture2D> tex_;
	};
} // namespace

#endif /* REGEN_SILHOUETTE_GENERATOR_H */
