#ifndef REGEN_ALIGNED_ARRAY_H_
#define REGEN_ALIGNED_ARRAY_H_

#include <memory>
#include <cstdlib>
#include <cstddef>
#include <stdexcept>

namespace regen {
	template<typename T>
	struct AlignedArray {
	public:
		explicit AlignedArray(size_t maxElements)
			: capacity_(maxElements) {
			data_ = static_cast<T*>(std::aligned_alloc(32, maxElements * sizeof(T)));
		}

		AlignedArray()
			: data_(nullptr), capacity_(0) {
			// Default constructor initializes to empty state
		}

		~AlignedArray() { std::free(data_); }

		AlignedArray(const AlignedArray&) = delete;

		void setToZero() {
			memset(data_, 0, capacity_ * sizeof(T));
		}

		void resize(size_t newSize) {
			std::free(data_);

			size_t newAllocSize = newSize * sizeof(T);
			size_t alignment = 32;
			if (newAllocSize % alignment != 0) {
				newAllocSize += alignment - (newAllocSize % alignment);
			}

			data_ = static_cast<T*>(std::aligned_alloc(alignment, newAllocSize));
			if (!data_) {
				throw std::bad_alloc();
			}

			capacity_ = static_cast<uint32_t>(newSize);
		}

		inline T& operator[](size_t index) {
			return data_[index];
		}

		inline T* data() const { return data_; }

		inline uint32_t size() const { return capacity_; }

	private:
		T* data_;
		uint32_t capacity_;
	};
} // namespace

#endif /* REGEN_ALIGNED_ARRAY_H_ */
