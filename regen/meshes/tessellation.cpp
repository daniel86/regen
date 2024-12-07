/*
 * tessellation.cpp
 *
 *  Created on: Dec 27, 2013
 *      Author: daniel
 */

#include <unordered_map>
#include <utility>
#include <regen/utility/logging.h>
#include "tessellation.h"

struct Edge {
    GLuint v1, v2;
    bool operator==(const Edge &other) const {
        return (v1 == other.v1 && v2 == other.v2) || (v1 == other.v2 && v2 == other.v1);
    }
};

namespace std {
    template <>
    struct hash<Edge> {
        std::size_t operator()(const Edge &e) const {
            return std::hash<GLuint>()(e.v1) ^ std::hash<GLuint>()(e.v2);
        }
    };
}

namespace regen {
	std::vector<TriangleFace> tessellate(GLuint lod, TriangleFace &in) {
		std::vector<TriangleFace> in_(1);
		in_[0] = in;
		return tessellate(lod, in_);
	}

	std::vector<TriangleFace> tessellate(GLuint lod, std::vector<TriangleFace> &in) {
		GLuint inFaces = in.size();
		// Each triangle is divided in 4 smaller triangles
		// for each LoD level.
		GLuint outFaces = pow(4.0, lod) * inFaces;
		std::vector<TriangleFace> out(outFaces);
		GLuint counter = inFaces;
		TriangleVertex pa, pb, pc;
		GLuint i, j;

		GLuint lastIndex = 0;
		for (i = 0; i < inFaces; ++i) {
			out[i] = in[i];

			auto *vertices = (TriangleVertex *) &in[i];
			for (j = 0; j < 3; ++j) {
				if (lastIndex < vertices[j].i) {
					lastIndex = vertices[j].i;
				}
			}
		}

		for (j = 0; j < lod; ++j) {
			const GLuint lastCount = counter;
			for (i = 0; i < lastCount; ++i) {
				pa.p = (out[i].v1.p + out[i].v2.p) * 0.5;
				pb.p = (out[i].v2.p + out[i].v3.p) * 0.5;
				pc.p = (out[i].v3.p + out[i].v1.p) * 0.5;
				pa.i = lastIndex + 1;
				pb.i = lastIndex + 2;
				pc.i = lastIndex + 3;
				lastIndex += 3;

				out[counter++] = TriangleFace(out[i].v1, pa, pc);
				out[counter++] = TriangleFace(pa, out[i].v2, pb);
				out[counter++] = TriangleFace(pb, out[i].v3, pc);
				out[i] = TriangleFace(pa, pb, pc);
			}
		}

		return out;
	}

	void tessellate(GLuint lod, Tessellation &tess) {
		GLuint inFaces = tess.inputFaces.size();
		// Each triangle is divided in 4 smaller triangles
		// for each LoD level.
		GLuint outFaces = pow(4.0, lod) * inFaces;
		tess.outputFaces.resize(outFaces);
		GLuint counter = inFaces;
		GLuint pa_i, pb_i, pc_i;
		GLuint i, j;

		std::unordered_map<Edge, GLuint> edgeMidpointMap;
		GLuint lastIndex = tess.vertices.size() - 1;

		for (i = 0; i < inFaces; ++i) {
			tess.outputFaces[i] = tess.inputFaces[i];
		}

		for (j = 0; j < lod; ++j) {
			const GLuint lastCount = counter;
			for (i = 0; i < lastCount; ++i) {
				auto &face = tess.outputFaces[i];
				auto addMidpoint = [&](GLuint v1, GLuint v2) {
					Edge edge = {v1, v2};
					if (edgeMidpointMap.find(edge) == edgeMidpointMap.end()) {
						tess.vertices.push_back((tess.vertices[v1] + tess.vertices[v2]) * 0.5);
						edgeMidpointMap[edge] = ++lastIndex;
					}
					return edgeMidpointMap[edge];
				};

				pa_i = addMidpoint(face.v1, face.v2);
				pb_i = addMidpoint(face.v2, face.v3);
				pc_i = addMidpoint(face.v3, face.v1);

				tess.outputFaces[counter++] = TessellationFace(face.v1, pa_i, pc_i);
				tess.outputFaces[counter++] = TessellationFace(pa_i, face.v2, pb_i);
				tess.outputFaces[counter++] = TessellationFace(pb_i, face.v3, pc_i);
				tess.outputFaces[i] = TessellationFace(pa_i, pb_i, pc_i);
			}
		}
	}
}
