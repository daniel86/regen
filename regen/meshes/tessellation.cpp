/*
 * tessellation.cpp
 *
 *  Created on: Dec 27, 2013
 *      Author: daniel
 */

#include <regen/utility/logging.h>
#include "tessellation.h"

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

				out[counter] = TriangleFace(out[i].v1, pa, pc);
				++counter;
				out[counter] = TriangleFace(pa, out[i].v2, pb);
				++counter;
				out[counter] = TriangleFace(pb, out[i].v3, pc);
				++counter;
				out[i] = TriangleFace(pa, pb, pc);
			}
		}

		return out;
	}
}
