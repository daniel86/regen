#include "attribute-less-mesh.h"

using namespace regen;

AttributeLessMesh::AttributeLessMesh(GLuint numVertices)
		: Mesh(GL_POINTS, BufferUpdateFlags::NEVER) {
	set_numVertices(numVertices);
}
