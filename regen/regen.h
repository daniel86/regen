#ifndef REGEN_REGEN_H
#define REGEN_REGEN_H

#include <vector>
#include <span>
#include <cstdint>

namespace regen {
#ifndef byte
	typedef unsigned char byte;
#endif

	template <typename T>
	static inline void setClamped(std::vector<T> &vec, uint32_t idx, const T &value) {
		vec[vec.size() <= idx ? 0u : idx] = value;
	}

	template <typename T>
	static inline void setClamped(std::span<T> &vec, uint32_t idx, const T &value) {
		vec[vec.size() <= idx ? 0u : idx] = value;
	}

	template <typename T>
	static inline const T &getClamped(const std::vector<T> &vec, uint32_t idx) {
		return vec.size() <= idx ? vec[0] : vec[idx];
	}

	template <typename T>
	static inline const T &getClamped(const std::span<T> &vec, uint32_t idx) {
		return vec.size() <= idx ? vec[0] : vec[idx];
	}
};
#endif //REGEN_REGEN_H
