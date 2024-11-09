#ifndef POINT_STATE_H_
#define POINT_STATE_H_

#include <regen/meshes/mesh-state.h>

namespace regen {
	class Point : public Mesh {
	public:
		Point();
	protected:
		ref_ptr<ShaderInput3f> pos_;
	};
} // namespace

#endif /* POINT_STATE_H_ */
