#ifndef REGEN_DEBUG_INTERFACE_H
#define REGEN_DEBUG_INTERFACE_H

#include <regen/math/vector.h>

namespace regen {
	class DebugInterface {
	public:
		virtual void drawLine(const Vec3f &from, const Vec3f &to, const Vec3f &color) = 0;

		virtual void drawCircle(const Vec3f &center, float radius, const Vec3f &color) = 0;
	};
}

#endif //REGEN_DEBUG_INTERFACE_H
