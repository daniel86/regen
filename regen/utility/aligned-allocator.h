#ifndef REGEN_ALIGNED_ALLOCATOR_H_
#define REGEN_ALIGNED_ALLOCATOR_H_

#include <memory>
#include <cstdlib>
#include <cstddef>
#include <stdexcept>

namespace regen {
	template <typename T, std::size_t Alignment>
	class AlignedAllocator {
	public:
		using value_type = T;

		AlignedAllocator() noexcept = default;

		template <typename U>
		AlignedAllocator(const AlignedAllocator<U, Alignment>&) noexcept {}

		T* allocate(std::size_t n) {
			void* ptr = nullptr;
			if (posix_memalign(&ptr, Alignment, n * sizeof(T)) != 0) {
				throw std::bad_alloc();
			}
			return reinterpret_cast<T*>(ptr);
		}

		void deallocate(T* p, std::size_t) noexcept {
			std::free(p);
		}

		// Required by standard allocator traits:
		template <typename U>
		struct rebind {
			using other = AlignedAllocator<U, Alignment>;
		};
	};

	// Comparison operators for allocator
	template <typename T1, std::size_t A1, typename T2, std::size_t A2>
	constexpr bool operator==(const AlignedAllocator<T1, A1>&, const AlignedAllocator<T2, A2>&) noexcept {
		return A1 == A2;
	}

	template <typename T1, std::size_t A1, typename T2, std::size_t A2>
	constexpr bool operator!=(const AlignedAllocator<T1, A1>&, const AlignedAllocator<T2, A2>&) noexcept {
		return A1 != A2;
	}
} // namespace

#endif /* REGEN_ALIGNED_ALLOCATOR_H_ */
