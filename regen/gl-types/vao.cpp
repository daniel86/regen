#include "vao.h"

using namespace regen;

VAO::VAO()
		: GLObject(glGenVertexArrays, glDeleteVertexArrays) {
}

void VAO::resetGL() {
	glDeleteVertexArrays(1, ids_);
	glGenVertexArrays(1, ids_);
}
