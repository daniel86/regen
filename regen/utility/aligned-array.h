#ifndef REGEN_ALIGNED_ARRAY_H_
#define REGEN_ALIGNED_ARRAY_H_

#include <memory>
#include <cstdlib>
#include <cstddef>
#include <stdexcept>

namespace regen {
	/**
	 * @brief A dynamically resizable array with 32-byte alignment.
	 * This class provides a simple array-like interface with 32-byte alignment
	 * for SIMD operations. It is designed to be used with types that require
	 * such alignment, like SIMD registers.
	 *
	 * @tparam T The type of elements in the array.
	 */
	template<typename T>
	struct AlignedArray {
	public:
		/**
		 * @brief Constructs an AlignedArray with a specified maximum number of elements.
		 * @param maxElements The maximum number of elements the array can hold.
		 */
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

		/**
		 * Sets all elements in the array to zero.
		 */
		void setToZero() {
			memset(data_, 0, capacity_ * sizeof(T));
		}

		/**
		 * @brief Resizes the array to a new size.
		 * @param newSize The new size of the array.
		 * @throws std::bad_alloc if memory allocation fails.
		 */
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

		/**
		 * @brief Access an element at a given index.
		 * @param index The index of the element to access.
		 * @return A reference to the element at the specified index.
		 */
		inline T& operator[](size_t index) {
			return data_[index];
		}

		/**
		 * @return the pointer to the underlying data.
		 */
		inline T* data() const { return data_; }

		/**
		 * @brief Returns the number of elements in the array.
		 * @return The number of elements in the array.
		 */
		inline uint32_t size() const { return capacity_; }

	private:
		T* data_;
		uint32_t capacity_;
	};
} // namespace

#endif /* REGEN_ALIGNED_ARRAY_H_ */
