#ifndef POINT_STATE_H_
#define POINT_STATE_H_

#include <regen/objects/mesh.h>

namespace regen {
	class Point : public Mesh {
	public:
		explicit Point(GLuint numVertices=1);
	protected:
		ref_ptr<ShaderInput3f> pos_;
	};
} // namespace

#endif /* POINT_STATE_H_ */
