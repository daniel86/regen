#include "attribute-less-mesh.h"

using namespace regen;

AttributeLessMesh::AttributeLessMesh(GLuint numVertices)
		: Mesh(GL_POINTS, BufferUpdateFlags::NEVER) {
	inputContainer_->set_numVertices(numVertices);
}
