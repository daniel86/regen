#include "tile-merge-generator.h"

using namespace regen;

void TileMergeGenerator::generateLOD(uint32_t lodLevel, std::vector<Vec4f> &outRectUVs) {
    // tileCount:
    uint32_t tileCount = 8;
    if (cfg_.tileCounts.size() > lodLevel) {
    	tileCount = cfg_.tileCounts[lodLevel];
    }
    uint32_t texW = tex_->width();
    uint32_t texH = tex_->height();

    uint32_t tx = tileCount;
    uint32_t ty = tileCount;
    uint32_t sx = (texW + tx - 1) / tx;
    uint32_t sy = (texH + ty - 1) / ty;

    // 1) build filled tile mask
    std::vector<uint8_t> filled(tx * ty, 0);
    for (uint32_t ty_i = 0; ty_i < ty; ++ty_i) {
        for (uint32_t tx_j = 0; tx_j < tx; ++tx_j) {
            uint32_t x0 = tx_j * sx;
            uint32_t y0 = ty_i * sy;
            uint32_t x1 = std::min(texW, x0 + sx);
            uint32_t y1 = std::min(texH, y0 + sy);
            uint32_t count = 0;
            uint32_t total = (x1 - x0) * (y1 - y0);
            for (uint32_t y = y0; y < y1; ++y) {
                for (uint32_t x = x0; x < x1; ++x) {
                    float a = tex_->sampleNearest<Vec4f>(Vec2ui(x,texH-y), tex_->textureData()).w;
                    if (a >= cfg_.alphaCut) {
						++count;
					}
                }
            }
            float coverage = total ? (float)count / total : 0.0f;
            filled[ty_i * tx + tx_j] = (coverage >= cfg_.coverageThreshold) ? 1 : 0;
        }
    }

    // 2) horizontal runs per row
    struct Run { uint32_t row, x0, x1; };
    std::vector<Run> runs;
    for (uint32_t r = 0; r < ty; ++r) {
        uint32_t c = 0;
        while (c < tx) {
            if (filled[r * tx + c]) {
                uint32_t s = c;
                while (c < tx && filled[r * tx + c]) ++c;
                uint32_t e = c - 1;
                runs.push_back({r, s, e});
            } else {
                ++c;
            }
        }
    }

    // 3) vertical merge of identical runs into rectangles
    std::vector<bool> used(runs.size(), false);
    for (size_t i = 0; i < runs.size(); ++i) {
        if (used[i]) continue;
        auto current = runs[i];
        used[i] = true;
        uint32_t r0 = current.row;
        uint32_t r1 = current.row;
        uint32_t x0 = current.x0;
        uint32_t x1 = current.x1;

        // try to extend downwards
        for (size_t j = i + 1; j < runs.size(); ++j) {
            if (used[j]) continue;
            auto &cand = runs[j];
            if (cand.x0 == x0 && cand.x1 == x1 && cand.row == r1 + 1) {
                // extend
                r1 = cand.row;
                used[j] = true;
            }
        }

        // apply padding in tile units
        int px0 = (int)x0 - (int)cfg_.padTiles;
        int py0 = (int)r0 - (int)cfg_.padTiles;
        int px1 = (int)x1 + (int)cfg_.padTiles;
        int py1 = (int)r1 + (int)cfg_.padTiles;
        px0 = std::max(px0, 0);
        py0 = std::max(py0, 0);
        px1 = std::min((int)tx - 1, px1);
        py1 = std::min((int)ty - 1, py1);

        // convert tile rect to pixel coords and then to normalized UVs
        float u0 = (float)(px0 * sx) / (float)texW;
        float v0 = (float)(py0 * sy) / (float)texH;
        float u1 = (float)std::min(texW, (px1 + 1) * sx) / (float)texW;
        float v1 = (float)std::min(texH, (py1 + 1) * sy) / (float)texH;

        outRectUVs.emplace_back(u0, v0, u1, v1);
        if (outRectUVs.size() >= cfg_.maxQuadsPerSprite) break;
    }
}
