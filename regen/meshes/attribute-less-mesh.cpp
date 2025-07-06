#include "attribute-less-mesh.h"

using namespace regen;

AttributeLessMesh::AttributeLessMesh(GLuint numVertices)
		: Mesh(GL_POINTS, BUFFER_HINT_STATIC) {
	inputContainer_->set_numVertices(numVertices);
}
