#include "tile-merge-generator.h"

using namespace regen;

void TileMergeGenerator::generateLOD(uint32_t lodLevel, std::vector<Vec4f> &outRectUVs) {
    // The idea here is simple: divide texture into T x T tiles, check for
    // each if there are enough opaque pixels, if yes mark tile filled.
    // Then merge filled tiles into rectangles, first horizontally, then vertically.

    uint32_t T = 8;
    if (cfg_.tileCounts.size() > lodLevel) {
    	T = cfg_.tileCounts[lodLevel];
    }
    const uint32_t texW = tex_->width();
    const uint32_t texH = tex_->height();
    const uint32_t sx = (texW + T - 1) / T;
    const uint32_t sy = (texH + T - 1) / T;

    // 1) build filled tile mask
    std::vector<uint8_t> filled(T * T, 0);
    for (uint32_t ty_i = 0; ty_i < T; ++ty_i) {
        for (uint32_t tx_j = 0; tx_j < T; ++tx_j) {
            uint32_t x0 = tx_j * sx;
            uint32_t y0 = ty_i * sy;
            uint32_t x1 = std::min(texW, x0 + sx);
            uint32_t y1 = std::min(texH, y0 + sy);
            uint32_t count = 0;
            uint32_t total = (x1 - x0) * (y1 - y0);
            for (uint32_t y = y0; y < y1; ++y) {
                for (uint32_t x = x0; x < x1; ++x) {
                    float a = tex_->sampleNearest<Vec4f>(Vec2ui(x,texH-y-1), tex_->textureData()).w;
                    if (a >= cfg_.alphaCut) {
						++count;
					}
                }
            }
            float coverage = total ? (float)count / total : 0.0f;
            filled[ty_i * T + tx_j] = (coverage >= cfg_.coverageThreshold) ? 1 : 0;
        }
    }

    // 2) horizontal runs per row
    struct Run { uint32_t row, x0, x1; };
    std::vector<Run> runs;
    for (uint32_t r = 0; r < T; ++r) {
        uint32_t c = 0;
        while (c < T) {
            if (filled[r * T + c]) {
                uint32_t s = c;
                while (c < T && filled[r * T + c]) ++c;
                uint32_t e = c - 1;
                runs.push_back({r, s, e});
            } else {
                ++c;
            }
        }
    }

    // 3) vertical merge of identical runs into rectangles
    struct Quad { uint32_t x0, x1, y0, y1; };
    std::vector<Quad> quads;
    std::vector<bool> used(runs.size(), false);
    for (size_t i = 0; i < runs.size(); ++i) {
        if (used[i]) continue;
        auto current = runs[i];
        used[i] = true;
        uint32_t r0 = current.row;
        uint32_t r1 = current.row;
        uint32_t x0 = current.x0;
        uint32_t x1 = current.x1;

        for (size_t j = i + 1; j < runs.size(); ++j) {
            if (used[j]) continue;
            auto &cand = runs[j];
            if (cand.x0 == x0 && cand.x1 == x1 && cand.row == r1 + 1) {
                r1 = cand.row;
                used[j] = true;
            }
        }
        quads.push_back({x0, x1, r0, r1});
    }

    // 4) shrink quads based on filled mask (remove empty rows/cols)
    for (auto &q : quads) {
		uint32_t px0 = q.x0 * sx;
		uint32_t py0 = q.y0 * sy;
		uint32_t px1 = std::min(texW, (q.x1 + 1) * sx);
		uint32_t py1 = std::min(texH, (q.y1 + 1) * sy);

		// top
		while (py0 < py1) {
			bool rowHasAlpha = false;
			for (uint32_t x = px0; x < px1; ++x) {
				float a = tex_->sampleNearest<Vec4f>(
					Vec2ui(x, texH - py0 - 1), tex_->textureData()).w;
				if (a >= cfg_.alphaCut) { rowHasAlpha = true; break; }
			}
			if (rowHasAlpha) break;
			++py0;
		}

		// bottom
		while (py1 > py0) {
			bool rowHasAlpha = false;
			for (uint32_t x = px0; x < px1; ++x) {
				float a = tex_->sampleNearest<Vec4f>(
					Vec2ui(x, texH - py1), tex_->textureData()).w;
				if (a >= cfg_.alphaCut) { rowHasAlpha = true; break; }
			}
			if (rowHasAlpha) break;
			--py1;
		}

		// left
		while (px0 < px1) {
			bool colHasAlpha = false;
			for (uint32_t y = py0; y < py1; ++y) {
				float a = tex_->sampleNearest<Vec4f>(
					Vec2ui(px0, texH - y - 1), tex_->textureData()).w;
				if (a >= cfg_.alphaCut) { colHasAlpha = true; break; }
			}
			if (colHasAlpha) break;
			++px0;
		}

		// right
		while (px1 > px0) {
			bool colHasAlpha = false;
			for (uint32_t y = py0; y < py1; ++y) {
				float a = tex_->sampleNearest<Vec4f>(
					Vec2ui(px1 - 1, texH - y - 1), tex_->textureData()).w;
				if (a >= cfg_.alphaCut) { colHasAlpha = true; break; }
			}
			if (colHasAlpha) break;
			--px1;
		}

		// optional pixel-level padding
		const uint32_t pad = cfg_.padPixels; // e.g., 1 or 2 pixels
		px0 = (px0 >= pad) ? px0 - pad : 0;
		py0 = (py0 >= pad) ? py0 - pad : 0;
		px1 = std::min(px1 + pad, texW);
		py1 = std::min(py1 + pad, texH);

		outRectUVs.emplace_back(
			float(px0) / float(texW),
			float(py0) / float(texH),
			float(px1) / float(texW),
			float(py1) / float(texH));
    }
}
