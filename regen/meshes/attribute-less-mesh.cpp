#include "attribute-less-mesh.h"

using namespace regen;

AttributeLessMesh::AttributeLessMesh(GLuint numVertices)
		: Mesh(GL_POINTS, BUFFER_USAGE_STATIC_DRAW) {
	inputContainer_->set_numVertices(numVertices);
}
