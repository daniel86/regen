/*
 * ref-ptr.h
 *
 *  Created on: 22.03.2011
 *      Author: daniel
 */

#ifndef REF_PTR_H_
#define REF_PTR_H_

#include <iostream>

namespace regen {
	/**
	 * \brief Reference counter template class.
	 */
	template<class T>
	class ref_ptr {
	public:
		/**
		 * static_cast conversion.
		 * @see http://en.cppreference.com/w/cpp/language/static_cast
		 * @param v a reference pointer.
		 * @return casted reference.
		 */
		template<typename K>
		static ref_ptr<T> staticCast(ref_ptr<K> v) {
			ref_ptr<T> casted;
			casted.ptr_ = static_cast<T *>(v.get());
			casted.refCount_ = v.refCount();
			if (casted.ptr_ != NULL) {
				casted.ref();
			}
			return casted;
		}

		/**
		 * dynamic_cast conversion.
		 * @see http://en.cppreference.com/w/cpp/language/static_cast
		 * @param v a reference pointer.
		 * @return casted reference.
		 */
		template<typename K>
		static ref_ptr<T> dynamicCast(ref_ptr<K> v) {
			ref_ptr<T> casted;
			casted.ptr_ = dynamic_cast<T *>(v.get());
			casted.refCount_ = v.refCount();
			if (casted.ptr_ != NULL) {
				casted.ref();
			}
			return casted;
		}

		/**
		 * Create reference counted T* instance by calling new without arguments.
		 * @return references counted instance of T
		 */
		static ref_ptr<T> alloc() { return ref_ptr<T>(new T); }

		/**
		 * Create reference counted T* instance by calling new provided argument.
		 * @param v0 the first constructor argument.
		 * @return references counted instance of T
		 */
		template<typename A>
		static ref_ptr<T> alloc(const A &v0) { return ref_ptr<T>(new T(v0)); }

		/**
		 * Create reference counted T* instance by calling new provided arguments.
		 * @param v0 the first constructor argument.
		 * @param v1 the second constructor argument.
		 * @return references counted instance of T
		 */
		template<typename A, typename B>
		static ref_ptr<T> alloc(const A &v0, const B &v1) { return ref_ptr<T>(new T(v0, v1)); }

		/**
		 * Create reference counted T* instance by calling new provided arguments.
		 * @param v0 the first constructor argument.
		 * @param v1 the second constructor argument.
		 * @param v2 the third constructor argument.
		 * @return references counted instance of T
		 */
		template<typename A, typename B, typename C>
		static ref_ptr<T> alloc(const A &v0, const B &v1, const C &v2) { return ref_ptr<T>(new T(v0, v1, v2)); }

		/**
		 * Create reference counted T* instance by calling new provided arguments.
		 * @param v0 the first constructor argument.
		 * @param v1 the second constructor argument.
		 * @param v2 the third constructor argument.
		 * @param v3 the fourth constructor argument.
		 * @return references counted instance of T
		 */
		template<typename A, typename B, typename C, typename D>
		static ref_ptr<T> alloc(const A &v0, const B &v1, const C &v2, const D &v3) {
			return ref_ptr<T>(new T(v0, v1, v2, v3));
		}

		/**
		 * Create reference counted T* instance by calling new provided arguments.
		 * @param v0 the first constructor argument.
		 * @param v1 the second constructor argument.
		 * @param v2 the third constructor argument.
		 * @param v3 the fourth constructor argument.
		 * @param v4 the fifth constructor argument.
		 * @return references counted instance of T
		 */
		template<typename A, typename B, typename C, typename D, typename E>
		static ref_ptr<T> alloc(const A &v0, const B &v1, const C &v2, const D &v3, const E &v4) {
			return ref_ptr<T>(new T(v0, v1, v2, v3, v4));
		}

		/**
		 * Init without data and reference counter.
		 */
		ref_ptr() : ptr_(nullptr), refCount_(nullptr) {}

		/**
		 * Copy constructor.
		 * Takes a reference on the data pointer of the other ref_ptr.
		 */
		ref_ptr(const ref_ptr<T> &other) : ptr_(other.ptr_), refCount_(other.refCount_) { if (ptr_ != nullptr) { ref(); }}

		/**
		 * Type-safe down-casting constructor.
		 * Takes a reference on the data pointer of the other ref_ptr.
		 */
		template<typename K>
		ref_ptr(ref_ptr<K> other) : ptr_(other.get()), refCount_(other.refCount()) { if (ptr_ != nullptr) { ref(); }}

		/**
		 * Destructor unreferences if data pointer set.
		 */
		~ref_ptr() { if (ptr_ != nullptr) { unref(); }}

		/**
		 * Access data pointer.
		 * Note: If no data set you will get a null pointer here.
		 */
		T *operator->() const { return ptr_; }

		/**
		 * Set from other ref_ptr,
		 * both will share same data and reference counter afterwards.
		 * Old data gets unreferenced.
		 */
		ref_ptr &operator=(ref_ptr<T> other) {
			if (ptr_ != NULL) { unref(); }
			ptr_ = other.ptr_;
			refCount_ = other.refCount_;
			if (ptr_ != NULL) { ref(); }
			return *this;
		}

		/**
		 * Compares ref_ptr by data pointer.
		 */
		template<typename K>
		bool operator==(const ref_ptr<K> &other) const { return ptr_ == other.get(); }

		/**
		 * Compares ref_ptr by data pointer.
		 */
		template<typename K>
		bool operator<(const ref_ptr<K> &other) const { return ptr_ < other.get(); }

		/**
		 * Returns referenced pointer.
		 * Do not delete, this is done in ref_ptr destructor.
		 * Returns NULL if no data pointer set.
		 */
		T *get() const { return ptr_; }

		/**
		 * Pointer to reference counter.
		 */
		unsigned int *refCount() const { return refCount_; }

	private:
		T *ptr_;
		unsigned int *refCount_;

		ref_ptr(T *ptr) : ptr_(ptr), refCount_(new unsigned int) { *refCount_ = 1; }

		void ref() {
			*refCount_ += 1;
		}

		void unref() {
			*refCount_ -= 1;
			if (*refCount_ == 0) {
				delete refCount_;
				refCount_ = nullptr;

				delete ptr_;
				ptr_ = nullptr;
			}
		}
	};
} // namespace

#endif /* REF_PTR_H_ */
