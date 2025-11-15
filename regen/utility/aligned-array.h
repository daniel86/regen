#ifndef REGEN_ALIGNED_ARRAY_H_
#define REGEN_ALIGNED_ARRAY_H_

#include <memory>
#include <cstdlib>
#include <cstring>
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
			allocate(maxElements);
		}

		AlignedArray()
			: data_(nullptr), capacity_(0) {
			// Default constructor initializes to empty state
		}

		~AlignedArray() { std::free(data_); }

		AlignedArray(const AlignedArray&) = delete;
		AlignedArray& operator=(const AlignedArray&) = delete;

		/**
		 * Sets all elements in the array to zero.
		 */
		void setToZero() {
			memset(data_, 0, allocatedSize_);
		}

		/**
		 * @return The capacity of the array.
		 */
		uint32_t capacity() const { return capacity_; }

		/**
		 * @brief Resizes the array to a new size.
		 * @param newSize The new size of the array.
		 * @param preserveData If true, preserves existing data up to the new size.
		 * @throws std::bad_alloc if memory allocation fails.
		 */
		void resize(size_t newSize, bool preserveData = false) {
			T *oldData = data_;
			uint32_t oldAllocatedSize = allocatedSize_;
			capacity_ = newSize;
			allocate(newSize);
			if (preserveData) {
				std::memcpy(data_, oldData, std::min(allocatedSize_, oldAllocatedSize));
			}
			std::free(oldData);
		}

		/**
		 * @brief Access an element at a given index.
		 * @param index The index of the element to access.
		 * @return A reference to the element at the specified index.
		 */
		inline T& operator[](size_t index) const {
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
		uint32_t allocatedSize_ = 0;

		static constexpr size_t Alignment = 32;

		void allocate(size_t count) {
			const size_t totalSize = count * sizeof(T);

			// Ensure totalSize is a multiple of Alignment
			allocatedSize_ = totalSize;
			if (allocatedSize_ % Alignment != 0) {
				allocatedSize_ += Alignment - (allocatedSize_ % Alignment);
			}

			data_ = static_cast<T*>(std::aligned_alloc(Alignment, allocatedSize_));
			if (!data_) {
				throw std::bad_alloc();
			}
		}
	};
} // namespace

#endif /* REGEN_ALIGNED_ARRAY_H_ */
