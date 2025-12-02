#ifndef REGEN_GL_OBJECT_H_
#define REGEN_GL_OBJECT_H_

#include <regen/config.h>
#include <regen/gl/render-state.h>
#include <regen/utility/ref-ptr.h>

#include <GL/glew.h>
#include "regen/scene/resource.h"

namespace regen {
	/**
	 * \brief Base class for buffer objects.
	 *
	 * Each buffer can generate multiple GL buffers,
	 * the active buffer can be changed with nextObject()
	 * and set_objectIndex().
	 */
	class GLObject : public Resource {
	public:
		/**
		 * Obtain n buffers.
		 */
		typedef void (GLAPIENTRY *CreateObjectFunc)(GLsizei n, uint32_t *buffers);

		/**
		 * Obtain n buffers.
		 */
		typedef void (GLAPIENTRY *CreateObjectFunc2)(GLenum target, GLsizei n, uint32_t *buffers);

		/**
		 * Release n buffers.
		 */
		typedef void (GLAPIENTRY *ReleaseObjectFunc)(GLsizei n, const uint32_t *buffers);

		/**
		 * @param createObjects allocate buffers.
		 * @param releaseObjects delete buffers.
		 * @param numObjects number of buffers to allocate.
		 */
		GLObject(
				CreateObjectFunc createObjects,
				ReleaseObjectFunc releaseObjects,
				uint32_t numObjects = 1);

		/**
		 * @param createObjects allocate buffers.
		 * @param releaseObjects delete buffers.
		 * @param numObjects number of buffers to allocate.
		 */
		GLObject(
				CreateObjectFunc2 createObjects,
				ReleaseObjectFunc releaseObjects,
				GLenum objectTarget,
				uint32_t numObjects = 1);

		GLObject(const GLObject &other);

		virtual ~GLObject();

		/**
		 * Releases and allocates resources again.
		 */
		void resetGL();

		/**
		 * Switch to the next allocated buffer.
		 * Next bind() call will bind the activated buffer.
		 */
		void nextObject();

		/**
		 * Returns the currently active buffer index.
		 */
		uint32_t objectIndex() const { return objectIndex_; }

		/**
		 * Sets the index of the active buffer.
		 */
		void set_objectIndex(uint32_t bufferIndex) { objectIndex_ = bufferIndex % numObjects_; }

		/**
		 * Returns number of buffers allocation
		 * for this Bufferobject.
		 */
		uint32_t numObjects() const { return numObjects_; }

		/**
		 * GL handle for currently active buffer.
		 */
		uint32_t id() const { return ids_[objectIndex_]; }

		/**
		 * Array of GL handles allocated for this buffer.
		 */
		uint32_t *ids() const { return ids_; }

	protected:
		uint32_t *ids_;
		// an atomic copy counter, for protecting deletion of shared data
		ref_ptr<std::atomic<uint32_t>> copyCounter_;
		uint32_t numObjects_;
		uint32_t objectIndex_;
		GLenum objectTarget_ = GL_NONE;
		ReleaseObjectFunc releaseObjects_;
		CreateObjectFunc createObjects_;
		CreateObjectFunc2 createObjects2_;
	};
} // namespace

#endif
