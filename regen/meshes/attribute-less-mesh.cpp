#include "attribute-less-mesh.h"

using namespace regen;

AttributeLessMesh::AttributeLessMesh(GLuint numVertices)
		: Mesh(GL_POINTS, BufferUpdateFlags{ BUFFER_UPDATE_NEVER, BUFFER_UPDATE_FULLY }) {
	inputContainer_->set_numVertices(numVertices);
}
