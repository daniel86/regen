#ifndef REGEN_TILE_MERGE_GENERATOR_H
#define REGEN_TILE_MERGE_GENERATOR_H

#include "silhouette-generator.h"

namespace regen {
	/**
	 * TileMergeGenerator is a silhouette generator that merges tiles based on the alpha channel of a texture.
	 * It generates UV rectangles for each tile that is considered filled based on the alpha threshold.
	 */
	class TileMergeGenerator : public SilhouetteGenerator {
	public:
		explicit TileMergeGenerator(const ref_ptr<Texture2D> &tex, const SilhouetteConfig &cfg = SilhouetteConfig())
				: SilhouetteGenerator(tex, cfg) {
		}

		~TileMergeGenerator() override = default;

		void generateLOD(uint32_t lodLevel, std::vector<Vec4f> &outRectUVs) override;
	};
} // namespace

#endif /* REGEN_TILE_MERGE_GENERATOR_H */
