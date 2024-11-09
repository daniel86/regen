#include "point.h"

using namespace regen;

///////////

Point::Point()
		: Mesh(GL_POINTS, VBO::USAGE_STREAM) {
	inputContainer_->set_numVertices(1);

	pos_ = ref_ptr<ShaderInput3f>::alloc("pos");
	pos_->setVertexData(1);
	pos_->setVertex(0, Vec3f(0.0, 0.0, 0.0));

	begin(ShaderInputContainer::INTERLEAVED);
	setInput(pos_);
	end();
}
