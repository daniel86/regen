#ifndef REGEN_STAMPED_H
#define REGEN_STAMPED_H

namespace regen {
	template <typename T> struct Stamped {
		T value;
		double time = 0.0;
		Stamped() : value(), time(0.0) {}
		Stamped(const T &v, double t) : value(v), time(t) {}
		bool operator==(const Stamped<T> &other) const {
			return value == other.value && time == other.time;
		}
		bool operator!=(const Stamped<T> &other) const {
			return !(*this == other);
		}
	};
} // knowrob

#endif //REGEN_STAMPED_H
