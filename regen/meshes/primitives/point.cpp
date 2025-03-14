#include "point.h"

using namespace regen;

///////////

Point::Point(GLuint numVertices)
		: Mesh(GL_POINTS, VBO::USAGE_STREAM) {
	inputContainer_->set_numVertices(numVertices);

	pos_ = ref_ptr<ShaderInput3f>::alloc("pos");
	pos_->setVertexData(numVertices);

	if (numVertices == 1) {
		pos_->setVertex(0, Vec3f(0.0, 0.0, 0.0));
		begin(ShaderInputContainer::INTERLEAVED);
		setInput(pos_);
		end();
	}

	// note: use a small epsilon to avoid division by zero
	minPosition_ = Vec3f(-1.0e-8f);
	maxPosition_ = Vec3f(1.0e-8f);
}
